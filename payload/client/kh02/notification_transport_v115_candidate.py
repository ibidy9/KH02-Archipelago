"""Display-only development diagnostics, separate from inventory receipts."""
import ctypes as c
import struct
from .area_map_transport_v101_candidate import AreaMapTransport

class NotificationTransport(AreaMapTransport):
    allowed_commands=AreaMapTransport.allowed_commands+(10,)
    async def show_notice(self, *, identity, epoch, text, ap_icon=False):
        if type(ap_icon) is not bool:raise ValueError('Explicit icon choice required')
        raw=text.encode('utf-16-le')
        if type(identity) is not int or not 0<identity<2**63 or type(epoch) is not int or not 0<epoch<2**64:
            raise ValueError('Invalid notification identity/epoch')
        if not 0<len(raw)<=320 or any(ord(ch)<32 or ch in '<>&' for ch in text):
            raise ValueError('Invalid notification text')
        packet=struct.pack('<QQQII',0x3134324e5041 if ap_icon else 0x3531314e5041,identity,epoch,len(raw)//2,0)+raw+bytes(328-len(raw))
        assert len(packet)==360
        async with self._async_lock:
            if not await self._sync():raise RuntimeError('Current AP authority required')
            self.block.requested_authority=self._token
            self.block.proposal_size=len(packet)
            c.memmove(c.addressof(self.block.proposal),packet,len(packet))
            reply=await self._offload(self._send,10)
            if reply.status!=1:raise RuntimeError('Notification request refused; do not retry blindly')


def read_custom(pid):
    if type(pid) is not int or pid<=0:raise ValueError('Invalid PID')
    k=c.WinDLL('kernel32',use_last_error=True)
    k.OpenFileMappingW.argtypes=[c.c_uint32,c.c_int,c.c_wchar_p];k.OpenFileMappingW.restype=c.c_void_p
    k.MapViewOfFile.argtypes=[c.c_void_p,c.c_uint32,c.c_uint32,c.c_uint32,c.c_size_t];k.MapViewOfFile.restype=c.c_void_p
    k.UnmapViewOfFile.argtypes=[c.c_void_p];k.CloseHandle.argtypes=[c.c_void_p]
    h=k.OpenFileMappingW(4,False,f'Local\\KH02APCustomPopupV115-{pid}')
    if not h:raise c.WinError(c.get_last_error())
    address=None
    try:
        address=k.MapViewOfFile(h,4,0,0,608)
        if not address:raise c.WinError(c.get_last_error())
        raw=c.string_at(address,608)
        if raw!=c.string_at(address,608):raise RuntimeError('UI diagnostic changing')
        v=dict(zip(('magic','sequence','epoch','identity','pending','status','submitted','icon_length'),struct.unpack('<8Q',raw[:64])))
        if v['magic']!=0x323131504343 or v['sequence']&1 or v['icon_length']>260:raise ValueError('Invalid UI diagnostic')
        v['icon']=raw[64:64+2*v['icon_length']].decode('utf-16-le')
        return v
    finally:
        if address:k.UnmapViewOfFile(address)
        k.CloseHandle(h)
