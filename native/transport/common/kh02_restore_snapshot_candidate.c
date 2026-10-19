#include "kh02_restore_snapshot_candidate.h"
#include <string.h>
static uint32_t refuse(Kh02RestoreSnapshot *out,uint32_t status) {
    memset(out,0,sizeof(*out));out->status=status;return status;
}
uint32_t Kh02RestoreSnapshotRead(Kh02BoundedRead read,void *opaque,uint64_t base,
        uint64_t context,Kh02RestoreSnapshot *out) {
    uint64_t table,again,methods[2],methods_again[2];
    unsigned char primary[KH02_RESTORE_INVENTORY_SIZE],checkpoint[KH02_RESTORE_INVENTORY_SIZE];
    if(!out)return KH02_RESTORE_BAD_CONTEXT;
    memset(out,0,sizeof(*out));
    if(!read || !base || base>0x0000800000000000ULL-0x04f5f000 ||
       !context || (context&7) || context>0x0000800000000000ULL-0x65d4)
        return refuse(out,KH02_RESTORE_BAD_CONTEXT);
    if(!read(opaque,context,&table,8))return refuse(out,KH02_RESTORE_READ_FAILED);
    if(table!=base+0x36df228)return refuse(out,KH02_RESTORE_BAD_CONTEXT);
    if(!read(opaque,table+0x390,methods,16))return refuse(out,KH02_RESTORE_READ_FAILED);
    if(methods[0]!=base+0x447b20 || methods[1]!=base+0x453d60)
        return refuse(out,KH02_RESTORE_BAD_CONTEXT);
    if(!read(opaque,context+0x102c,out->primary,sizeof(out->primary)) ||
       !read(opaque,context+0x4d54,out->checkpoint,sizeof(out->checkpoint)) ||
       !read(opaque,context+0x102c,primary,sizeof(primary)) ||
       !read(opaque,context+0x4d54,checkpoint,sizeof(checkpoint)) ||
       !read(opaque,table+0x390,methods_again,16) ||
       !read(opaque,context,&again,8))return refuse(out,KH02_RESTORE_READ_FAILED);
    if(table!=again || memcmp(methods,methods_again,16) ||
       memcmp(primary,out->primary,sizeof(primary)) ||
       memcmp(checkpoint,out->checkpoint,sizeof(checkpoint)))
        return refuse(out,KH02_RESTORE_CHANGED);
    out->context=context;out->status=KH02_RESTORE_OK;return out->status;
}
