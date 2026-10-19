#include "kh02_save_snapshot_candidate.h"
#include <string.h>
static uint32_t u32(const unsigned char *p) {uint32_t n;memcpy(&n,p,4);return n;}
static uint64_t u64(const unsigned char *p) {uint64_t n;memcpy(&n,p,8);return n;}
static uint32_t refuse(Kh02SaveSnapshot *out,uint32_t reason) {
    /* Never expose a partial record as a usable snapshot. Keep only diagnostics. */
    memset(out->record,0,sizeof(out->record));out->address=0;out->status=reason;return reason;
}
uint32_t Kh02SaveSnapshotRead(Kh02BoundedRead read,void *opaque,uint64_t base,
        uint64_t manager,uint32_t index,Kh02SaveSnapshot *out) {
    static const uint32_t fields[8]={100,0x10,0xA0,0x3B40,0x172500,0x1725B0,0,0x10};
    unsigned char header[16],again[48],record[KH02_SAVE_RECORD_SIZE];
    uint64_t buffer,address;unsigned i;
    if(!out)return KH02_SNAPSHOT_BAD_SELECTION;
    memset(out,0,sizeof(*out));out->index=index;
    if(!read || !base || base>0x00007FFFFFF00000ULL || manager!=base+0x4485500 || index>=100)
        return refuse(out,KH02_SNAPSHOT_BAD_SELECTION);
    if(!read(opaque,manager,out->manager,48))return refuse(out,KH02_SNAPSHOT_READ_FAILED);
    for(i=0;i<8;i++)if(u32(out->manager+4*i)!=fields[i])return refuse(out,KH02_SNAPSHOT_BAD_LAYOUT);
    buffer=u64(out->manager+32);
    /* Array length/capacity are not yet live-proven. This strict candidate
       refuses other layouts instead of guessing a safe backing allocation. */
    if(!buffer || buffer>0x0000800000000000ULL-0x1725B0 ||
       u32(out->manager+40)!=0x1725B0 || u32(out->manager+44)<0x1725B0 ||
       u32(out->manager+44)>0x01000000)return refuse(out,KH02_SNAPSHOT_BAD_LAYOUT);
    if(!read(opaque,buffer,header,16))return refuse(out,KH02_SNAPSHOT_READ_FAILED);
    if(memcmp(header,"SAVE",4) || u32(header+4)!=1 || u32(header+8)!=0x1725B0 || u32(header+12))
        return refuse(out,KH02_SNAPSHOT_BAD_LAYOUT);
    address=buffer+0xB0+(uint64_t)index*KH02_SAVE_RECORD_SIZE;
    if(!read(opaque,address,out->record,KH02_SAVE_RECORD_SIZE) ||
       !read(opaque,address,record,KH02_SAVE_RECORD_SIZE) ||
       !read(opaque,manager,again,48))return refuse(out,KH02_SNAPSHOT_READ_FAILED);
    if(memcmp(out->record,record,KH02_SAVE_RECORD_SIZE) || memcmp(out->manager,again,48))
        return refuse(out,KH02_SNAPSHOT_CHANGED);
    out->address=address;out->status=KH02_SNAPSHOT_OK;return out->status;
}
