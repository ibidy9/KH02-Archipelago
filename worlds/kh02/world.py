"""Steam testing generation integration; see package README."""
from BaseClasses import Item, ItemClassification as IC, Location, Region
from worlds.AutoWorld import World

from .starting_loadout_v258 import StartingLoadoutOptions as MickeyOptions, configure as configure_loadout
from .access_v261 import RULES, evaluate, rule_for
from .traps_v244 import TRAP_IDS
from .phantom_placement_v100_candidate import install as install_phantom_rule
from .goal_slot_v94_candidate import from_options
from .pool_v95_candidate import WORLDS, WAYFINDERS, catalog, plan
from .traps_v244 import ITEM_IDS, build
from .pool_v96_candidate import FILLER_WEIGHTS
from .placement_v97_candidate import install
from .wardrobe_logic_v98_candidate import select

GAME = 'Kingdom Hearts 0.2 Birth by Sleep'
ZODIACS = tuple(name for name, code in ITEM_IDS.items() if 0x020223 <= code <= 0x02022e)
ELEMENTS = ('Progressive Fire', 'Progressive Blizzard', 'Progressive Thunder')
MODEL_LIMITATIONS = (
    'Enemy rules describe documented sufficient routes; not all alternate encounter routes are mapped.',
    'Wardrobe logic uses seed-selected progression pieces; native completion accepts any qualifying pieces.',
    'Combat logic assumes selectable difficulty and renewable grinding; it is not a skill guarantee.',
)



class ReviewItem(Item):
    game = GAME


class ReviewLocation(Location):
    game = GAME


class KH02World(World):
    game = GAME
    hidden = False
    options_dataclass = MickeyOptions
    item_name_to_id = dict(ITEM_IDS)
    location_name_to_id = {row['name']: row['id'] for row in catalog()}

    def generate_early(self):
        self.starting_loadout = configure_loadout(self.options)
        if not self.options.randomized_world_access:
            raise ValueError('Only proven independent world access is supported')
        if self.options.starting_world.value == 4:
            inventory = self.options.start_inventory.value
            choices = (0, 1, 2, 3) if any(inventory.get(name, 0) > 0
                for name in ('Cartwheel', 'Barrier')) else (0, 1, 2)
            self.options.starting_world.value = self.random.choice(choices)
        self.configuration = from_options(self.options)
        self.goal = self.configuration['goal']
        self.requirement = self.configuration['goal_requirement']
        self.start = self.configuration['starting_world']
        self.wardrobe = select(self.random, requirement=self.requirement)
        self.rows = plan(goal=self.goal, requirement=self.requirement,
                         starting_world=self.start)['locations']

    def owns(self, state, name, count=1):
        return state.has(name, self.player, count)

    def all_items(self, state, names):
        return all(self.owns(state, name) for name in names)

    def world_open(self, state, name):
        return name == self.start or self.owns(state, name + ' Access')

    def selected_requirement(self, state):
        if self.requirement == 'wardrobe_sets':
            return self.wardrobe.goal_sets(lambda name: self.owns(state, name))
        return self.all_items(state, WAYFINDERS if self.requirement == 'wayfinders' else ZODIACS)

    def rush_requirement(self, state):
        return (self.selected_requirement(state) if self.goal == 'boss_rush'
                else self.all_items(state, ZODIACS))

    def create_regions(self):
        regions = {name: Region(name, self.player, self.multiworld)
                   for name in ('Menu',) + WORLDS + ('WW Mines', 'WW Story Complete', 'Boss Rush', 'Goal')}
        self.multiworld.regions.extend(regions.values())
        for name in WORLDS:
            regions['Menu'].connect(regions[name], rule=lambda s, name=name: self.world_open(s, name))
        regions[WORLDS[1]].connect(regions['WW Mines'], rule=lambda s: self.owns(s, 'Doubleflight'))
        regions['WW Mines'].connect(regions['WW Story Complete'])
        regions[WORLDS[1]].connect(regions['Boss Rush'],
                                  rule=lambda s: self.rush_requirement(s))
        if self.goal == 'boss_rush':
            regions['Boss Rush'].connect(regions['Goal'], rule=lambda s: evaluate(self,s,RULES['goal_boss_rush']))
        else:
            regions[WORLDS[3]].connect(regions['Goal'],
                                       rule=lambda s: evaluate(self,s,RULES['goal_demon_tide']))
        for row in self.rows:
            region = row.get('world', 'Menu')
            if row['family'] == 'rush_round' or row.get('number') == 42:
                region = 'Boss Rush'
            elif row['family'] == 'world_end' and region == WORLDS[1]:
                region = 'WW Story Complete'
            elif row['family'] == 'mirror' and row['name'].endswith(' - Mines'):
                region = 'WW Mines'
            regions[region].locations.append(ReviewLocation(self.player, row['name'], row['id'], regions[region]))
        victory = ReviewLocation(self.player, 'Defeat Selected Final Boss', None, regions['Goal'])
        regions['Goal'].locations.append(victory)
        victory.place_locked_item(ReviewItem('Victory', IC.progression, None, self.player))

    def get_filler_item_name(self):
        return 'Potion'

    def create_item(self, name):
        if name not in ITEM_IDS:
            raise ValueError(f'Unknown item: {name}')
        if name in TRAP_IDS:
            classification = IC.trap
        elif name in self.wardrobe.all_names:
            classification = IC.progression if name in self.wardrobe.progression else IC.useful
        elif name in FILLER_WEIGHTS:
            classification = IC.filler
        elif name == 'Mickey':
            classification = IC.progression  # Required by Mickey-dependent objectives.
        elif name.endswith(' Area Map') or name in ('Max HP Increase', 'Max MP Increase'):
            classification = IC.useful
        else:
            classification = IC.progression
        return ReviewItem(name, classification, ITEM_IDS[name], self.player)

    def create_items(self):
        capacity = len(self.multiworld.get_unfilled_locations(self.player))
        inventory = build(goal=self.goal, requirement=self.requirement, starting_world=self.start,
                          rng=self.random, unfilled_locations=capacity, trap_percentage=self.options.trap_percentage.value,
                          mp_weight=self.options.mp_recharge_trap_weight.value, stun_weight=self.options.stun_trap_weight.value, mickey=self.options.mickey.current_key)
        names = list(inventory.items)
        for name, count in self.starting_loadout.items():
            for _ in range(count):
                names.remove(name)
                names.append(self.get_filler_item_name())
        self.multiworld.itempool.extend(self.create_item(name) for name in names)

    def set_rules(self):
        for row in self.rows:
            location = self.get_location(row['name'])
            self.set_rule(location, rule_for(self, row))
            if (row.get('minimum_level') or 0) > 60:
                previous = location.item_rule
                location.item_rule = lambda item, old=previous: old(item) and not (
                    item.game == GAME and item.name in {w + ' Access' for w in WORLDS})
        install(self, self.rows, goal=self.goal, requirement=self.requirement)
        install_phantom_rule(self, self.rows, allow_important=bool(self.options.zodiac_phantom_important_items))
        # Cure may not be locked behind the final Phantom, independently of the
        # optional important-item switch and the selected goal.
        for row in self.rows:
            if row['name'] == 'Boss Rush Round 5 - Zodiac Phantom' or row.get('number') == 42:
                location = self.get_location(row['name'])
                old = location.item_rule
                location.item_rule = lambda item, old=old: old(item) and not (
                    item.player == self.player and item.name == 'Cure Magic')
        self.set_completion_rule(lambda s: self.owns(s, 'Victory'))

    def fill_slot_data(self):
        from .xp_slot_v185_candidate import make_slot_data
        data = make_slot_data(xp_multiplier=self.options.xp_multiplier.value, mickey=self.options.mickey.current_key,
            goal=self.goal, goal_requirement=self.requirement,
            starting_world=self.start, randomized_world_access=True)

        data.update(schema_version=6,death_link=bool(self.options.death_link.value),item_schema_version=6,traps=dict(percentage=self.options.trap_percentage.value,
            mp_weight=self.options.mp_recharge_trap_weight.value,stun_weight=self.options.stun_trap_weight.value))
        return data
