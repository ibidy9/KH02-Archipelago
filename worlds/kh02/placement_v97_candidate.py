"""Additive AP item-placement restrictions; no native or location access changes."""

ZODIAC_IDS = frozenset(range(0x020223, 0x02022f))


def requires_zodiacs(goal, requirement):
    if goal not in ('demon_tide', 'boss_rush'):
        raise ValueError('Unknown selected goal')
    if requirement not in ('wayfinders', 'zodiacs', 'wardrobe_sets'):
        raise ValueError('Unknown selected requirement')
    return goal == 'demon_tide' or requirement == 'zodiacs'


def is_rush_location(row):
    # Objective42 completes after the last round in a Demon Tide seed.
    return row['family'] == 'rush_round' or (row['family'] == 'objective' and row.get('number') == 42)


def install(world, rows, *, goal, requirement):
    """Compose with existing item rules after validating all target locations.

    Only this world's player's Zodiacs are restricted. Foreign-player items
    remain subject to their own reachability and existing placement rules.
    This does not replace AP access rules or prove a complete seed beatable.
    """
    restricted = requires_zodiacs(goal, requirement)
    targets = []
    seen = set()
    for row in rows:
        if not is_rush_location(row):
            continue
        location = world.get_location(row['name'])
        if (location.player != world.player or location.address != row['id']
                or location.address in seen or not callable(location.item_rule)):
            raise ValueError('Rush location identity differs from catalog')
        seen.add(location.address)
        targets.append(location)
    if not restricted:
        return 0
    for location in targets:
        previous = location.item_rule
        player = world.player

        def rule(item, previous=previous, player=player):
            return previous(item) and not (item.player == player and item.code in ZODIAC_IDS)

        location.item_rule = rule
    return len(targets)
