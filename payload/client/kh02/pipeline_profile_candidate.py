"""Explicit startup-only system-file policy; no save writes or classification."""
import hashlib
import json
import os
from pathlib import Path

SYSTEM='KH2.8_system_WW.png'
POLICY='capture-system-startup-v1'


def compare(expected,actual,policy=None):
    if policy is None:
        if actual!=expected:raise ValueError('Protected profile changed')
        return
    if policy!=POLICY or SYSTEM not in expected or 'KH0.2BbS_WW.png' not in expected:
        raise ValueError('Invalid startup profile policy')
    if set(actual)!=set(expected) or any(actual[k]!=v for k,v in expected.items() if k!=SYSTEM):
        raise ValueError('Protected profile changed outside the named system file')


def capture(preparation,value):
    """Exclusive evidence copy before ARM. Failure leaves the attempt consumed."""
    from .pipeline_preparation_candidate import inventory
    profile=Path(value['profile_path']);observed=inventory(profile)
    compare(value['profile_files'],observed,value.get('profile_policy'))
    raw=(profile/SYSTEM).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=observed[SYSTEM]:
        raise ValueError('System file changed during capture')
    output=preparation.parent/'startup-profile';output.mkdir()
    with (output/SYSTEM).open('xb') as stream:
        stream.write(raw);stream.flush();os.fsync(stream.fileno())
    if inventory(profile)!=observed:
        raise ValueError('Profile changed during startup capture')
    report=dict(policy=POLICY,observed=observed,baseline=value['profile_files'],
        changed_files=[k for k,v in observed.items() if value['profile_files'][k]!=v],
        system_contents_classified=False,authorizes_delivery=False)
    with (output/'capture.json').open('x') as stream:
        json.dump(report,stream,indent=2);stream.write('\n');stream.flush();os.fsync(stream.fileno())
    return hashlib.sha256((output/'capture.json').read_bytes()).hexdigest()


def verify_capture(preparation,value,claim):
    from .pipeline_preparation_candidate import inventory
    output=preparation.parent/'startup-profile'
    if output.is_symlink() or output.is_junction():raise ValueError('Linked startup capture')
    actual=inventory(output)
    if set(actual)!={SYSTEM,'capture.json'} or actual['capture.json']!=claim.get('startup_capture_sha256'):
        raise ValueError('Startup capture is missing or changed')
    report=json.loads((output/'capture.json').read_text())
    observed=inventory(Path(value['profile_path']))
    compare(value['profile_files'],observed,value.get('profile_policy'))
    if (report.get('policy')!=POLICY or report.get('baseline')!=value['profile_files'] or
            report.get('observed')!=observed or actual[SYSTEM]!=observed[SYSTEM]):
        raise ValueError('Startup capture differs from the current profile')
