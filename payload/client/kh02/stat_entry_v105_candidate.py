"""Stat-enabled sibling of the proven mixed receipt preparation."""
from . import area_map_entry_v101_candidate as prior
from .stat_receipts_v105_candidate import build_policy
from .fresh_replay_preparation_candidate import prepare_replay

attach = prior.attach

def supported_effects():
    return tuple(sorted(set(prior.supported_effects()) | {0x02020b, 0x02020c, 0x020205, 0x020206, 0x020207}))

def prepare(session, storage, *, cookie, grant_nonce, journal_path, runtime_start):
    config, snapshot = session.context.goal_admission.current(session.context)
    if config.starting_world != runtime_start:
        raise ValueError('Seed starting world does not match selected runtime')
    build_policy(snapshot.received_items())
    result = prepare_replay(session, storage, cookie=cookie, grant_nonce=grant_nonce,
        supported=supported_effects(), journal_path=journal_path)
    after, current = session.context.goal_admission.current(session.context)
    if after != config or current != snapshot or not session.context.snapshot_is_current(snapshot):
        raise ValueError('Seed admission changed during preparation')
    return result
