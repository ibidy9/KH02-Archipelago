"""Schema5 preparation and one shared-owner goal service; explicit opt-in."""
from dataclasses import dataclass
from .fresh_replay_preparation_candidate import prepare_replay
from .goal_session_v89_candidate import GoalSession
from .area_map_receipts_v101_candidate import build_policy,HISTORY_ONLY,AREA_MAP_IDS
from .zodiac_receipts_v99_candidate import ZODIAC_IDS
from .production_schema import ITEM_DESCRIPTORS
from .default_action_items import ACTION_BITS
from .style_gate_v47 import MAGIC_IDS


@dataclass(frozen=True)
class GoalSlotView:
    goal:str
    starting_world:str

@dataclass(frozen=True)
class PreparedView:
    storage:object
    slot_data:GoalSlotView

@dataclass(frozen=True)
class AttachedView:
    context:object
    transport:object
    process:object
    prepared:PreparedView

def supported_effects():
    result=[]
    from .received import ReceivedItem
    for id in sorted(set(ITEM_DESCRIPTORS)|set(ACTION_BITS)|set(MAGIC_IDS)|set(ZODIAC_IDS)|set(AREA_MAP_IDS)):
        if id in HISTORY_ONLY:continue
        try:build_policy((ReceivedItem(0,id,0),))
        except ValueError:continue
        result.append(id)
    return tuple(result)

def prepare(session,storage,*,cookie,grant_nonce,journal_path,runtime_start):
    config,snapshot=session.context.goal_admission.current(session.context)
    if config.starting_world!=runtime_start:
        raise ValueError('Seed starting world does not match selected runtime')
    build_policy(snapshot.received_items())
    result=prepare_replay(session,storage,cookie=cookie,grant_nonce=grant_nonce,
        supported=supported_effects(),journal_path=journal_path)
    config_after,current=session.context.goal_admission.current(session.context)
    if config_after!=config or not session.context.snapshot_is_current(snapshot) or current!=snapshot:
        raise ValueError('Seed admission changed during preparation')
    return result

def attach(owner,journal_root):
    owner._check_session()
    context=owner.session.context
    config,snapshot=context.goal_admission.current(context)
    if snapshot.key!=owner.prepared.storage.key:raise ValueError('Prepared storage scope differs')
    view=AttachedView(context,owner,owner.proof,PreparedView(owner.prepared.storage,
        GoalSlotView(config.selected_goal.value,config.starting_world)))
    return GoalSession(view,context.goal_admission,journal_root)
