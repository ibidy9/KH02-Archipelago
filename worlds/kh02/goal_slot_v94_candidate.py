"""Opt-in generation contract; does not register or alter the prototype world.

Keep this file independent of client/native dependencies so it can be packaged
in an apworld. Integration tests compare its output with the client parser.
"""

GAME = 'Kingdom Hearts 0.2 Birth by Sleep'
GOALS = ('demon_tide', 'boss_rush')
REQUIREMENTS = ('wayfinders', 'zodiacs', 'wardrobe_sets')
STARTS = ('Castle Town', 'The World Within', 'Forest of Thorns', 'Depths of Darkness')


def make_slot_data(*, goal, goal_requirement, starting_world, randomized_world_access):
    if type(goal) is not str or goal not in GOALS:
        raise ValueError('Unknown goal')
    if type(goal_requirement) is not str or goal_requirement not in REQUIREMENTS:
        raise ValueError('Unknown goal requirement')
    if type(starting_world) is not str or starting_world not in STARTS:
        raise ValueError('Unknown starting world')
    if type(randomized_world_access) is not bool:
        raise ValueError('Boolean world-access option required')
    return dict(schema_version=2, item_schema_version=5, game=GAME,
                goal=goal, goal_requirement=goal_requirement,
                starting_world=starting_world,
                randomized_world_access=randomized_world_access,
                minimal_clear_content=True, critical_available=True)


def from_options(options):
    """Require explicit goal options; never silently upgrade old world options."""
    index = options.starting_world.value
    if type(index) is not int or not 0 <= index < len(STARTS):
        raise ValueError('Unknown starting-world option')
    return make_slot_data(goal=options.goal.current_key,
                          goal_requirement=options.goal_requirement.current_key,
                          starting_world=STARTS[index],
                          randomized_world_access=bool(options.randomized_world_access))
