"""KH02 canonical names; foreign games resolve only through their AP slot data."""
import json
from pathlib import Path
GAME='Kingdom Hearts 0.2 Birth by Sleep'
NAMES={int(k):v for k,v in json.loads((Path(__file__).resolve().parents[2]/'data/notification-item-names-v115.json').read_text()).items()}
def item_name(context,item,slot=None):
    slot=context.expected_key.slot if slot is None else slot
    info=getattr(context,'slot_info',{}).get(slot)
    game=GAME if slot==context.expected_key.slot else getattr(info,'game',None)
    if game==GAME and item in NAMES:return NAMES[item]
    value=context.item_names.lookup_in_slot(item,slot)
    return 'Archipelago Item' if value.startswith('Unknown item (') else value
