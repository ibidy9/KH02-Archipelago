"""Per-seed conservative wardrobe logic; actual game still accepts any sets."""
from dataclasses import dataclass
import json
from importlib.resources import files

CATEGORIES = ('head', 'arms', 'back', 'pattern')


@dataclass(frozen=True)
class WardrobeSelection:
    sets_required: bool
    selected: tuple[tuple[str, ...], ...]
    all_names: frozenset[str]

    @property
    def progression(self):
        return frozenset(name for group in self.selected for name in group)

    def any_piece(self, has):
        return any(has(name) for name in self.progression)

    def all_categories(self, has):
        return all(any(has(name) for name in group) for group in self.selected)

    def goal_sets(self, has):
        if not self.sets_required:
            raise ValueError('This seed did not select wardrobe-set goal logic')
        return all(has(name) for name in self.progression)


def select(rng, *, requirement, categories=None):
    if requirement not in ('wayfinders', 'zodiacs', 'wardrobe_sets'):
        raise ValueError('Unknown goal requirement')
    if categories is None:
        path = files(__package__).joinpath('data/wardrobe-goal-sets.json')
        categories = json.loads(path.read_text())['categories']
    if set(categories) != set(CATEGORIES):
        raise ValueError('Exactly four wardrobe categories required')
    names = []
    for category, count in zip(CATEGORIES, (4, 3, 3, 7)):
        groups = categories[category]
        if len(groups) != count or any(len(group) != 3 for group in groups):
            raise ValueError('Wardrobe set structure differs')
        names.extend(name for group in groups for name in group)
    if len(set(names)) != 51 or any(type(name) is not str or not name for name in names):
        raise ValueError('51 distinct wardrobe names required')
    sets_required = requirement == 'wardrobe_sets'
    selected = []
    for category in CATEGORIES:
        groups = categories[category]
        if sets_required:
            selected.append(tuple(rng.choice(groups)))
        else:
            # Objectives21/22 still need wardrobe even under another goal gate.
            selected.append((rng.choice([name for group in groups for name in group]),))
    return WardrobeSelection(sets_required, tuple(selected), frozenset(names))
