#include "provider_install.h"
#include <string.h>
static BOOL copy(const void *p,void *out,SIZE_T n) {
    SIZE_T got=0;return ReadProcessMemory(GetCurrentProcess(),p,out,n,&got)&&got==n;
}
static BOOL protect(void *p,SIZE_T n,DWORD flags,DWORD *old){return VirtualProtect(p,n,flags,old);}
static void *exchange(void *volatile *p,void *replacement,void *expected) {
    return InterlockedCompareExchangePointer(p,replacement,expected);
}
BOOL Kh02InstallProvider(Kh02ProviderInstall *s,unsigned char *base,void *const replacements[5],const Kh02TableOps *ops) {
    static const unsigned slots[5]={2,3,4,5,8};
    static const uintptr_t expected[9]={0x23ce40,0x52ce20,0x52ce40,0x537440,0x533070,0x527030,0x537280,0x23ce40,0x531ae0};
    static const unsigned char prefix[10]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x30};
    static const Kh02TableOps real_ops={protect,exchange};
    if(!s || s->status!=KH02_TABLE_NEW)return FALSE;
    s->status=KH02_TABLE_REFUSED;
    if(!base || !replacements)return FALSE;
    if(!ops)ops=&real_ops;
    if(!ops->protect || !ops->exchange)return FALSE;
    for(unsigned i=0;i<5;i++)if(!replacements[i])return FALSE;
    IMAGE_DOS_HEADER dos;IMAGE_NT_HEADERS64 nt;unsigned char actual[10];void *table_pointer;
    if(!copy(base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || dos.e_lfanew>0x100000 ||
       !copy(base+dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE ||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       nt.OptionalHeader.SizeOfImage!=0x4f5f000 ||
       !copy(base+0x531ae0,actual,sizeof(actual)) || memcmp(actual,prefix,sizeof(prefix)) ||
       !copy(base+0x4485688,&table_pointer,sizeof(table_pointer)) || table_pointer!=base+0x3755190)return FALSE;
    void *volatile *table=(void*volatile*)(base+0x3755190);
    if(!copy((const void*)table,s->original,sizeof(s->original)))return FALSE;
    for(unsigned i=0;i<9;i++)if(s->original[i]!=(void*)(base+expected[i]))return FALSE;
    /* Snapshot values only: the object's real vtable pointer remains untouched. */
    DWORD ignored;
    if(!ops->protect((void*)table,sizeof(s->original),PAGE_READWRITE,&s->original_protection))return FALSE;
    for(unsigned i=0;i<5;i++) {
        unsigned slot=slots[i];
        if(ops->exchange(table+slot,replacements[i],s->original[slot])!=s->original[slot])goto rollback;
        s->changed++;
    }
    if(!ops->protect((void*)table,sizeof(s->original),s->original_protection,&ignored))goto rollback;
    s->status=KH02_TABLE_INSTALLED;return TRUE;
rollback:
    /* Establish writable protection again before touching any partially changed
       cell. Never assume a failed protection call left a writable page. */
    s->status=KH02_TABLE_UNSAFE;
    if(!ops->protect((void*)table,sizeof(s->original),PAGE_READWRITE,&ignored))return FALSE;
    BOOL restored=TRUE;
    while(s->changed) {
        unsigned i=--s->changed,slot=slots[i];
        if(ops->exchange(table+slot,s->original[slot],replacements[i])!=replacements[i])restored=FALSE;
    }
    if(!ops->protect((void*)table,sizeof(s->original),s->original_protection,&ignored))restored=FALSE;
    void *after[9];
    if(!copy((const void*)table,after,sizeof(after)) || memcmp(after,s->original,sizeof(after)))restored=FALSE;
    if(restored)s->status=KH02_TABLE_ROLLED_BACK;
    /* UNSAFE is not permission to continue, unload a late DLL, or retry.
       The mandatory early bootstrap must refuse process initialization. */
    return FALSE;
}
