/* Offline installer primitive. No process discovery, launcher or live command.
   Prepared callbacks, code and hook storage remain retained until process exit. */
#include "kh02_restore_hook_candidate.h"
#include "kh02_save_pipeline_candidate.h"
#include <string.h>
typedef struct Kh02SaveHookCallbacks {
    Kh02PipelineStage load;uintptr_t (*capture)(uintptr_t);uintptr_t (*restore)(uintptr_t,uint32_t);
    Kh02PipelineStage stage;Kh02PipelineRefresh refresh;
    unsigned char (*platform)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
} Kh02SaveHookCallbacks;
typedef struct Kh02SaveHookOriginals {
    Kh02PipelineStage load;uintptr_t (*capture)(uintptr_t);uintptr_t (*restore)(uintptr_t,uint32_t);
    Kh02PipelineStage stage;Kh02PipelineRefresh refresh;Kh02PublicationOriginal platform;
} Kh02SaveHookOriginals;
static Kh02RestoreHook entries[5];static Kh02SaveHookCallbacks calls;
static Kh02SaveHookOriginals originals;static void *volatile *cell;
static DWORD owner;static uint32_t attempted,prepared,published,cell_owned;
static unsigned char load_entry(uintptr_t p,uint32_t i){return calls.load(p,i);}
static uintptr_t capture_entry(uintptr_t p){return calls.capture(p);}
static uintptr_t restore_entry(uintptr_t p,uint32_t m){return calls.restore(p,m);}
static unsigned char stage_entry(uintptr_t p,uint32_t i){return calls.stage(p,i);}
static uintptr_t refresh_entry(uintptr_t p,uint32_t i){return calls.refresh(p,i);}
static __declspec(noinline) unsigned char platform_entry(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e) {
    return calls.platform(a,b,c,d,e,(uintptr_t)__builtin_return_address(0));
}
static int hook_read(uintptr_t address,void *out,uint32_t size) {
    SIZE_T got=0;return ReadProcessMemory(GetCurrentProcess(),(void*)address,out,size,&got)&&got==size;
}
/* Prepare all entries before caller creates its lifecycle/router from originals.
   Base identity/hash authorization belongs to the future sealed host protocol. */
MEMBER_API uint32_t Kh02SaveHooksPrepare(unsigned char *base,Kh02SaveHookOriginals *out) {
    IMAGE_DOS_HEADER dos;IMAGE_NT_HEADERS64 nt;void *found;uint32_t status;
    const uint32_t rvas[5]={0x5399b0,0x447b20,0x453d60,0x5391e0,0x53ecf0},kinds[5]={0,3,6,1,4};
    void *wrappers[5]={load_entry,capture_entry,restore_entry,stage_entry,refresh_entry};
    if(attempted || !out)return KH02_BAD_REQUEST;attempted=1;owner=GetCurrentThreadId();memset(out,0,sizeof(*out));
    if(!base || !hook_read((uintptr_t)base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE ||
       dos.e_lfanew<0 || dos.e_lfanew>0x100000 || !hook_read((uintptr_t)base+dos.e_lfanew,&nt,sizeof(nt)) ||
       nt.Signature!=IMAGE_NT_SIGNATURE || nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
       nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt.OptionalHeader.SizeOfImage!=0x04f5f000)return KH02_BAD_BUILD;
    cell=(void*volatile*)(base+0x37551a8);originals.platform=(Kh02PublicationOriginal)(base+0x537440);
    if(!hook_read((uintptr_t)cell,&found,8) || found!=(void*)originals.platform)return KH02_PATCH_CONFLICT;
    for(uint32_t i=0;i<5;++i){status=Kh02RestoreHookPrepare(&entries[i],base+rvas[i],kinds[i],wrappers[i]);if(status!=KH02_OK)return status;}
    originals.load=(Kh02PipelineStage)entries[0].trampoline;originals.capture=(uintptr_t(*)(uintptr_t))entries[1].trampoline;
    originals.restore=(uintptr_t(*)(uintptr_t,uint32_t))entries[2].trampoline;
    originals.stage=(Kh02PipelineStage)entries[3].trampoline;originals.refresh=(Kh02PipelineRefresh)entries[4].trampoline;
    *out=originals;prepared=1;return KH02_OK;
}
/* Callbacks must already point to fully initialized owners; a partial publish
   retains the barrier and those owners. Caller revokes adoption on any failure.
   No automatic rollback, unpatch, disposal or original-save fall-through. */
MEMBER_API uint32_t Kh02SaveHooksPublish(const Kh02SaveHookCallbacks *next) {
    DWORD old,ignored;void *found;uint32_t status;
    if(!prepared || published || GetCurrentThreadId()!=owner || !next || !next->load || !next->capture ||
       !next->restore || !next->stage || !next->refresh || !next->platform)return KH02_BAD_REQUEST;
    published=1;calls=*next;
    if(!VirtualProtect((void*)cell,8,PAGE_READWRITE,&old))return KH02_CLEANUP_UNCERTAIN;
    found=InterlockedCompareExchangePointer(cell,(void*)platform_entry,(void*)originals.platform);
    if(found==(void*)originals.platform)cell_owned=1;
    if(!VirtualProtect((void*)cell,8,old,&ignored))return KH02_CLEANUP_UNCERTAIN;
    if(!cell_owned)return KH02_PATCH_CONFLICT;
    for(uint32_t i=0;i<5;++i){status=Kh02RestoreHookPublish(&entries[i]);if(status!=KH02_OK)return status;}
    return KH02_OK;
}
MEMBER_API uint32_t Kh02SaveHooksBarrierOwned(void){return cell_owned;}
#ifdef KH02_SAVE_HOOKS_SELFTEST
MEMBER_API void Kh02SaveHooksTestFail(uint32_t n){Kh02RestoreHookTestPublishCountdown=n;}
#endif
