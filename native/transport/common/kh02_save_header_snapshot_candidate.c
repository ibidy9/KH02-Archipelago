/* Separate bounded snapshot variant; the original record reader stays intact. */
#define Kh02SaveSnapshotRead header_original_snapshot
#include "kh02_save_snapshot_candidate.c"
#undef Kh02SaveSnapshotRead
uint32_t Kh02SaveSnapshotRead(Kh02BoundedRead read,void *opaque,uint64_t base,
    uint64_t manager,uint32_t index,Kh02SaveSnapshot *out) {
    Kh02SaveSnapshot first,second;unsigned char header[0xb0],after[48];uint64_t buffer;
    uint32_t status;
    if(!out)return KH02_SNAPSHOT_BAD_SELECTION;
    memset(out,0,sizeof(*out));out->index=index;
    status=header_original_snapshot(read,opaque,base,manager,index,&first);
    if(status!=KH02_SNAPSHOT_OK)return refuse(out,status);
    buffer=u64(first.manager+32);
    if(!read(opaque,buffer,header,sizeof(header)))return refuse(out,KH02_SNAPSHOT_READ_FAILED);
    status=header_original_snapshot(read,opaque,base,manager,index,&second);
    if(status!=KH02_SNAPSHOT_OK)return refuse(out,status);
    if(memcmp(first.manager,second.manager,48) || memcmp(first.record,second.record,KH02_SAVE_RECORD_SIZE))
        return refuse(out,KH02_SNAPSHOT_CHANGED);
    if(!read(opaque,buffer,second.header,sizeof(second.header)) || !read(opaque,manager,after,48))
        return refuse(out,KH02_SNAPSHOT_READ_FAILED);
    if(memcmp(header,second.header,sizeof(header)) || memcmp(second.manager,after,48))
        return refuse(out,KH02_SNAPSHOT_CHANGED);
    if(memcmp(header,"SAVE",4) || u32(header+4)!=1 || u32(header+8)!=KH02_SAVE_RECORD_SIZE*100+0xb0 || u32(header+12))
        return refuse(out,KH02_SNAPSHOT_BAD_LAYOUT);
    *out=second;return KH02_SNAPSHOT_OK;
}
#ifdef KH02_SAVE_HOOKS_SELFTEST
__declspec(dllexport) uint32_t Kh02SaveHeaderTestSnapshot(Kh02BoundedRead read,void *opaque,uint64_t base,
    uint64_t manager,uint32_t index,Kh02SaveSnapshot *out) {
    return Kh02SaveSnapshotRead(read,opaque,base,manager,index,out);
}
#endif
