"""Host-side command-6 ABI for the separately built production owner.

The mapped transport integration is intentionally not enabled here.  This
module constructs a fully validated private command block without accepting
caller-provided game addresses or native item codes.
"""
from __future__ import annotations

import ctypes as c

from .production_policy import POLICY_HEADER, PolicyBinding, encode_policy
from .production_schema import PolicySnapshot
from .receipt_set_candidate import ReceiptSet, encode
from .controlled_transport_candidate import ControlledBlock


MEMBERSHIP_CAP = 102456


class NativePolicyWire(c.Structure):
    _fields_ = [
        ("magic", c.c_uint32), ("version", c.c_uint32),
        ("size", c.c_uint32), ("item_schema_version", c.c_uint32),
        ("owner_cookie", c.c_uint64), ("process_cookie", c.c_uint64),
        ("authority", c.c_uint64), ("revision_low", c.c_uint32),
        ("revision_high", c.c_uint32), ("scope_binding", c.c_ubyte * 32),
        ("hp_items", c.c_uint32), ("mp_items", c.c_uint32),
        ("ability_mask", c.c_uint32), ("spell_mask", c.c_uint32),
        ("world_mask", c.c_uint32), ("reserved", c.c_uint32),
    ]


class NativePolicyCommand(c.Structure):
    _fields_ = [
        ("command", c.c_uint32), ("reserved", c.c_uint32),
        ("sequence", c.c_uint64), ("live_authority", c.c_uint64),
        ("generation", c.c_uint64), ("policy", NativePolicyWire),
        ("membership_size", c.c_uint32), ("membership_reserved", c.c_uint32),
        ("membership", c.c_ubyte * MEMBERSHIP_CAP),
    ]


class ProductionControlledBlock(ControlledBlock):
    _fields_ = [
        ("policy_command", NativePolicyCommand),
        ("policy_challenge", c.c_uint64),
        ("policy_scope", c.c_ubyte * 32),
        ("policy_ready", c.c_uint32),
        ("policy_reserved", c.c_uint32),
    ]


def make_policy_command(*, sequence: int, generation: int,
                        binding: PolicyBinding, policy: PolicySnapshot,
                        membership: ReceiptSet) -> NativePolicyCommand:
    if type(sequence) is not int or not 0 < sequence < 2**64:
        raise ValueError("Nonzero command sequence required")
    if type(generation) is not int or not 0 < generation < 2**64:
        raise ValueError("Nonzero native load generation required")
    if membership.owner_cookie != binding.owner_cookie:
        raise ValueError("Receipt membership belongs to another owner")
    if membership.items != policy.items[:len(membership.items)]:
        raise ValueError("Receipt membership is not a policy-history prefix")
    policy_bytes = encode_policy(binding, policy)
    if len(policy_bytes) != c.sizeof(NativePolicyWire) or len(policy_bytes) != POLICY_HEADER.size:
        raise RuntimeError("Python/native policy ABI mismatch")
    receipt_bytes = encode(membership)
    if len(receipt_bytes) > MEMBERSHIP_CAP:
        raise ValueError("Receipt membership exceeds command capacity")
    result = NativePolicyCommand()
    result.command = 6
    result.sequence = sequence
    result.live_authority = binding.authority
    result.generation = generation
    c.memmove(c.addressof(result.policy), policy_bytes, len(policy_bytes))
    result.membership_size = len(receipt_bytes)
    c.memmove(c.addressof(result.membership), receipt_bytes, len(receipt_bytes))
    return result
