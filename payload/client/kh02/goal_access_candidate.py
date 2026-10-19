"""Effect-free access rules. Access is never victory evidence.

Inputs are canonical names from authenticated AP ownership, NOT native clear
flags, wardrobe visibility or arbitrary inventory reads. Native ID binding and
completion events are separate integration work.
"""
from dataclasses import dataclass
from enum import Enum
import json
from pathlib import Path


class Goal(Enum):
    DEMON_TIDE='demon_tide'
    BOSS_RUSH='boss_rush'


class TideRequirement(Enum):
    WAYFINDERS='wayfinders'
    ZODIACS='zodiacs'
    WARDROBE='wardrobe_sets'


WAYFINDERS=frozenset(("Terra's Wayfinder","Aqua's Wayfinder","Ventus's Wayfinder"))
ZODIACS=frozenset(name+' Relic' for name in ('Aries','Taurus','Gemini','Cancer',
    'Leo','Virgo','Libra','Scorpio','Sagittarius','Capricorn','Aquarius','Pisces'))
CATEGORIES=('head','arms','back','pattern')
WORLDS=frozenset(('Castle Town','The World Within','Forest of Thorns','Depths of Darkness'))


def load_sets(path=None):
    path=path or Path(__file__).resolve().parents[2]/'data/wardrobe-goal-sets.json'
    data=json.loads(Path(path).read_text())
    if data.get('schema')!='kh02-wardrobe-goal-sets-v1' or data.get('requires_equipped') is not False:
        raise ValueError('Unrecognized wardrobe-set contract')
    raw=data['categories']
    if set(raw)!=set(CATEGORIES):raise ValueError('All four wardrobe categories required')
    result={};seen=set()
    for category,count in zip(CATEGORIES,(4,3,3,7)):
        groups=raw[category]
        if len(groups)!=count:raise ValueError('Unexpected wardrobe-set count')
        result[category]=[]
        for group in groups:
            if len(group)!=3 or any(type(n) is not str or not n for n in group):
                raise ValueError('Three named pieces required')
            if len(set(group))!=3 or seen.intersection(group):raise ValueError('Repeated wardrobe piece')
            seen.update(group);result[category].append(frozenset(group))
        result[category]=tuple(result[category])
    if len(seen)!=51:raise ValueError('All 51 wardrobe pieces required')
    return result


@dataclass(frozen=True)
class Access:
    demon_tide: bool
    boss_rush: bool


def access(owned_names, *, starting_world, selected_goal, tide_requirement, sets=None):
    if starting_world not in WORLDS or not isinstance(selected_goal,Goal) or not isinstance(tide_requirement,TideRequirement):
        raise ValueError('Explicit supported world/goal/requirement required')
    names=tuple(owned_names)
    if any(type(n) is not str or not n for n in names):raise ValueError('Canonical item names required')
    owned=frozenset(names)
    world=lambda name:starting_world==name or name+' Access' in owned
    zodiacs=ZODIACS<=owned
    if tide_requirement is TideRequirement.WAYFINDERS:ready=WAYFINDERS<=owned
    elif tide_requirement is TideRequirement.ZODIACS:ready=zodiacs
    else:
        groups=sets if sets is not None else load_sets()
        if set(groups)!=set(CATEGORIES):raise ValueError('Incomplete wardrobe categories')
        ready=all(any(group<=owned for group in groups[category]) for category in CATEGORIES)
    # Boss rush remains available under either selected goal. Tide is the final
    # route only for its selected goal; a rush seed must not accidentally end it.
    return Access(selected_goal is Goal.DEMON_TIDE and world('Depths of Darkness') and ready,
                  world('The World Within') and zodiacs)
