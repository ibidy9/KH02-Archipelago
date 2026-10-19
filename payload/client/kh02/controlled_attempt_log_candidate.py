"""Exclusive attempt records, retained even when an acknowledgment is lost."""
import json,os
from pathlib import Path

def record_attempt(directory,entry):
    directory=Path(directory)
    attempt=entry.get('attempt');state=entry.get('state')
    if type(attempt) is not int or not 1<=attempt<=4096 or state not in ('reserved','acknowledged'):
        raise ValueError('Invalid controlled attempt')
    if not directory.is_dir():raise ValueError('Prepared attempt directory required')
    if state=='acknowledged':
        previous=json.loads((directory/f'{attempt:04d}-reserved.json').read_text(encoding='utf-8'))
        if any(previous.get(k)!=entry.get(k) for k in ('attempt','epoch','item','proposal')):
            raise ValueError('Acknowledgment differs from reserved request')
    data=json.dumps(entry,sort_keys=True,indent=2).encode('utf-8')
    with (directory/f'{attempt:04d}-{state}.json').open('xb') as f:
        f.write(data);f.flush();os.fsync(f.fileno())
