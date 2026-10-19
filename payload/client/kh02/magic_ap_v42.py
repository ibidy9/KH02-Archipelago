"""AP-derived magic ownership; experimental mask commands unavailable."""
from .magic_predicate_v41 import MagicPredicateBlock,WindowsMagicPredicateTransport
class WindowsMagicAPTransport(WindowsMagicPredicateTransport):
    allowed_commands=(1,2,3,4,5,6,7)
    async def set_magic_mask(self,mask,epoch):
        raise RuntimeError('Magic ownership is derived from authenticated AP history')
