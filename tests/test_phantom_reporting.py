import sys, unittest
from pathlib import Path
from dataclasses import fields

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "payload"))
from client.kh02.level_reporting_v186_candidate import completed, ExitObservation


class PhantomReporting(unittest.TestCase):
    def test_routes_independent_of_mines_visit(self):
        for routes in range(8):
            for mines in (0, 1):
                for bonus in (0, 8):
                    flags = bytearray(8)
                    flags[2] = routes << 3
                    flags[5] = mines | bonus
                    args = {f.name: 0 for f in fields(ExitObservation)}
                    args.update(
                        cookie=1,
                        epoch=1,
                        load_id=1,
                        owner=1,
                        header=bytes(64),
                        switches=bytes(flags),
                        live_level=50,
                    )
                    result = completed(ExitObservation(**args))
                    count = routes.bit_count()
                    self.assertEqual(0x20451 in result, count >= 1)
                    self.assertEqual(0x20452 in result, count >= 2)
                    self.assertEqual(0x20453 in result, count == 3 and bool(bonus))

    def test_temporary_route_selection_does_not_award_boss(self):
        for selected in range(8):
            flags = bytearray(8)
            flags[2] = selected
            args = {f.name: 0 for f in fields(ExitObservation)}
            args.update(
                cookie=1,
                epoch=1,
                load_id=1,
                owner=1,
                header=bytes(64),
                switches=bytes(flags),
                live_level=50,
            )
            self.assertFalse(completed(ExitObservation(**args)) & {0x20451, 0x20452, 0x20453})


if __name__ == "__main__":
    unittest.main()
