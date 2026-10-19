"""Mapped production transport extending the proven controlled commands 1-5."""
from __future__ import annotations

import ctypes as c

from .controlled_transport_candidate import WindowsControlledTransport
from .production_policy import native_scope_binding
from .production_policy_command import NativePolicyCommand,ProductionControlledBlock


class WindowsProductionTransport(WindowsControlledTransport):
    block_type=ProductionControlledBlock
    allowed_commands=(1,2,3,4,5,6)

    def policy_binding_values(self):
        if self.block is None or self.block.policy_ready!=1 or self.block.policy_reserved:
            raise RuntimeError("Native policy owner is not ready")
        expected=native_scope_binding(self.prepared.storage.key)
        actual=bytes(self.block.policy_scope)
        if actual!=expected or not self.block.policy_challenge:
            raise RuntimeError("Native policy binding differs from admitted AP scope")
        return self.block.policy_challenge,actual

    def publish_policy(self,command: NativePolicyCommand) -> int:
        if not isinstance(command,NativePolicyCommand):
            raise ValueError("Typed policy command required")
        challenge,scope=self.policy_binding_values()
        if (command.policy.owner_cookie!=self.prepared.cookie or
            command.policy.process_cookie!=challenge or
            command.live_authority!=self._token or
            bytes(command.policy.scope_binding)!=scope):
            raise ValueError("Policy command is not bound to this native owner")
        c.memmove(c.addressof(self.block.policy_command),c.addressof(command),c.sizeof(command))
        self.block.requested_authority=self._token
        return self._send(6).status
