"""Seed-scoped, at-most-once transient effects. Never modifies a game save."""
import asyncio
import ctypes as c
import hashlib
import json
from pathlib import Path
import sqlite3
import struct
import time

TRAPS = {0x020234: (1, 'MP Recharge Trap'), 0x020235: (2, 'Stun Trap')}
FIELDS = ('magic','id','epoch','kind','status','reason','calls','hp','mp','charge',
          'charge_time','badstat','duration','state','paused','context_flags','actor','last_id')

def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'))

def normalize_slot(value):
    if type(value) is not dict:
        raise ValueError('Trap slot data must be an object')
    base = dict(value)
    if base.get('schema_version') == 4:
        return base, None
    if type(base.get('schema_version')) is not int or base['schema_version'] != 5 or base.get('item_schema_version') != 6:
        raise ValueError('Unsupported trap slot version')
    config = base.pop('traps', None)
    if type(config) is not dict or set(config) != {'percentage','mp_weight','stun_weight'}:
        raise ValueError('Invalid trap configuration')
    if any(type(v) is not int or not 0 <= v <= 100 for v in config.values()):
        raise ValueError('Trap settings must be integers from 0 to 100')
    if config['percentage'] and not (config['mp_weight'] or config['stun_weight']):
        raise ValueError('Enabled traps need a nonzero weight')
    base.update(schema_version=4, item_schema_version=5)
    return base, canonical(config)

def receipt_identity(net):
    return canonical([net.item,net.location,net.player,net.flags])

class TrapJournal:
    def __init__(self, root, key):
        self.key = key
        scope = canonical([key.seed_name,key.team,key.slot])
        root = Path(root); root.mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(root/(hashlib.sha256(scope.encode()).hexdigest()+'.sqlite'))
        self.db.row_factory = sqlite3.Row
        self.db.execute('PRAGMA synchronous=FULL')
        self.db.execute('CREATE TABLE IF NOT EXISTS scope (id INTEGER PRIMARY KEY, identity TEXT NOT NULL)')
        self.db.execute('CREATE TABLE IF NOT EXISTS traps (idx INTEGER PRIMARY KEY, identity TEXT NOT NULL, item INTEGER NOT NULL, state TEXT NOT NULL, outcome TEXT)')
        with self.db:
            self.db.execute('INSERT OR IGNORE INTO scope VALUES (1,?)',(scope,))
            if self.db.execute('SELECT identity FROM scope WHERE id=1').fetchone()[0] != scope:
                raise ValueError('Foreign trap journal')
            # Interrupted native attempts may have happened. Never replay them.
            self.db.execute("UPDATE traps SET state='done', outcome='uncertain' WHERE state='claimed'")

    def observe(self, snapshot):
        if snapshot.key != self.key:
            raise ValueError('Foreign trap history')
        rows = {r['idx']: r for r in self.db.execute('SELECT * FROM traps')}
        for index,row in rows.items():
            if index >= len(snapshot.items) or receipt_identity(snapshot.items[index]) != row['identity']:
                raise ValueError('Trap history changed or shrank')
        with self.db:
            for index,net in enumerate(snapshot.items):
                if net.item in TRAPS and index not in rows:
                    self.db.execute("INSERT INTO traps VALUES (?,?,?,'pending',NULL)",
                                    (index,receipt_identity(net),net.item))

    def next(self):
        row = self.db.execute("SELECT * FROM traps WHERE state='pending' ORDER BY idx LIMIT 1").fetchone()
        return dict(row) if row else None

    def claim(self, index):
        with self.db:
            if self.db.execute("UPDATE traps SET state='claimed' WHERE idx=? AND state='pending'",(index,)).rowcount != 1:
                raise ValueError('Trap already claimed')

    def finish(self, index, outcome, *, deferred=False):
        with self.db:
            if self.db.execute("UPDATE traps SET state=?,outcome=? WHERE idx=? AND state='claimed'",
                               ('pending' if deferred else 'done',outcome,index)).rowcount != 1:
                raise ValueError('Trap completion mismatch')

    def close(self):
        self.db.close()

async def request(owner, kind=0, identity=0):
    owner.allowed_commands = tuple(sorted(set(owner.allowed_commands+(14,))))
    async with owner._async_lock:
        if not await owner._sync():
            raise RuntimeError('Trap requires current AP authority')
        epoch = owner.block.meter_access.epoch
        packet = struct.pack('<QQQII',0x32343252545041,identity,epoch,kind,0)
        owner.block.requested_authority = owner._token
        owner.block.proposal_size = len(packet)
        c.memmove(c.addressof(owner.block.proposal),packet,len(packet))
        reply = await owner._offload(owner._send,14)
        if owner.block.proposal_size != 144:
            raise RuntimeError('Uncertain trap completion; do not retry')
        value = dict(zip(FIELDS,struct.unpack('<18Q',bytes(owner.block.proposal)[:144])))
        if (value['magic'],value['id'],value['epoch'],value['status']) != (0x32343252545041,identity,epoch,reply.status):
            raise RuntimeError('Trap acknowledgment differs; do not retry')
        return value

class TrapService:
    def __init__(self, owner, root, key, *, clock=time.monotonic, send=request):
        self.owner=owner; self.journal=TrapJournal(root,key); self.clock=clock; self.send=send
        self.ready_since=None; self.epoch=None; self.next_effect=0; self.lock=asyncio.Lock()

    def defer(self):
        self.ready_since=None

    async def tick(self, client):
        async with self.lock:
            snapshot=client.context.current_snapshot()
            self.journal.observe(snapshot)
            # A completed/uncertain attempt may need its notice after reconnect.
            with client.notice_queue.db:
                for row in self.journal.db.execute("SELECT idx FROM traps WHERE state='done'"):
                    client.notice_queue.db.execute("UPDATE notices SET state='queued' WHERE event=? AND state='trap_waiting'",('r:'+str(row['idx']),))
            item=self.journal.next()
            if not item or client.delivery.pending is not None or client.notice_queue.next() is not None:
                self.defer(); return
            value=await self.send(self.owner)
            now=self.clock()
            # Stabilize the owner, not the action state: combat remains eligible.
            if (value['status'] != 1 or value['paused'] or
                not value['hp'] or value['badstat'] or value['context_flags']&2):
                self.defer(); return
            if self.epoch != value['epoch'] or self.ready_since is None:
                self.epoch=value['epoch']; self.ready_since=now; return
            if now-self.ready_since < 2 or now < self.next_effect:
                return
            if not client.context.snapshot_is_current(snapshot):
                self.defer(); return
            self.journal.claim(item['idx'])  # Durable before native publication.
            result=await self.send(self.owner,TRAPS[item['item']][0],item['idx']+1)
            # Only explicit pre-call refusals are retryable, never lost replies.
            deferred=result['status']==2 and result['reason'] in (1,2,3,5,6,11) and result['last_id'] != item['idx']+1
            self.journal.finish(item['idx'],str(result['reason']),deferred=deferred)
            self.defer()
            if not deferred:
                self.next_effect=now+15
            client.context.observe(dict(event='trap',index=item['idx'],item=item['item'],
                                        status=result['status'],reason=result['reason'],deferred=deferred))

    def close(self):
        self.journal.close()
