"""Replay startup-log boundary cases without a game or process handle."""

import importlib.util
import json
from pathlib import Path
from tempfile import TemporaryDirectory
from types import SimpleNamespace
import unittest

source = Path(__file__).resolve().parents[1] / "client/ue4ss_readiness.py"
spec = importlib.util.spec_from_file_location("ue4ss_readiness", source)
readiness = importlib.util.module_from_spec(spec)
spec.loader.exec_module(readiness)


class StartupDiagnosticsTests(unittest.TestCase):
    def setUp(self):
        temporary = TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        root = Path(temporary.name)
        self.log = root / "UE4SS.log"
        self.report = root / "startup.json"
        self.calls = []
        self.notes = []
        proof = SimpleNamespace(
            executable=root / "game.exe",
            observe=lambda preparation, **kwargs: self.calls.append(preparation) or "proof",
        )
        self.game = dict(
            pid=71,
            created=readiness.FILETIME_EPOCH + 1,
            path=str(proof.executable),
            modules=[dict(path=str(root / "UE4SS.dll")), dict(path=str(root / "other/main.dll"))],
        )
        self.reader = readiness.ReceiptStartupReader(
            proof, self.log, diagnostic_path=self.report, notify=self.notes.append
        )

    def observe(self):
        return self.reader.observe("scope", games=[self.game])

    def test_timeout_after_large_retry_log(self):
        self.log.write_bytes(
            b"old scan detail\n" * 60000 + b"PS Scan attempt 165\n"
            b"[PS] Failed to find FName::ToString: example\nFatal Error: PS scan timed out\n"
        )
        with self.assertRaisesRegex(RuntimeError, "FName::ToString"):
            self.observe()
        report = json.loads(self.report.read_text())
        self.assertEqual(report["last_attempt"], "165")
        self.assertEqual(report["loaded_modules"]["UE4SS.dll"], "expected")
        self.assertEqual(
            report["loaded_modules"]["Mods/KH02ReceiptHost/dlls/main.dll"], "elsewhere"
        )
        self.assertEqual(self.calls, [])

    def test_stale_error_does_not_stop_new_process(self):
        self.log.write_text("Fatal Error: PS scan timed out")
        self.game["created"] = readiness.FILETIME_EPOCH + self.log.stat().st_mtime_ns // 100 + 10000
        self.assertIsNone(self.observe())
        self.assertEqual(json.loads(self.report.read_text())["state"], "stale_log")
        self.reader.wait_started -= 60
        self.assertIsNone(self.observe())
        self.assertIsNone(self.observe())
        self.assertEqual(len(self.notes), 1)

    def test_only_matching_ready_marker_delegates_to_proof(self):
        def log_for(pid):
            return (
                f"[KH02ReceiptBridge] READY pid={pid} abi=1\n[KH02ReceiptLua] REGISTERED\n".encode()
            )

        self.log.write_bytes(log_for(9))
        self.assertIsNone(self.observe())
        self.log.write_bytes(b"earlier activity\n" * 60000 + log_for(71))
        self.assertEqual(self.observe(), "proof")
        self.assertEqual(self.calls, ["scope"])

    def test_unidentified_timeout_is_reported_without_guess(self):
        self.log.write_text("[PS] Scan failed\nFatal Error: PS scan timed out\n")
        with self.assertRaisesRegex(RuntimeError, "no individual lookup"):
            self.observe()
        self.assertEqual(json.loads(self.report.read_text())["reported_failed_resolvers"], [])


if __name__ == "__main__":
    unittest.main()
