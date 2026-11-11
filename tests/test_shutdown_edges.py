import struct
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch, Mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "payload"))
from client.kh02.shutdown_transport_v191_candidate import (
    ShutdownTransport,
    GameConnectionEnded,
    LevelTransport,
    progress_query_action,
)
from client.kh02.query_overlap_v184_candidate import MickeyTransport, MAGIC


class ShutdownEdges(unittest.TestCase):
    def owner(self):
        o = object.__new__(ShutdownTransport)
        o.pid = 100
        o.thread = 200
        o.window = 300
        o.pending_sequence = None
        o.revoke = Mock()
        return o

    def test_closed_before_query_never_submits(self):
        o = self.owner()
        o._window_identity = lambda: None
        with patch.object(LevelTransport, "_request") as send:
            with self.assertRaises(GameConnectionEnded):
                o._request(3, 7)
            send.assert_not_called()
            o.revoke.assert_called_once()

    def test_closed_during_query(self):
        o = self.owner()
        o._window_identity = Mock(side_effect=[(100, 200), None])
        with patch.object(LevelTransport, "_request", return_value=None) as send:
            with self.assertRaises(GameConnectionEnded):
                o._request(3, 7)
            self.assertEqual(send.call_count, 1)
            o.revoke.assert_called_once()

    def test_missing_reply_with_live_window_stays_failure(self):
        o = self.owner()
        o._window_identity = lambda: (100, 200)
        with patch.object(LevelTransport, "_request", return_value=None):
            self.assertIsNone(o._request(3, 7))
        o.revoke.assert_not_called()

    def test_other_commands_not_reclassified(self):
        o = self.owner()
        o._window_identity = Mock(return_value=None)
        with patch.object(LevelTransport, "_request", return_value=None) as send:
            self.assertIsNone(o._request(11, 7))
            send.assert_called_once()
        o._window_identity.assert_not_called()

    def test_missing_query_preserves_actual_diagnostic_no_retry(self):
        o = object.__new__(MickeyTransport)
        o.pending_sequence = None
        o.last_published = 8
        o.cleanup = SimpleNamespace()
        o.prepared = SimpleNamespace(cookie=12)
        o._request = Mock(return_value=None)
        with self.assertRaisesRegex(RuntimeError, "do not retry"):
            o._send(3)
        self.assertEqual(o._request.call_count, 1)
        self.assertIsNone(o.query_diagnostic["reply"])
        self.assertEqual(o.query_diagnostic["requested_sequence"], 9)


class ProgressShutdownEdges(ShutdownEdges):
    def owner(self, action=11, **overrides):
        o = super().owner()
        o.prepared = SimpleNamespace(cookie=12)
        values = dict(magic=0x3631314F5041, epoch=1, cookie=12, load=1, action=action, reserved=0)
        values.update(overrides)
        o.block = SimpleNamespace(proposal_size=40, proposal=struct.pack("<4Q2I", *values.values()))
        return o

    def test_other_commands_not_reclassified(self):
        # This subclass has a valid action11 packet; action9 is a mutation.
        o = self.owner(9)
        o._window_identity = Mock(return_value=None)
        with patch.object(LevelTransport, "_request", return_value=None) as send:
            self.assertIsNone(o._request(11, 7))
            send.assert_called_once()
        o._window_identity.assert_not_called()

    def test_each_snapshot_before_and_during_close(self):
        for action in (2, 4, 11):
            for identities in ([None], [(100, 200), None]):
                with self.subTest(action=action, identities=identities):
                    o = self.owner(action)
                    o._window_identity = Mock(side_effect=identities)
                    with patch.object(LevelTransport, "_request", return_value=None) as send:
                        with self.assertRaises(GameConnectionEnded):
                            o._request(11, 7)
                        self.assertEqual(send.call_count, len(identities) - 1)
                    o.revoke.assert_called_once()

    def test_malformed_or_mutating_packets_not_classified(self):
        for kwargs in [dict(action=a) for a in (1, 3, 5, 6, 7, 8, 9, 10, 12)] + [
            dict(magic=0),
            dict(epoch=0),
            dict(cookie=13),
            dict(load=0),
            dict(reserved=1),
        ]:
            with self.subTest(kwargs=kwargs):
                self.assertIsNone(progress_query_action(self.owner(**kwargs), 11))
        o = self.owner()
        o.block.proposal_size = 39
        self.assertIsNone(progress_query_action(o, 11))

    def test_acknowledged_bad_reply_and_reused_window_not_hidden(self):
        o = self.owner()
        o._window_identity = Mock(return_value=(100, 200))
        bad = object()
        with patch.object(LevelTransport, "_request", return_value=bad):
            self.assertIs(o._request(11, 7), bad)
        o.revoke.assert_not_called()
        o._window_identity = Mock(side_effect=[(999, 888), None])
        with patch.object(LevelTransport, "_request", return_value=None):
            self.assertIsNone(o._request(11, 8))
        o.revoke.assert_not_called()

    def test_progress_failure_records_action_without_retry(self):
        from client.kh02.progress_overlap_v196_candidate import ProgressTransport

        o = object.__new__(ProgressTransport)
        template = self.owner()
        o.__dict__.update(template.__dict__)
        o.last_published = 8
        o.cleanup = SimpleNamespace()
        o._request = Mock(return_value=None)
        with self.assertRaisesRegex(RuntimeError, "do not retry"):
            o._send(11)
        o._request.assert_called_once_with(11, 9)
        self.assertEqual(o.query_diagnostic["action"], 11)
        self.assertIsNone(o.query_diagnostic["reply"])


if __name__ == "__main__":
    unittest.main()
