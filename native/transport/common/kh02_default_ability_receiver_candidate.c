#include "kh02_default_ability_receiver_candidate.h"
#include <string.h>

static int ptr(Kh02BoundedRead read,void *context,uint64_t address,uint64_t *out) {
    return address>=0x10000 && address<=0x7fffffffffffULL-8 &&
        read(context,address,out,8)==1;
}

uint32_t Kh02DefaultAbilityInspect(Kh02BoundedRead read,void *context,
        uint64_t base,uint64_t receiver,uint32_t kind,
        Kh02DefaultAbilityReceiver *out) {
    Kh02DefaultAbilityReceiver candidate={0};uint64_t again;
    if(!out)return KH02_GATE_REFUSED;
    memset(out,0,sizeof(*out));
    if(!read || base<0x10000 || base>0x7fffffffffffULL-0x04f5f000 ||
       receiver<0x10000 || receiver>0x7fffffffffffULL-0x1000 ||
       (receiver&7) || (kind!=1 && kind!=2 && kind!=4 && kind!=28) ||
       !ptr(read,context,receiver,&candidate.vtable) ||
       candidate.vtable<base+0x3599000 ||
       candidate.vtable>base+0x4484000-0x1260 ||
       candidate.vtable==base+0x36df228 ||
       !ptr(read,context,candidate.vtable+0x1230,&candidate.query) ||
       !ptr(read,context,candidate.vtable+0x1248,&candidate.add) ||
       !ptr(read,context,candidate.vtable+0x1258,&candidate.remove) ||
       candidate.query<base+0x1000 || candidate.query>=base+0x3598000 ||
       candidate.add<base+0x1000 || candidate.add>=base+0x3598000 ||
       candidate.remove<base+0x1000 || candidate.remove>=base+0x3598000 ||
       candidate.query==base+0x25c5c0 || candidate.query==base+0x23ce40 ||
       candidate.add==base+0x23ce40 || candidate.remove==base+0x23ce40 ||
       !ptr(read,context,receiver,&again) || again!=candidate.vtable)
        return KH02_GATE_REFUSED;
    candidate.receiver=receiver;candidate.kind=kind;
    *out=candidate;return KH02_GATE_OK;
}
