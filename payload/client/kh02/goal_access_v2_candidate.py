"""Separate candidate for selectable requirements on either goal.

Pure policy only: native Boss Rush mirror override remains live-unproven.
Non-goal Boss Rush retains its ordinary Zodiac access in Demon Tide seeds.
"""
from .goal_access_candidate import (Access,Goal,TideRequirement,WAYFINDERS,ZODIACS,
                                    WORLDS,CATEGORIES,load_sets)

def access(owned_names, *, starting_world, selected_goal, requirement):
    if starting_world not in WORLDS or not isinstance(selected_goal,Goal) or not isinstance(requirement,TideRequirement):
        raise ValueError('Explicit supported world, goal and requirement required')
    names=tuple(owned_names)
    if any(type(n) is not str or not n for n in names):raise ValueError('Canonical item names required')
    owned=frozenset(names)
    if requirement is TideRequirement.WAYFINDERS:ready=WAYFINDERS<=owned
    elif requirement is TideRequirement.ZODIACS:ready=ZODIACS<=owned
    else:ready=all(any(group<=owned for group in groups) for groups in load_sets().values())
    world=lambda name:starting_world==name or name+' Access' in owned
    return Access(
        selected_goal is Goal.DEMON_TIDE and world('Depths of Darkness') and ready,
        world('The World Within') and (ready if selected_goal is Goal.BOSS_RUSH else ZODIACS<=owned))
