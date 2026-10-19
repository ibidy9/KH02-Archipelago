"""AP-owned four-action transport; arbitrary development mask commands disabled."""
from .default_gates_v36 import ActionGateBlock, WindowsActionGateTransport


class WindowsActionOwnershipTransport(WindowsActionGateTransport):
    allowed_commands = (1, 2, 3, 4, 5, 6, 7)

    async def set_action_gates(self, mask, epoch):
        raise RuntimeError('AP action ownership is derived from authenticated received history')
