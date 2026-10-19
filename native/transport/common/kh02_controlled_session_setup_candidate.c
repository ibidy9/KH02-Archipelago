/* Effects-off game-thread bootstrap. No mapped transport/launcher is installed
   here; invoke from the existing owner-thread transport after startup. */
#include "kh02_controlled_session_harness_candidate.c"
#include <bcrypt.h>
#include "kh02_controlled_owner_candidate.h"
static uint32_t controlled_transport_permit(void *unused);
MEMBER_API Kh02PublicationService *Kh02SessionPublicationServiceOpen(const char*,Kh02DiskLookup*);
KH02_RECEIPT_API Kh02DiskLookup *Kh02DiskLookupCreateEmpty(const Kh02ReceiptScope*,
    const Kh02ReceiptIdentity*,uint32_t,const uint64_t*,uint32_t);
typedef struct Kh02FreshSetupWire {
    uint32_t magic,version,size,flags;uint64_t cookie,load_id;
    uint32_t team,slot,seed_size,history_count,supported_count,path_size,prefix_size,reserved;
    unsigned char binding[32],generation[16],padding[16];
} Kh02FreshSetupWire;
_Static_assert(sizeof(Kh02FreshSetupWire)==128,"fresh setup ABI");
typedef unsigned (*Kh02StorageCheck)(uint64_t,const unsigned char*,const unsigned char*);
static Kh02StorageCheck storage_check;
static uint64_t storage_created;
static unsigned char storage_binding[32],storage_digest[32];
static uint32_t setup_attempted;
static DWORD setup_thread;
static uint32_t storage_permit(void *unused) {
    (void)unused;
    return GetCurrentThreadId()==setup_thread && storage_check &&
        storage_check(storage_created,storage_binding,storage_digest)==1;
}
static uint32_t fresh_setup_arm(unsigned char *base,const unsigned char *input,uint32_t size,
    uint64_t created,Kh02StorageCheck check) {
    Kh02FreshSetupWire h;unsigned char *copy=NULL,*aligned=NULL;
    const unsigned char *seed,*path,*prefix;Kh02ReceiptScope scope;Kh02DiskLookup *lookup;
    Kh02PublicationService *service;Kh02FrozenSave baseline={0};BCRYPT_ALG_HANDLE algorithm=NULL;
    char journal[1024]={0};wchar_t text[1024];uint64_t grant_nonce;uint32_t status=2,entry_count,target,offset;Kh02DiskEntry entries[2]={0};
    static const char hex[]="0123456789abcdef";
    if(setup_attempted)return 2;setup_attempted=1;setup_thread=GetCurrentThreadId();
    if(!base || !input || size<128 || size>128+4096+4096*32+1023+32 || !created || !check)return 2;
    copy=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copy)return 2;
    memcpy(copy,input,size);memcpy(&h,copy,128);
    if(h.magic!=0x4b465350 || h.version!=1 || h.size!=size || h.flags!=6 || !h.cookie || h.load_id!=1 ||
       !h.seed_size || h.seed_size>4096 || h.history_count>4096 || !h.supported_count || h.supported_count>4096 ||
       h.path_size<4 || h.path_size>1023 || h.prefix_size!=32 || h.reserved ||
       (uint64_t)128+h.seed_size+h.history_count*24+h.supported_count*8+h.path_size+32>size)goto done;
    memcpy(&entry_count,h.padding,4);memcpy(&target,h.padding+4,4);
    if(!entry_count || entry_count>2 || target!=1)goto done;
    memcpy(&grant_nonce,h.padding+8,8);if(!grant_nonce)goto done;
    {unsigned char any=0;for(uint32_t i=0;i<16;++i)any|=h.generation[i];if(!any)goto done;}
    seed=copy+128;path=seed+h.seed_size+h.history_count*24+h.supported_count*8;prefix=path+h.path_size;
    for(uint32_t i=0;i<32;++i)if(!memchr(hex,prefix[i],16))goto done;
    if(!((path[0]>='A'&&path[0]<='Z') || (path[0]>='a'&&path[0]<='z')) || path[1]!=':' ||
       (path[2]!='/' && path[2]!='\\') || memchr(path,0,h.path_size) ||
       !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)path,h.path_size,text,1024))goto done;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0 ||
       BCryptHash(algorithm,NULL,0,copy,size,storage_digest,32)<0)goto done;
    if(check(created,h.binding,storage_digest)!=1)goto done;
    storage_check=check;storage_created=created;memcpy(storage_binding,h.binding,32);
    aligned=VirtualAlloc(NULL,h.history_count*24+h.supported_count*8,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!aligned)goto done;
    memcpy(aligned,seed+h.seed_size,h.history_count*24+h.supported_count*8);
    scope=(Kh02ReceiptScope){h.cookie,1,h.team,h.slot,seed,h.seed_size,0};
    offset=(uint32_t)(prefix-copy)+32;
    for(uint32_t i=0;i<entry_count;i++) {
        uint32_t receipt_size,reserved;
        if(size-offset<8)goto done;
        memcpy(&receipt_size,copy+offset,4);memcpy(&reserved,copy+offset+4,4);offset+=8;
        if(reserved || receipt_size<56 || receipt_size>KH02_RECEIPT_MAX_BYTES || size-offset<0x3b40u+receipt_size)goto done;
        entries[i]=(Kh02DiskEntry){copy+offset,0x3b40,0,copy+offset+0x3b40,receipt_size,0};offset+=0x3b40+receipt_size;
    }
    if(offset!=size)goto done;
    lookup=Kh02DiskLookupCreate(&scope,(Kh02ReceiptIdentity*)aligned,h.history_count,
        (uint64_t*)(aligned+h.history_count*24),h.supported_count,entries,entry_count);
    if(!lookup)goto done;
    memcpy(journal,path,h.path_size);service=Kh02SessionPublicationServiceOpen(journal,lookup);if(!service)goto done;
    /* Startup's loader callback never executes this; the game-thread transport
       invokes it before publishing any save hooks. Retain borrowed resources. */
    status=Kh02SaveHarnessCreate(base,lookup,&scope,(Kh02ReceiptIdentity*)aligned,h.history_count,
        (uint64_t*)(aligned+h.history_count*24),h.supported_count,service);
    if(status!=1)goto done;
    status=2;
    if(Kh02FreezeSave(harness_read,NULL,(uintptr_t)base,(uintptr_t)base+0x4485520,&baseline)!=KH02_FREEZE_OK)goto done;
    status=Kh02SaveHarnessPrepareResume(baseline.data,baseline.length,(const char*)prefix,32,target,storage_permit,NULL);
    if(status==1 && storage_permit(NULL))status=Kh02GrantOwnerArm((uintptr_t)base,harness_life,h.cookie,grant_nonce);else status=2;
    if(status==1)status=Kh02ControlledOwnerBind(harness_router,controlled_transport_permit,NULL);
    if(status==1 && storage_permit(NULL))status=Kh02SaveHarnessPublish();else status=2;
done:
    if(status!=1){Kh02GrantOwnerStop();if(harness_router)Kh02SaveRouterStop(harness_router);}
    if(baseline.data)Kh02ReleaseFrozenSave(&baseline);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    /* Setup/lookup/history remain retained for the harness lifetime. */
    return status;
}
MEMBER_API uint32_t Kh02FreshSetupArm(const unsigned char *input,uint32_t size) {
    FILETIME created,exit_time,kernel,user;HMODULE storage=GetModuleHandleW(L"dinput8.dll");
    Kh02StorageCheck check=NULL;FARPROC found;
    if(!storage || !GetProcessTimes(GetCurrentProcess(),&created,&exit_time,&kernel,&user))return 2;
    found=GetProcAddress(storage,"Kh02ResumeStorageCheck");if(!found)return 2;
    memcpy(&check,&found,sizeof(check));
    return fresh_setup_arm((unsigned char*)GetModuleHandleW(NULL),input,size,
        ((uint64_t)created.dwHighDateTime<<32)|created.dwLowDateTime,check);
}
#ifdef KH02_FRESH_SETUP_SELFTEST
MEMBER_API uint32_t Kh02FreshSetupTestArm(unsigned char *base,const unsigned char *input,uint32_t size,
    uint64_t created,Kh02StorageCheck check){return fresh_setup_arm(base,input,size,created,check);}
#endif
