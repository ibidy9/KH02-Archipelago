"""Sparse command builder sharing the proven transport's exact ABI types."""
import ctypes as c
from .production_policy_command import NativePolicyCommand,NativePolicyWire,MEMBERSHIP_CAP
from .production_policy import PolicyBinding,POLICY_HEADER,encode_policy
from .production_schema import PolicySnapshot
from .receipt_set_candidate import ReceiptSet,encode

def make_policy_command(*, sequence: int, generation: int,
                        binding: PolicyBinding, policy: PolicySnapshot,
                        membership: ReceiptSet) -> NativePolicyCommand:
    if type(sequence) is not int or not 0 < sequence < 2**64:
        raise ValueError("Nonzero command sequence required")
    if type(generation) is not int or not 0 < generation < 2**64:
        raise ValueError("Nonzero native load generation required")
    if membership.owner_cookie != binding.owner_cookie:
        raise ValueError("Receipt membership belongs to another owner")
    if any(i.index >= len(policy.items) or policy.items[i.index] != i for i in membership.items):
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
