"""Isolated offline reference transaction, not a native save-worker backend.

All associated records and their publication binding commit together. Runtime
owner/load/revision tags are validated on input but are not durable receipt IDs.
No existing single-record journal or live grant receipt is migrated here.
"""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import sqlite3
import struct

from .receipt_bundle_candidate import decode, validate_history
from .received import DeliveryKey, ReceivedItem
from .save_association_candidate import DurableAssociation
from .save_publication_candidate import Publication


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=True).encode('ascii')


def framed_digest(*parts):
    return hashlib.sha256(b''.join(len(part).to_bytes(8, 'little') + part for part in parts)).hexdigest()


def _scope(value):
    if (not isinstance(value, list) or len(value) != 3 or type(value[0]) is not str or
            not 1 <= len(value[0].encode('utf-8')) <= 4096 or
            any(type(n) is not int or not 0 <= n < 2**32 for n in value[1:])):
        raise ValueError('Invalid durable AP scope')
    return canonical(value)


def _members(value):
    if (not isinstance(value, list) or len(value) > 4096 or any(not isinstance(row, list) or len(row) != 3 or
            any(type(n) is not int or not 0 <= n < 2**64 for n in row) for row in value) or
            any(value[i][0] >= value[i+1][0] for i in range(len(value)-1))):
        raise ValueError('Invalid durable receipt identity')
    return canonical(value)


def _publication_records(token, container, association, binding):
    if (type(token) is not str or not token or len(token) > 256 or type(container) is not bytes or
            len(container) != 0x1725b0 or container[:16] != struct.pack('<4sIII', b'SAVE', 1, 0x1725b0, 0) or
            type(association) is not bytes or Publication(token, container, association).binding != binding):
        raise ValueError('Corrupt publication binding')
    value = json.loads(association)
    if (canonical(value) != association or type(value) is not dict or
            set(value) != {'version', 'scope', 'bindings', 'sets'} or type(value['version']) is not int or value['version'] != 1 or
            type(value['sets']) is not list or len(value['sets']) > 64 or
            type(value['bindings']) is not list or len(value['bindings']) > 100):
        raise ValueError('Invalid multi-record association')
    scope = _scope(value['scope'])
    sets = [_members(items) for items in value['sets']]
    if sets != sorted(set(sets)):
        raise ValueError('Noncanonical membership table')
    records, used, seen, previous = [], set(), {}, -1
    for row in value['bindings']:
        if (type(row) is not list or len(row) != 2 or any(type(n) is not int for n in row) or
                not previous < row[0] < 100 or not 0 <= row[1] < len(sets)):
            raise ValueError('Invalid record binding')
        index, member_index = row
        record = container[0xb0+index*0x3b40:0xb0+(index+1)*0x3b40]
        members = sets[member_index]
        if record in seen and seen[record] != members:
            raise ValueError('Conflicting published record membership')
        records.append((scope, record, members));seen[record] = members;used.add(member_index);previous = index
    if used != set(range(len(sets))):
        raise ValueError('Unreferenced receipt set')
    return records


SCHEMA = '''CREATE TABLE metadata(version INTEGER NOT NULL);
INSERT INTO metadata VALUES(1);
CREATE TABLE publications(token TEXT PRIMARY KEY, container BLOB NOT NULL,
                          association BLOB NOT NULL, binding TEXT NOT NULL) WITHOUT ROWID;
CREATE TABLE records(scope BLOB NOT NULL, record_hash TEXT NOT NULL,
                     record BLOB NOT NULL, members BLOB NOT NULL, digest TEXT NOT NULL,
                     PRIMARY KEY(scope,record_hash)) WITHOUT ROWID;'''


@dataclass(frozen=True)
class PreparedRecords:
    publication: Publication
    scope: bytes
    records: tuple[tuple[bytes, bytes], ...]


def prepare(token, raw_bundle, history, supported):
    if type(token) is not str or not token or len(token) > 256:
        raise ValueError('Invalid publication token')
    bundle = decode(raw_bundle)
    validate_history(bundle, history, supported)
    scope = canonical([bundle.key.seed_name, bundle.key.team, bundle.key.slot])
    memberships = {value.revision: canonical([[item.index, item.item_id, item.player] for item in value.items])
                   for value in bundle.sets}
    # Canonical durable identities depend on actual membership, not volatile
    # bank instances or revision counters. Sort and deduplicate the values.
    unique = sorted(set(memberships.values()))
    indices = {members: index for index, members in enumerate(unique)}
    bindings = []
    records = []
    seen = {}
    for index, revision in enumerate(bundle.revisions):
        if not revision:
            continue  # Unknown records remain unknown, including all manual slots.
        members = memberships[revision]
        start = 0xb0 + index * 0x3b40
        record = bundle.container[start:start + 0x3b40]
        if record in seen and seen[record] != members:
            raise ValueError('Conflicting membership inside one publication')
        seen[record] = members
        bindings.append([index, indices[members]])
        records.append((record, members))
    association = canonical(dict(version=1, scope=json.loads(scope), bindings=bindings,
                                 sets=[json.loads(members) for members in unique]))
    return PreparedRecords(Publication(token, bundle.container, association), scope, tuple(records))


def initialize(path: Path):
    with path.open('xb'):
        pass
    db = sqlite3.connect(path, timeout=0)
    try:
        db.execute('PRAGMA synchronous=FULL')
        db.executescript('BEGIN IMMEDIATE;\n' + SCHEMA + '\nCOMMIT;')
    finally:
        db.close()


class MultiRecordJournal:
    def __init__(self, path: Path, *, readonly=False):
        if type(readonly) is not bool:raise ValueError('Invalid journal access mode')
        mode='ro' if readonly else 'rw'
        self.db = sqlite3.connect(path.resolve().as_uri() + '?mode='+mode, uri=True, timeout=0)
        try:
            if readonly:self.db.execute('BEGIN')  # Audit and lookup share one read snapshot.
            if not readonly:self.db.execute('PRAGMA synchronous=FULL')
            if self.db.execute('SELECT version FROM metadata').fetchall() != [(1,)]:
                raise ValueError('Unsupported multi-record journal schema')
            if self.db.execute('PRAGMA quick_check').fetchall() != [('ok',)]:
                raise ValueError('Journal integrity check failed')
            self.read_all()  # Also verifies payload checksums and canonical forms.
            referenced = set()
            for row in self.db.execute('SELECT * FROM publications'):
                for scope, record, members in _publication_records(*row):
                    record_hash = hashlib.sha256(record).hexdigest()
                    actual = self.db.execute('SELECT record,members,digest FROM records WHERE scope=? AND record_hash=?',
                                             (scope, record_hash)).fetchone()
                    if actual != (record, members, framed_digest(scope, record, members)):
                        raise ValueError('Publication has a missing or conflicting record association')
                    referenced.add((scope, record_hash))
            if referenced != set(self.db.execute('SELECT scope,record_hash FROM records')):
                raise ValueError('Orphaned record association')
        except BaseException:
            self.db.close()
            raise

    def close(self):
        self.db.close()

    def read_all(self):
        result = []
        for scope, record_hash, record, members, digest in self.db.execute('SELECT * FROM records'):
            if (any(type(value) is not bytes for value in (scope, record, members)) or len(record) != 0x3b40 or
                    hashlib.sha256(record).hexdigest() != record_hash or
                    framed_digest(scope, record, members) != digest):
                raise ValueError('Record association checksum mismatch')
            key = json.loads(scope)
            items = json.loads(members)
            if canonical(key) != scope or canonical(items) != members:
                raise ValueError('Noncanonical record association')
            _scope(key);_members(items)
            result.append(DurableAssociation(DeliveryKey(*key), record, tuple(ReceivedItem(*row) for row in items)))
        return tuple(result)

    def append(self, token, raw_bundle, history, supported, *, _checkpoint=None):
        prepared = prepare(token, raw_bundle, history, supported)
        publication = prepared.publication
        self.db.execute('BEGIN IMMEDIATE')
        try:
            row = self.db.execute('SELECT container,association,binding FROM publications WHERE token=?', (token,)).fetchone()
            expected = (publication.container, publication.association, publication.binding)
            if row is not None and row != expected:
                raise ValueError('Publication token reused with different contents')
            # Validate existing stored payloads before accepting any new association.
            self.read_all()
            for index, (record, members) in enumerate(prepared.records):
                record_hash = hashlib.sha256(record).hexdigest()
                digest = framed_digest(prepared.scope, record, members)
                existing = self.db.execute('SELECT record,members,digest FROM records WHERE scope=? AND record_hash=?',
                                           (prepared.scope, record_hash)).fetchone()
                if existing is not None and existing != (record, members, digest):
                    raise ValueError('Identical save bytes or digest have conflicting membership')
                if existing is None:
                    self.db.execute('INSERT INTO records VALUES(?,?,?,?,?)',
                                    (prepared.scope, record_hash, record, members, digest))
                if _checkpoint:
                    _checkpoint('record-' + str(index))
            if row is None:
                self.db.execute('INSERT INTO publications VALUES(?,?,?,?)', (token, *expected))
            if _checkpoint:
                _checkpoint('before-commit')
            self.db.commit()
            if _checkpoint:
                _checkpoint('after-commit')
            return publication.binding  # No acknowledgment is returned before commit.
        except BaseException:
            self.db.rollback()
            raise
