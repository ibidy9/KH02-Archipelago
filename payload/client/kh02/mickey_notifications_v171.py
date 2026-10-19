"""Seed-scoped UI journal. Never writes item receipts, game memory or saves."""
import json,hashlib,sqlite3
from pathlib import Path
from .mickey_receipts_v171 import build_policy, HISTORY_ONLY, TRAP_IDS
from .production_schema import ItemCategory
from .area_map_receipts_v101_candidate import AreaMapCategory,ZodiacCategory
from .pickup_text_v112_candidate import format_pickup

def canonical(v):return json.dumps(v,sort_keys=True,separators=(',',':'),ensure_ascii=True)

class NotificationQueue:
    def __init__(self,root,key):
        self.key=key
        scope=canonical([key.seed_name,key.team,key.slot])
        root=Path(root);root.mkdir(parents=True,exist_ok=True)
        self.path=root/(hashlib.sha256(scope.encode()).hexdigest()+'.sqlite')
        self.db=sqlite3.connect(self.path)
        self.db.row_factory=sqlite3.Row
        self.db.execute('PRAGMA synchronous=FULL')
        self.db.execute('CREATE TABLE IF NOT EXISTS scope (singleton INTEGER PRIMARY KEY CHECK(singleton=1), identity TEXT NOT NULL)')
        self.db.execute('CREATE TABLE IF NOT EXISTS notices (id INTEGER PRIMARY KEY AUTOINCREMENT, event TEXT UNIQUE NOT NULL, identity TEXT NOT NULL, text TEXT NOT NULL, full_text TEXT NOT NULL, state TEXT NOT NULL)')
        with self.db:
            self.db.execute('INSERT OR IGNORE INTO scope VALUES (1,?)',(scope,))
            if self.db.execute('SELECT identity FROM scope').fetchone()[0]!=scope:raise ValueError('Foreign notification scope')

    def _put(self,event,identity,text,state):
        identity=canonical(identity)
        row=self.db.execute('SELECT * FROM notices WHERE event=?',(event,)).fetchone()
        if row:
            if row['identity']!=identity:raise ValueError('Notification identity changed')
            return row
        self.db.execute('INSERT INTO notices(event,identity,text,full_text,state) VALUES (?,?,?,?,?)',
            (event,identity,text.text,text.full_text,state))
        return None

    def observe(self,snapshot,membership,item_name):
        if snapshot.key!=self.key or membership.key!=self.key:raise ValueError('Foreign AP snapshot or membership')
        policy=build_policy(snapshot.received_items())
        applied={i.index:i for i in membership.items}
        if len(applied)!=len(membership.items) or any(i>=len(policy.items) or policy.items[i]!=v for i,v in applied.items()):
            raise ValueError('Unreceived membership')
        with self.db:
            for index,(net,descriptor) in enumerate(zip(snapshot.items,policy.descriptors)):
                native=descriptor.category in (ItemCategory.NATIVE_CONSUMABLE,ItemCategory.NATIVE_WARDROBE,
                    ItemCategory.ABSOLUTE_STAT,AreaMapCategory.NATIVE_AREA_MAP,ZodiacCategory.NATIVE_ZODIAC)
                logical=net.item in HISTORY_ONLY
                state='trap_waiting' if net.item in TRAP_IDS else 'native' if native else 'queued' if logical else 'existing' if index in applied else 'waiting'
                old=self._put('r:'+str(index),[net.item,net.location,net.player,net.flags],
                    format_pickup(item_name(net.item)),state)
                if old and old['state']=='waiting' and index in applied:
                    self.db.execute("UPDATE notices SET state='queued' WHERE id=?",(old['id'],))
            count=self.db.execute("SELECT count(*) FROM notices WHERE event LIKE 'r:%'").fetchone()[0]
            if count>len(snapshot.items):raise ValueError('Notification history shrank')
        return policy

    def outgoing(self,snapshot,args,*,known_locations,item_name,player_name):
        if snapshot.key!=self.key:raise ValueError('Foreign outgoing scope')
        if args.get('type')!='ItemSend':return False
        net=args.get('item');receiver=args.get('receiving')
        if not isinstance(net,(list,tuple)) or len(net)!=4 or any(type(x) is not int for x in net) or type(receiver) is not int:
            raise ValueError('Malformed structured ItemSend')
        item,location,sender,flags=net
        if sender!=self.key.slot or receiver==self.key.slot:return False
        if location not in known_locations or not 1<=receiver<=65535 or item<0:raise ValueError('Unknown outgoing check/recipient')
        text=format_pickup(item_name(item,receiver),recipient=player_name(receiver),recipient_slot=receiver)
        with self.db:self._put('o:'+str(location),[item,location,sender,receiver,flags],text,'queued')
        return True

    def next(self):
        row=self.db.execute("SELECT * FROM notices WHERE state IN ('waiting','queued') ORDER BY id LIMIT 1").fetchone()
        return dict(row) if row and row['state']=='queued' else None

    def claim(self,identity):
        with self.db:
            if self.db.execute("UPDATE notices SET state='reserved' WHERE id=? AND state='queued'",(identity,)).rowcount!=1:
                raise ValueError('Notification was already reserved')

    def finish(self,identity,state):
        if state not in ('displayed','cancelled','refused'):raise ValueError('Invalid terminal UI state')
        with self.db:
            if self.db.execute("UPDATE notices SET state=? WHERE id=? AND state='reserved'",(state,identity)).rowcount!=1:
                raise ValueError('Notification completion mismatch')

    def close(self):self.db.close()
