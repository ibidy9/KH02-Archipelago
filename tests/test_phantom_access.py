"""Cure is a reachability prerequisite, not just a forbidden boss reward."""
import importlib.util
import itertools
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
RULES = json.loads((ROOT / "worlds/kh02/data/access-rules-v261.json").read_text())
# Load the real evaluator without importing AP's world-registration machinery.
namespace = {}
source = (ROOT / "worlds/kh02/access_v261.py").read_text()
exec(source[source.index("def evaluate"):], {"RULES": RULES}, namespace)
evaluate = namespace["evaluate"]
evaluate.__globals__["evaluate"] = evaluate


class World:
    def __init__(self, owned, unlocked=True):
        self.owned = set(owned)
        self.unlocked = unlocked

    def owns(self, state, name):
        return name in self.owned

    def world_open(self, state, name):
        return self.unlocked

    def rush_requirement(self, state):
        return self.unlocked


class PhantomAccessTests(unittest.TestCase):
    def test_every_missing_kit_combination(self):
        kit = ("Cartwheel", "Barrier", "Cure Magic")
        rows = list(RULES["locations"].values())
        phantom = next(r for r in rows if r["name"] == "Boss Rush Round 5 - Zodiac Phantom")
        objective = next(r for r in rows if r["name"].startswith("Objective 42 -"))
        for present in itertools.product((False, True), repeat=3):
            world = World(k for k, yes in zip(kit, present) if yes)
            with self.subTest(present=present):
                for rule in (phantom["rule"], objective["rule"], RULES["goal_boss_rush"]):
                    self.assertEqual(evaluate(world, None, rule), all(present))
                self.assertEqual(evaluate(world, None, phantom["green"]), all(present))
                self.assertTrue(evaluate(world, None, phantom["yellow"]))
        blocked = World(kit, unlocked=False)
        self.assertFalse(evaluate(blocked, None, phantom["green"]))
        self.assertFalse(evaluate(blocked, None, phantom["yellow"]))

    def test_earlier_rounds_do_not_require_phantom_kit(self):
        for row in RULES["locations"].values():
            if row["name"].startswith(tuple(f"Boss Rush Round {i} -" for i in range(1, 5))):
                self.assertTrue(evaluate(World([]), None, row["rule"]))

    def test_tracker_export_is_current(self):
        spec = importlib.util.spec_from_file_location("export_rules", ROOT / "tools/export_tracker_rules.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module.export(check=True)


if __name__ == "__main__":
    unittest.main()
