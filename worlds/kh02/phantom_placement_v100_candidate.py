"""Protect every player's progression/useful items behind the optional final boss."""
from BaseClasses import ItemClassification, LocationProgressType

FINAL_CHECK_IDS = frozenset((0x020464, 0x0202aa))


def install(world, rows, *, allow_important):
    if type(allow_important) is not bool:
        raise ValueError('Explicit boolean Phantom reward option required')
    targets=[]
    for row in rows:
        if row['id'] not in FINAL_CHECK_IDS:
            continue
        location=world.get_location(row['name'])
        if (location.player!=world.player or location.address!=row['id']
                or location in targets or not callable(location.item_rule)):
            raise ValueError('Final Phantom location identity differs')
        targets.append(location)
    if allow_important:
        return 0  # Do not undo another exclusion or the Zodiac placement rule.
    for location in targets:
        previous=location.item_rule
        location.progress_type=LocationProgressType.EXCLUDED
        location.item_rule=lambda item, old=previous: old(item) and not (
            item.classification & (ItemClassification.progression | ItemClassification.useful))
    return len(targets)
