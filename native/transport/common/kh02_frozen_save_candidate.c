#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "kh02_frozen_save_candidate.h"
#if (defined(KH02_PLATFORM_LIVE) || defined(KH02_FROZEN_PLATFORM_LIVE)) && defined(KH02_FREEZE_SELFTEST)
#error Live builds must not expose allocation fault injection
#endif
#ifdef KH02_FREEZE_SELFTEST
static volatile LONG freeze_fault,freeze_allocations;
__declspec(dllexport) void Kh02FreezeTestFault(uint32_t fault){InterlockedExchange(&freeze_fault,(LONG)fault);}
__declspec(dllexport) uint32_t Kh02FreezeTestAllocations(void){return (uint32_t)InterlockedCompareExchange(&freeze_allocations,0,0);}
#endif
static void *allocate_buffer(void) {
#ifdef KH02_FREEZE_SELFTEST
    if(InterlockedCompareExchange(&freeze_fault,0,0)==1)return NULL;
#endif
    void *p=VirtualAlloc(NULL,KH02_FULL_SAVE_SIZE,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
#ifdef KH02_FREEZE_SELFTEST
    if(p)InterlockedIncrement(&freeze_allocations);
#endif
    return p;
}
static int release_buffer(void *p) {
    if(!VirtualFree(p,0,MEM_RELEASE))return 0;
#ifdef KH02_FREEZE_SELFTEST
    InterlockedDecrement(&freeze_allocations);
#endif
    return 1;
}
static int protect_buffer(void *p,DWORD *old) {
#ifdef KH02_FREEZE_SELFTEST
    if(InterlockedCompareExchange(&freeze_fault,0,0)==2)return 0;
#endif
    return VirtualProtect(p,KH02_FULL_SAVE_SIZE,PAGE_READONLY,old)!=0;
}
static uint32_t u32(const unsigned char *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint64_t u64(const unsigned char *p){uint64_t n;memcpy(&n,p,8);return n;}
_Static_assert(sizeof(Kh02FrozenSave)==16,"Native array descriptor ABI");
uint32_t Kh02ReleaseFrozenSave(Kh02FrozenSave *out) {
    if(!out)return KH02_FREEZE_BAD_INPUT;
    if(out->data && !release_buffer((void*)out->data))return KH02_FREEZE_BAD_INPUT;
    memset(out,0,sizeof(*out));return KH02_FREEZE_OK;
}
uint32_t Kh02FreezeSave(Kh02BoundedRead read,void *opaque,uint64_t base,uint64_t descriptor,Kh02FrozenSave *out) {
    static const uint32_t expected[8]={100,0x10,0xa0,0x3b40,0x172500,0x1725b0,0,0x10};
    unsigned char manager[48],again[48],chunk[0x10000],*owned;uint64_t source;
    uint32_t i,n,status=KH02_FREEZE_OK;DWORD protection;
    if(!out || out->data || out->length || out->capacity || !read || !base ||
       base>0x0000800000000000ULL-0x04f5f000 || descriptor!=base+0x4485520)return KH02_FREEZE_BAD_INPUT;
    if(!read(opaque,base+0x4485500,manager,48))return KH02_FREEZE_READ_FAILED;
    for(i=0;i<8;i++)if(u32(manager+4*i)!=expected[i])return KH02_FREEZE_BAD_INPUT;
    source=u64(manager+32);
    if(!source || source>0x0000800000000000ULL-KH02_FULL_SAVE_SIZE ||
       u32(manager+40)!=KH02_FULL_SAVE_SIZE || u32(manager+44)<KH02_FULL_SAVE_SIZE ||
       u32(manager+44)>0x1000000)return KH02_FREEZE_BAD_INPUT;
    owned=allocate_buffer();
    if(!owned)return KH02_FREEZE_NO_MEMORY;
    if(!read(opaque,source,owned,KH02_FULL_SAVE_SIZE)){status=KH02_FREEZE_READ_FAILED;goto fail;}
    if(memcmp(owned,"SAVE",4) || u32(owned+4)!=1 || u32(owned+8)!=KH02_FULL_SAVE_SIZE || u32(owned+12)) {
        status=KH02_FREEZE_BAD_INPUT;goto fail;
    }
    for(i=0;i<KH02_FULL_SAVE_SIZE;i+=n) {
        n=KH02_FULL_SAVE_SIZE-i;if(n>sizeof(chunk))n=sizeof(chunk);
        if(!read(opaque,source+i,chunk,n)){status=KH02_FREEZE_READ_FAILED;goto fail;}
        if(memcmp(owned+i,chunk,n)){status=KH02_FREEZE_CHANGED;goto fail;}
    }
    if(!read(opaque,base+0x4485500,again,48)){status=KH02_FREEZE_READ_FAILED;goto fail;}
    if(memcmp(manager,again,48)){status=KH02_FREEZE_CHANGED;goto fail;}
    if(!protect_buffer(owned,&protection)) {
        status=KH02_FREEZE_PROTECT_FAILED;goto fail;
    }
    out->data=owned;out->length=KH02_FULL_SAVE_SIZE;out->capacity=KH02_FULL_SAVE_SIZE;
    return KH02_FREEZE_OK;
fail:
    release_buffer(owned);return status;
}
