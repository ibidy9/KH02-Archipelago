"""Exercise startup recovery using only disposable files and game observations."""

import asyncio
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "payload"))
import client.kh02

client.kh02.__path__.insert(0, str(ROOT / "integrations/ue4ss/client"))
from client.kh02 import steam_recovery as recovery
from client.kh02.steam_installation import Installation
from client.kh02.steam_coordinator_v187_candidate import serve_prepared
from client.kh02.stable_runtime_v245 import RuntimeLease


class RecoveryTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.profiles = self.root / "profiles"
        self.exe = self.root / "game.exe"
        self.exe.write_bytes(b"disposable executable")
        digest = hashlib.sha256(self.exe.read_bytes()).digest()
        mocked = patch.object(recovery, "STEAM_BUILD", digest.hex())
        mocked.start()
        self.addCleanup(mocked.stop)
        clock = patch.object(recovery, "filetime", return_value=50)
        clock.start()
        self.addCleanup(clock.stop)
        binding = bytes(range(32))
        nonce = bytes(range(32, 64))
        profile = self.profiles / ("previous-P1-" + binding.hex()[:16])
        self.session = profile / "sessions" / "previous-launch"
        self.session.mkdir(parents=True)
        (profile / "claims").mkdir()
        self.claim = profile / "claims" / (nonce.hex() + ".claim")
        self.claim.write_bytes((25).to_bytes(8, "little"))
        (profile / "profile.json").write_text(json.dumps(dict(binding=binding.hex())))
        (self.session / "launch.json").write_text(
            json.dumps(dict(executable=str(self.exe), profile=str(profile), binding=binding.hex()))
        )
        ready = bytearray(1104)
        struct.pack_into("<8sIIQQ", ready, 0, b"KH02AP01", 1, 1104, 10, 100)
        ready[32:128] = digest + binding + nonce
        resume = bytearray(144)
        struct.pack_into("<8sII", resume, 0, b"KH02RS01", 1, 144)
        resume[16:80] = binding + nonce
        self.ready = self.root / "kh02-ap-ready.bin"
        self.resume = self.root / "kh02-ap-resume.bin"
        self.transaction = Installation(
            self.session / "installation",
            require_closed=lambda: None,
            allowed_targets=(self.ready, self.resume),
        )
        self.transaction.install([(self.ready, bytes(ready)), (self.resume, bytes(resume))])
        (self.session / "prepared-ready.bin").write_bytes(ready)
        self.save = profile / "saves" / "KH0.2BbS_WW.png"
        self.save.parent.mkdir()
        self.save.write_bytes(b"save sentinel")

    def recover(self):
        return recovery.recover_interrupted(self.exe, self.profiles, require_closed=lambda: None)

    def test_spent_and_partial_cleanup(self):
        self.ready.unlink()
        self.assertTrue(self.recover())
        self.assertFalse(self.resume.exists())
        self.assertFalse(self.recover())
        self.assertEqual(self.save.read_bytes(), b"save sentinel")

    def test_unclaimed_until_expired(self):
        self.claim.unlink()
        with self.assertRaisesRegex(ValueError, "still reserved"):
            self.recover()
        self.assertTrue(self.ready.exists())
        self.assertTrue(self.resume.exists())
        with patch.object(recovery, "filetime", return_value=101):
            self.assertTrue(self.recover())

    def test_changed_resume_preserves_both(self):
        data = bytearray(self.resume.read_bytes())
        data[98] ^= 1
        self.resume.write_bytes(data)
        with self.assertRaisesRegex(ValueError, "differs"):
            self.recover()
        self.assertTrue(self.ready.exists())
        self.assertEqual(self.resume.read_bytes(), data)

    def test_abandoned_exclusive_preparation_can_recover_immediately(self):
        self.claim.unlink()
        path = self.session / "launch.json"
        launch = json.loads(path.read_text())
        launch["exclusive_preparation"] = True
        path.write_text(json.dumps(launch))
        self.assertTrue(self.recover())
        self.assertFalse(self.ready.exists())
        self.assertFalse(self.resume.exists())

    def test_extra_manifest_file_is_never_removed(self):
        manifest = json.loads(self.transaction.manifest.read_text())
        manifest["files"].append({**manifest["files"][0], "path": str(self.save)})
        self.transaction.manifest.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, "outside"):
            self.recover()
        self.assertTrue(self.ready.exists())
        self.assertTrue(self.resume.exists())
        self.assertEqual(self.save.read_bytes(), b"save sentinel")

    def test_concurrent_owner_cannot_enter_recovery(self):
        def guarded():
            with RuntimeLease(require_closed=lambda: None, root=self.root / "runtime"):
                self.recover()

        with RuntimeLease(require_closed=lambda: None, root=self.root / "runtime"):
            with self.assertRaisesRegex(RuntimeError, "lease"):
                guarded()
        self.assertTrue(self.ready.exists())
        self.assertTrue(self.resume.exists())

    def test_exit_before_attachment_cleans_up(self):
        import subprocess

        subprocess_timeout = subprocess.TimeoutExpired("inventory", 1)
        samples = iter(
            [
                (),
                ({"pid": 7, "created": 2, "path": str(self.exe)},),
                ({"pid": 7, "created": 2, "path": None},),
                subprocess_timeout,
                (),
                (),
            ]
        )

        def inventory():
            sample = next(samples)
            if isinstance(sample, Exception):
                raise sample
            return sample

        messages = []
        asyncio.run(
            serve_prepared(
                SimpleNamespace(context=SimpleNamespace()),
                SimpleNamespace(storage=SimpleNamespace(expires=100)),
                reader=SimpleNamespace(
                    observe=lambda preparation, games: (
                        Path(games[0]["path"]) and None if games else None
                    )
                ),
                installation=self.transaction,
                transport_dll=None,
                transport_sha256=None,
                session_directory=self.session,
                journal_root=None,
                running_games=inventory,
                stop=asyncio.Event(),
                clock=lambda: 20,
                interval=0.05,
                notify=messages.append,
            )
        )
        self.assertIn("0.2 closed before AP attached. Cleaning up this launch.", messages)
        self.assertFalse(self.ready.exists())
        self.assertFalse(self.resume.exists())


if __name__ == "__main__":
    unittest.main()
