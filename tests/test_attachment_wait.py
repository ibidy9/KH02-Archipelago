"""Exercise the shipped serializer with a virtual clock; no game or Windows hook."""

import sys
import unittest
from pathlib import Path
from types import SimpleNamespace as NS
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "payload"))
from client.kh02.transition_wait_v212_candidate import TransitionWaitTransport
from client.kh02.controlled_transport_candidate import ControlledBlock
from client.kh02.save_transport_host_candidate import WindowsSaveTransport


class AttachmentWait(unittest.TestCase):
    def exercise(self, ack=None, status=1, post=True, exit_at=None):
        owner = object.__new__(TransitionWaitTransport)
        owner.block = ControlledBlock()
        owner.last_published = 0
        owner.pending_sequence = None
        owner.timeout = 2.0
        owner.thread = 7
        owner.pid = 123
        owner.message = 0x8271
        clock = NS(now=0.0)
        posts = []
        owner.user = NS(PostThreadMessageW=lambda *args: posts.append(args) or post)
        owner.game_exited = lambda: exit_at is not None and clock.now >= exit_at

        def sleep(seconds):
            clock.now += seconds
            if ack is not None and clock.now >= ack:
                owner.block.response_sequence = 1
                owner.block.cleanup.status = status

        with patch(
            "client.kh02.save_transport_host_candidate.time.monotonic", lambda: clock.now
        ), patch("client.kh02.save_transport_host_candidate.time.sleep", sleep), patch(
            "client.kh02.save_transport_host_candidate.c.get_last_error", lambda: 5, create=True
        ):
            reply = WindowsSaveTransport._serialized_request(owner, 1, 1)
            if reply is None:
                # An uncertain request must not be posted a second time.
                WindowsSaveTransport._serialized_request(owner, 2, 2)
        self.assertEqual(len(posts), 1)
        return owner, reply

    def test_slow_startup_gets_same_request_ack(self):
        o, r = self.exercise(ack=8)
        self.assertIsNotNone(r)
        self.assertEqual(o.arm_diagnostic["outcome"], "reply_received")
        self.assertGreaterEqual(o.arm_diagnostic["elapsed_seconds"], 8)
        self.assertIsNone(o.pending_sequence)

    def test_no_reply_is_timeout_and_retains_pending(self):
        o, r = self.exercise()
        self.assertIsNone(r)
        self.assertEqual(o.arm_diagnostic["outcome"], "timeout")
        self.assertEqual(o.arm_diagnostic["timeout_seconds"], 30)
        self.assertEqual(o.pending_sequence, 1)

    def test_rejection_is_not_timeout(self):
        o, r = self.exercise(ack=0.1, status=2)
        self.assertEqual(r.status, 2)
        self.assertEqual(o.arm_diagnostic["reply"]["status"], 2)
        self.assertLess(o.arm_diagnostic["elapsed_seconds"], 1)

    def test_post_failure_keeps_windows_error(self):
        o, r = self.exercise(post=False)
        self.assertEqual(o.arm_diagnostic["outcome"], "post_failed")
        self.assertEqual(o.arm_diagnostic["winerror"], 5)

    def test_exit_distinct_from_timeout(self):
        o, r = self.exercise(exit_at=0.2)
        self.assertEqual(o.arm_diagnostic["outcome"], "game_exited")

    def test_other_deadlines_unchanged(self):
        o, _ = self.exercise(ack=0.1)
        self.assertEqual(o._acknowledgement_timeout(3), 30)
        for command in (2, 4, 5):
            self.assertEqual(o._acknowledgement_timeout(command), 2)


if __name__ == "__main__":
    unittest.main()
