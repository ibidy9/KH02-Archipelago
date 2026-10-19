"""Castle/level checks from admitted v127 samples; no raw-route admission.

Additive sibling. Existing objective/chest reporting and rewards remain intact.
The journal retains observed completions, not native receipts or AP items.
"""
from dataclasses import asdict,dataclass
import json
from pathlib import Path
import sqlite3
import struct
from .objective_checks_v116_candidate import ObjectiveReporter
from .progress_checks_v127_candidate import completed_level_checks
from .level_transport_v186_candidate import LiveLevelObservation as ExitObservation

SUPPORTED=frozenset(range(0x020400,0x020414))|frozenset(range(0x020420,0x020425))|{0x020450,0x020440,0x020441,0x020451,0x020452,0x020453,0x020454,0x020455,0x020456,0x020442}|frozenset(range(0x020430,0x020437))|frozenset(range(0x020470,0x020473))|{0x020443}|frozenset(range(0x020460,0x020465))

def completed(observation):
    if (type(observation) is not ExitObservation or
        any(type(v) is not int or not 0<v<2**64 for v in
            (observation.cookie,observation.epoch,observation.load_id,observation.owner)) or
        type(observation.header) is not bytes or len(observation.header)!=64 or
        type(observation.switches) is not bytes or len(observation.switches)!=8):
        raise ValueError('Exact admitted progress observation required')
    level=observation.live_level
    # An uninitialized or malformed level must not manufacture completions.
    # Keep independently observed flags, as the diagnostic decoder does.
    result=set(completed_level_checks(level)) if 50<=level<=99 else set()
    flags=observation.switches
    for i,mask in enumerate((8,16,32,64,128)):
        if flags[1]&mask:result.add(0x020420+i)
    if flags[5]&4:result.add(0x020450)
    if type(observation.exits) is not int or observation.exits & ~15:raise ValueError("Invalid exit witness")
    if observation.exits & 1:result.add(0x020440)
    if observation.exits & 2:result.add(0x020441)
    if observation.exits & 4:result.add(0x020442)
    if observation.exits & 8:result.add(0x020443)
    for mask,location in ((0x10,0x020454),(0x20,0x020455),(0x40,0x020456)):
        if flags[5]&mask:result.add(location)
    from .progress_checks_v127_candidate import completed_switch_checks
    result.update(completed_switch_checks(flags[:6]))
    # Permanent route completions commit only after sequential Phantom victories.
    # Temporary route selection bits never count. First/final boss flags corroborate.
    routes=(flags[2]&0x38).bit_count()
    if routes>=1 and flags[5]&1:result.add(0x020451)
    if routes>=2 and flags[5]&1:result.add(0x020452)
    if routes==3 and flags[5]&8:result.add(0x020453)
    if type(observation.darksides) is not int or observation.darksides & ~7:raise ValueError("Invalid Darkside witness")
    result.update(0x020470+i for i in range(3) if observation.darksides & (1<<i))
    if type(observation.rush) is not int or observation.rush & ~31:raise ValueError("Invalid rush witness")
    result.update(0x020460+i for i in range(5) if observation.rush & (1<<i))
    return frozenset(result)

class ProgressJournal:
    """Separate seed/team/slot-bound monotonic observation ledger.

    Never opens a game save or receipt database. AP acknowledgements remain the
    sole authority for whether a retained observation needs another send.
    """
    def __init__(self,path,*,key):
        self.key=key
        self.db=sqlite3.connect(Path(path),timeout=5)
        try:
            self.db.execute('PRAGMA synchronous=FULL')
            tables={r[0] for r in self.db.execute("SELECT name FROM sqlite_master WHERE type='table'")}
            scope=json.dumps(asdict(key),sort_keys=True,separators=(',',':'))
            if not tables:
                with self.db:
                    self.db.execute('CREATE TABLE scope(version INTEGER NOT NULL, identity TEXT NOT NULL)')
                    self.db.execute('CREATE TABLE observed(location INTEGER PRIMARY KEY)')
                    self.db.execute('INSERT INTO scope VALUES(1,?)',(scope,))
            elif tables!={'scope','observed'}:
                raise ValueError('Not a progress observation journal')
            if list(self.db.execute('SELECT version,identity FROM scope'))!=[(1,scope)]:
                raise ValueError('Foreign progress journal scope or version')
            self.observed()
        except BaseException:
            self.db.close();raise
    def observed(self):
        values=frozenset(row[0] for row in self.db.execute('SELECT location FROM observed'))
        if not values<=SUPPORTED:raise ValueError('Unrecognized progress journal location')
        return values
    def retain(self,locations):
        values=tuple(locations)
        if any(type(v) is not int or v not in SUPPORTED for v in values):raise ValueError('Unsupported progress completion')
        with self.db:
            self.db.executemany('INSERT OR IGNORE INTO observed(location) VALUES(?)',((v,) for v in values))
    def close(self):self.db.close()

@dataclass(frozen=True)
class _Report:
    native:ExitObservation
    locations:frozenset
    @property
    def cookie(self):return self.native.cookie
    @property
    def epoch(self):return self.native.epoch
    @property
    def load_id(self):return self.native.load_id
    def completed(self):return self.locations

class _Context:
    def __init__(self,context,journal):self.context=context;self.journal=journal
    def __getattr__(self,name):return getattr(self.context,name)
    async def check_locations(self,locations):
        # ObjectiveReporter has checked scope, receipt binding and current
        # native/AP authority. Commit before the first network yield.
        self.journal.retain(locations)
        await self.context.check_locations(locations)

class ProgressReporter:
    def __init__(self,context,*,key,cookie,read,current,journal):
        if journal.key!=key:raise ValueError('Foreign progress journal')
        async def capture():
            value=await read()
            if value is None:return None
            observation,receipt,epoch=value
            report=_Report(observation,completed(observation)|journal.observed())
            return report,receipt,epoch
        self.reporter=ObjectiveReporter(_Context(context,journal),key=key,cookie=cookie,
            read=capture,current=lambda report:current(report.native))
    async def poll(self):return await self.reporter.poll()
