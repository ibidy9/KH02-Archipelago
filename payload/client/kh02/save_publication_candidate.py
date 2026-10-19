"""Offline ordering model, not a native hook or durable storage implementation."""
from dataclasses import dataclass, replace
from hashlib import sha256


@dataclass(frozen=True)
class Publication:
    token: str
    container: bytes
    association: bytes  # Future canonical payload: scope, exact records, receipts.
    phase: str = 'prepared'

    @property
    def binding(self) -> str:
        # Length framing prevents ambiguous concatenation; includes transaction.
        token = self.token.encode('utf-8')
        parts = (token, self.container, self.association)
        return sha256(b''.join(len(p).to_bytes(8, 'little') + p for p in parts)).hexdigest()


def prepare(token: str, container: bytes, association: bytes, *,
            effects_drained: bool, membership_frozen: bool) -> Publication:
    if (not token or type(container) is not bytes or len(container) != 0x1725b0
            or type(association) is not bytes or not association
            or not effects_drained or not membership_frozen):
        raise ValueError('Unresolved effects or invalid frozen publication inputs')
    return Publication(token, container, association)


def acknowledge_durable(publication: Publication, token: str, binding: str) -> Publication:
    """Accept hypothetical verified flush evidence; does not perform a flush."""
    if (publication.phase != 'prepared' or token != publication.token
            or binding != publication.binding):
        raise ValueError('Stale, conflicting, or duplicate durability acknowledgment')
    return replace(publication, phase='durable')


def begin_native_save(publication: Publication) -> Publication:
    if publication.phase != 'durable':
        raise ValueError('Original save cannot run before exact association durability')
    return replace(publication, phase='native-started')


def finish_native_save(publication: Publication, success: bool) -> Publication:
    if publication.phase != 'native-started' or type(success) is not bool:
        raise ValueError('No matching native operation or definitive outcome')
    return replace(publication, phase='saved' if success else 'native-failed')


def fail_association(publication: Publication) -> Publication:
    if publication.phase != 'prepared':
        raise ValueError('Cannot retroactively cancel a durable/native publication')
    return replace(publication, phase='association-failed')
