/* Reuse the proven inventory anchors and bit operation, active record only.
 * The live gear leaves the reference record unchanged. No other ability bit
 * is controlled here; this operation can only enable the shared meter. */
#include "kh02_meter_projection_v48.h"
#include <string.h>
uint32_t Kh02MeterProject(Kh02BoundedRead read,Kh02MeterWrite write,void *context,
        uint64_t world,uint64_t expected_owner,Kh02MeterProjection *out) {
    Kh02InventorySnapshot inventory;uint64_t location,again;
    unsigned char before,after,reference,observed;
    if(!out)return KH02_GATE_REFUSED;
    memset(out,0,sizeof(*out));
    if(!read || !write || !expected_owner ||
       Kh02ReadInventory(read,context,world,&inventory)!=KH02_GATE_OK ||
       inventory.owner!=expected_owner || inventory.primary==inventory.reference ||
       inventory.primary<0x10f8c || inventory.reference<0x10f8c)return KH02_GATE_REFUSED;
    location=inventory.primary-0xf8c;
    if(!read(context,location,&before,1) ||
       !read(context,inventory.reference-0xf8c,&reference,1) ||
       (before&0xe0) || (reference&0xe0) ||
       !read(context,world+0x158,&again,8) || again!=expected_owner)return KH02_GATE_REFUSED;
    /* Bit4 is independent magic/story availability, not an owner validity test.
     * Native setter4ef1d0 ORs bit8 independently. Fresh checkpoints can have0;
     * preserve every other bit and never write the reference. */
    after=before|8;out->before=before;out->after=after;out->reference=reference;
    if(after==before)return KH02_GATE_OK;
    if(!read(context,location,&observed,1) || observed!=before)return KH02_GATE_REFUSED;
    int written=write(context,location,&after,1);
    if(!read(context,location,&observed,1))return KH02_GATE_UNCERTAIN;
    if(!written && observed==before)return KH02_GATE_REFUSED;
    if(observed!=after)return KH02_GATE_UNCERTAIN;
    if(!read(context,world+0x158,&again,8) || again!=expected_owner ||
       !read(context,inventory.reference-0xf8c,&observed,1) || observed!=reference)
        return KH02_GATE_UNCERTAIN;
    out->changed=1;return KH02_GATE_OK;
}
