"""Defer only a fully acknowledged read-only DeathLink query on hook overlap.

Missing/mismatched acknowledgements remain fatal. This never sends an effect,
retries a request, resets pending sequence, or weakens cleanup checks.
"""
import struct
from dataclasses import asdict
from .route_transport_candidate import MAGIC

def send_query(owner):
    if owner.block.proposal_size!=32:raise ValueError('Death query size differs')
    magic,identity,epoch,kind,reserved=struct.unpack('<QQQII',bytes(owner.block.proposal[:32]))
    if magic!=0x393432444c5041 or identity or not epoch or kind or reserved:
        raise ValueError('Only read-only DeathLink query allowed')
    if owner.pending_sequence is not None:raise RuntimeError('Previous native response is uncertain')
    sequence=owner.last_published+1;owner.cleanup.sequence=sequence
    reply=owner._request(15,sequence)
    details=dict(command=15,kind=0,sequence=sequence,reply=asdict(reply) if reply is not None else None)
    owner.death_query_diagnostic=details
    def fail():raise RuntimeError('DeathLink read-only acknowledgement failed; do not retry: '+repr(details))
    if reply is None:fail()
    if (reply.magic,reply.version,reply.size,reply.sequence,reply.owner_cookie,reply.generation,reply.entry_restored)!=(MAGIC,1,48,sequence,owner.prepared.cookie,1,0):fail()
    if reply.status not in (1,2) or reply.active!=1 or reply.in_flight<0:fail()
    if reply.in_flight:
        owner.death_query_overlaps=getattr(owner,'death_query_overlaps',0)+1
        return None
    return reply
