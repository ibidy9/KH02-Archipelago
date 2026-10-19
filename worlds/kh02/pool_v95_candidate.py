"""Lossless planning inventory. No world registration or native effects.

Counts describe proposed locations, not proven reachable placements. Unknown
reward quantities and native handlers block release generation explicitly.
"""
from collections import Counter
import json
from importlib.resources import files

ROOT = files(__package__)
WORLDS = ('Castle Town', 'The World Within', 'Forest of Thorns', 'Depths of Darkness')
LEGACY_NAMES = ('Fire Magic', 'Blizzard Magic', 'Thunder Magic', 'Cure Magic',
    'Doubleflight', 'Air Slide', 'Shotlock', 'Spellweaver', 'Potion', 'Ether',
    'Max HP Increase', 'Max MP Increase', 'Hi-Potion', 'Mega-Potion', 'Mega-Ether',
    'Elixir', 'Megalixir') + tuple(w + ' Access' for w in WORLDS)
ITEM_IDS = {name: 0x020201 + i for i, name in enumerate(LEGACY_NAMES)}
ITEM_IDS.update({name: 0x020216 + i for i, name in enumerate(
    ('Cartwheel', 'Barrier', 'Counter Blast', 'Aerial Recovery'))})
ITEM_IDS.update({name: 0x02021d + i for i, name in enumerate(
    ('Progressive Fire', 'Progressive Blizzard', 'Progressive Thunder'))})
WAYFINDERS = ("Terra's Wayfinder", "Aqua's Wayfinder", "Ventus's Wayfinder")
ITEM_IDS.update({name: 0x020220 + i for i, name in enumerate(WAYFINDERS)})
ITEM_IDS.update({name + ' Relic': 0x020223 + i for i, name in enumerate(
    ('Aries', 'Taurus', 'Gemini', 'Cancer', 'Leo', 'Virgo', 'Libra', 'Scorpio',
     'Sagittarius', 'Capricorn', 'Aquarius', 'Pisces'))})
# New draft IDs only; corresponding native map handlers remain unimplemented.
ITEM_IDS.update({world + ' Area Map': 0x02022f + i for i, world in enumerate(WORLDS)})
WARDROBE = json.loads(ROOT.joinpath('data/wardrobe-goal-identities-v78.json').read_text(encoding='utf-8'))['rows']
ITEM_IDS.update({row['name']: row['item_id'] for row in WARDROBE})

MIRRORS = ('Water Cores and Flutterings', 'Flame Core and Shadows', 'Candle Room',
           'Correct Chest Room', 'Mines', 'Labyrinth', 'Endless Stairs')
STORY_BOSSES = (
    ('Castle Town Demon Tower', 'Castle Town'),
    ('Phantom Aqua 1', 'The World Within'),
    ('Phantom Aqua 2', 'The World Within'),
    ('Phantom Aqua 3', 'The World Within'),
    ('Forest First Darkside', 'Forest of Thorns'),
    ('Forest Final Darkside', 'Forest of Thorns'),
    ('Depths Demon Tower', 'Depths of Darkness'),
)
RUSH_ROUNDS = ('Darkside', 'Elemental Heartless', 'Shadows Neoshadows and Flutterings',
               'Demon Tower', 'Zodiac Phantom')


def catalog():
    chest = json.loads(ROOT.joinpath('data/vanilla-chest-rewards.json').read_text(encoding='utf-8'))
    old = [(world, number) for world, count in zip(WORLDS, (5, 8, 10, 5))
           for number in range(1, count + 1)]
    keys = [(world, number) for world, rewards in chest['areas'].items()
            for number in range(1, len(rewards) + 1)]
    ordered = old + [key for key in keys if key not in old]
    rows = []
    for index, (area, number) in enumerate(ordered, 1):
        rows.append(dict(id=0x020200 + index, name=f'{area} Chest {number}',
            family='chest', world='Castle Town' if area == 'Main Road' else area,
            reward=chest['areas'][area][number - 1], native_binding='existing' if index <= 28 else 'pending'))
    objective_names = {}
    for line in ROOT.joinpath('research/objective-catalog.md').read_text(encoding='utf-8').splitlines():
        cells = [x.strip() for x in line.strip('|').split('|')]
        if line.startswith('|') and cells[0].isdigit() and len(cells) >= 6:
            objective_names[int(cells[0])] = cells[1]
    for number in range(1, 51):
        rows.append(dict(id=0x020280 + number,
            name=f'Objective {number} - {objective_names[number]}', family='objective',
            number=number, minimum_level={7: 60, 8: 70, 9: 80}.get(number),
            goals=['demon_tide'] if number == 42 else ['demon_tide', 'boss_rush']))
    rows += [dict(id=0x020400 + level - 51, name=f'Reach Level {level}', family='level',
                  minimum_level=level) for level in range(51, 71)]
    rows += [dict(id=0x020420 + i, name=f'Castle Town Gear {i + 1}', family='gear',
                  world='Castle Town') for i in range(5)]
    rows += [dict(id=0x020430 + i, name=f'Shatter Mirror - {name}', family='mirror',
                  world='The World Within') for i, name in enumerate(MIRRORS)]
    rows += [dict(id=0x020440 + i, name=f'Activate {world} Exit', family='world_end',
                  world=world, trigger='end route activation, including either Depths choice')
             for i, world in enumerate(WORLDS)]
    rows += [dict(id=0x020450 + i, name=f'Defeat {name}', family='story_boss', world=world)
             for i, (name, world) in enumerate(STORY_BOSSES)]
    rows += [dict(id=0x020460 + i, name=f'Boss Rush Round {i + 1} - {name}',
                  family='rush_round', world='The World Within',
                  goals=['demon_tide'] if i == 4 else ['demon_tide', 'boss_rush'])
             for i, name in enumerate(RUSH_ROUNDS)]
    rows += [dict(id=0x020470 + i, name=f'Rocky Path Darkside {i + 1}',
                  family='optional_darkside', world='Forest of Thorns',
                  native_binding='individual defeat identities pending') for i in range(3)]
    if len({r['id'] for r in rows}) != len(rows) or len({r['name'] for r in rows}) != len(rows):
        raise ValueError('Duplicate location identity')
    if len(set(ITEM_IDS.values())) != len(ITEM_IDS):
        raise ValueError('Duplicate item identity')
    return rows


def plan(*, goal, requirement, starting_world, randomized_world_access=True):
    if goal not in ('demon_tide', 'boss_rush') or requirement not in ('wayfinders', 'zodiacs', 'wardrobe_sets'):
        raise ValueError('Explicit goal configuration required')
    if starting_world not in WORLDS or type(randomized_world_access) is not bool:
        raise ValueError('Explicit world configuration required')
    rows = [r for r in catalog() if goal in r.get('goals', ('demon_tide', 'boss_rush'))]
    pool = Counter(r['reward'] for r in rows if r['family'] == 'chest')
    pool.update(row['name'] for row in WARDROBE)
    pool.update(('Cure Magic', 'Doubleflight', 'Air Slide', 'Shotlock', 'Spellweaver',
                 'Cartwheel', 'Barrier', 'Counter Blast', 'Aerial Recovery'))
    pool.update({name: 2 for name in ('Progressive Fire', 'Progressive Blizzard', 'Progressive Thunder')})
    if randomized_world_access:
        pool.update(world + ' Access' for world in WORLDS if world != starting_world)
    if requirement == 'wayfinders': pool.update(WAYFINDERS)
    groups = dict(Counter(row['family'] for row in rows))
    before_bosses = sum(count for family, count in groups.items()
                        if family not in ('story_boss', 'rush_round', 'optional_darkside'))
    # Historical normal-story sequence: 02 03 01 03 01 02. Keep both count
    # interpretations visible; final denominations must preserve actual rewards.
    return dict(goal=goal, requirement=requirement, starting_world=starting_world,
        release_ready=False, location_groups=groups, locations=rows,
        before_boss_locations=before_bosses,
        locations_without_optional_darkside_split=len(rows)-3,
        locations_with_optional_darkside_split=len(rows),
        mandatory_item_counts=dict(sorted(pool.items())),
        mandatory_items_before_stats=pool.total(),
        normal_story_reward_events=6,
        normal_story_units_at_5hp_10mp={'Max HP Increase': 6, 'Max MP Increase': 2},
        provisional_items_with_normal_stat_units=pool.total()+8,
        unknowns=['Native chest quantities (41 reward entries, not all quantity fields verified)',
                  'Zodiac Phantom MP reward denomination and count; observed 60 to 100 is not attribution',
                  'Individual Rocky Path Darkside defeat bindings',
                  'Native check bindings, suppression, and complete generation reachability'],
        world_access_forbidden_locations=[r['id'] for r in rows if (r.get('minimum_level') or 0) > 60])


def require_capacity(item_count, unfilled_locations):
    """Never trim the mandatory pool or count locked/event locations as capacity."""
    if type(item_count) is not int or type(unfilled_locations) is not int or min(item_count, unfilled_locations) < 0:
        raise ValueError('Nonnegative exact counts required')
    if item_count > unfilled_locations:
        raise ValueError(f'Mandatory pool has {item_count} items for {unfilled_locations} locations; add {item_count-unfilled_locations} checks. No items were removed.')
    return unfilled_locations - item_count
