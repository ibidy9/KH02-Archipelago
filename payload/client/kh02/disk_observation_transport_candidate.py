"""Disk observation transport using the proven identity and retained-exit owner.

No live release is provided. A future packager must pin and seal its preparation,
recovery evidence and accepted profile before issuing the single-use claim.
"""
import ctypes as c
import hashlib
import json
from pathlib import Path
from .route_transport_candidate import WindowsRouteTransport,RoutePolicy
from .pipeline_exit_cleanup_candidate import PipelineExitCleanup
from .disk_observation_candidate import HEADER,MAX_SETUP,ObservationBlock,parse_closed

def checked_inputs(preparation,expected_hash,*,dll,dll_hash,pid,creation_time,window):
    """Check immutable preparation/claim linkage; does not authorize a release."""
    preparation=Path(preparation).resolve(strict=True)
    raw=preparation.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=expected_hash:raise ValueError('Disk preparation seal changed')
    value=json.loads(raw)
    if value.get('schema')!='kh02-disk-observation-input-v1' or value.get('authorizes_delivery') is not False:
        raise ValueError('Unsupported disk observation input')
    payload_path=preparation.parent/'lookup.bin'
    if payload_path.is_symlink() or payload_path.resolve(strict=True).parent!=preparation.parent:
        raise ValueError('Lookup input escapes preparation')
    payload=payload_path.read_bytes()
    if not HEADER.size<=len(payload)<=MAX_SETUP or hashlib.sha256(payload).hexdigest()!=value.get('payload_sha256'):
        raise ValueError('Lookup payload changed')
    cookie=value.get('owner_cookie')
    if type(cookie) is not int or not 0<cookie<2**64 or HEADER.unpack_from(payload)[8]!=cookie:
        raise ValueError('Lookup owner mismatch')
    if Path(value['dll']).resolve(strict=True)!=Path(dll).resolve(strict=True) or value.get('dll_sha256')!=dll_hash.lower():
        raise ValueError('Disk DLL is not the sealed artifact')
    if hashlib.sha256(Path(dll).read_bytes()).hexdigest()!=dll_hash.lower():raise ValueError('Disk DLL changed')
    claim=json.loads((preparation.parent/'launch-attempt.json').read_text())
    expected=dict(schema='kh02-disk-observation-launch-v1',preparation_sha256=expected_hash,
        payload_sha256=value['payload_sha256'],owner_cookie=cookie,pid=pid,creation_time=creation_time,
        window=window,authorizes_delivery=False,outcome='reserved-no-native-result')
    if any(type(claim.get(k)) is not type(v) or claim.get(k)!=v for k,v in expected.items()):
        raise ValueError('Disk observation launch claim mismatch')
    if value.get('profile_policy') is not None:
        from .pipeline_profile_candidate import verify_capture
        verify_capture(preparation,value,claim)
    return value,payload

class WindowsDiskObservationTransport(WindowsRouteTransport):
    block_type=ObservationBlock
    message=0x826f
    mapping_prefix='KH02APDiskObservation'
    enabled_export='Kh02DiskObservationLiveEnabled'
    validate_export='Kh02ValidateDiskObservation'
    hook_export='Kh02DiskObservationHook'

    def __init__(self,dll,*,dll_sha256,preparation,preparation_sha256,pid,creation_time,window,timeout=2.0):
        value,payload=checked_inputs(preparation,preparation_sha256,dll=dll,dll_hash=dll_sha256,
            pid=pid,creation_time=creation_time,window=window)
        self.payload=payload;self.evidence=None
        super().__init__(Path(dll),dll_sha256=dll_sha256,pid=pid,creation_time=creation_time,window=window,
            policy=RoutePolicy(owner_cookie=value['owner_cookie'],generation=1),timeout=timeout)

    def start(self):
        try:super().start()
        finally:
            if self.arm_sent:
                self.cleanup=PipelineExitCleanup(self,cookie=self.policy.owner_cookie,generation=1,last_sequence=self.last_published)

    def _request(self,command,sequence):
        if command==1:
            if self.last_published or self.pending_sequence is not None:
                raise RuntimeError('Cannot replace a sent or pending disk setup')
            self.block.setup_size=len(self.payload);self.block.reserved_setup=0
            c.memmove(c.addressof(self.block.setup),self.payload,len(self.payload))
        return super()._request(command,sequence)

    def release(self):
        if self.arm_sent and not self.game_exited():raise RuntimeError('Retain disk observer until original-process exit')
        if self.block is not None:
            try:self.evidence=parse_closed(ObservationBlock.from_buffer_copy(self.block))
            except ValueError as error:self.evidence=dict(authorizes_delivery=False,complete=False,error=str(error))
        super().release()
