import sys, unittest
from pathlib import Path
from dataclasses import fields

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "payload"))
from client.kh02.level_reporting_v186_candidate import completed, ExitObservation


class GearReporting(unittest.TestCase):
    def checks(self, byte):
        flags = bytearray(8)
        flags[1] = byte
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
        return completed(ExitObservation(**args)) & set(range(0x20420, 0x20425))

    def test_all_256_flag_combinations(self):
        for byte in range(256):
            expected = {0x20421 + i for i in range(4) if byte & (16 << i)}
            if byte & 12:
                expected.add(0x20420)
            self.assertEqual(self.checks(byte), expected)

    def test_automatic_gear_without_savepoint_tutorial(self):
        self.assertEqual(self.checks(4), {0x20420})
        self.assertEqual(self.checks(0), set())

    def test_old_savepoint_completion_still_recognized(self):
        self.assertEqual(self.checks(8), {0x20420})


if __name__ == "__main__":
    unittest.main()
