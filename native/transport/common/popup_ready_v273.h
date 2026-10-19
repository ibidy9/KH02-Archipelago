/* v207's proven cinematic predicate, shared by display readiness only.
 * No process scan, write, virtual call or new hook. Fail closed on mismatch. */
#ifndef KH02_POPUP_READY_V273_H
#define KH02_POPUP_READY_V273_H
static int popup_ready273(Kh02BoundedRead read,void *p,uint64_t base,uint64_t context) {
    uint64_t vt,fn;uint32_t flags,again;unsigned char code[11];
    static const unsigned char expected[]={0x8b,0x81,0x48,0x03,0,0,0xd1,0xe8,0x24,1,0xc3};
    return read && context && read(p,context,&vt,8) && vt==base+0x36df228 &&
        read(p,vt+0x270,&fn,8) && fn==base+0x44f2f0 &&
        read(p,fn,code,11) && !memcmp(code,expected,11) &&
        read(p,context+0x348,&flags,4) && !(flags&2) &&
        read(p,context+0x348,&again,4) && flags==again;
}
#endif
