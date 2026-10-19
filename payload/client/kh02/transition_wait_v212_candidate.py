"""Same-request transition allowance for validated progress reads.

No resend, response substitution, mutation timeout change or identity relaxation.
"""
import struct
from .transition_wait_v204_candidate import TransitionWaitTransport as Previous

class TransitionWaitTransport(Previous):
    def _acknowledgement_timeout(self, command):
        if command == 11 and self.block.proposal_size == 40:
            magic,epoch,cookie,load,action,reserved=struct.unpack('<4Q2I',bytes(self.block.proposal[:40]))
            if (magic==0x3631314f5041 and epoch>0 and load>0 and
                cookie==self.prepared.cookie and action in (2,4,11) and reserved==0):
                return 30.0
        return super()._acknowledgement_timeout(command)
