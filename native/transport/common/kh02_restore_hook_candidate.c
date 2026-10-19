#include "kh02_restore_hook_candidate.h"
#include <string.h>
#ifdef KH02_RESTORE_HOOK_SELFTEST
uint32_t Kh02RestoreHookTestFailRegistration,Kh02RestoreHookTestPublishCountdown;
#endif

typedef struct Prefix { unsigned char bytes[32],unwind[16];uint32_t size; } Prefix;
static const Prefix prefixes[]={
    {{0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,0x48,0x83,0xec,0x20},
     {1,15,6,0,15,0x64,9,0,15,0x34,8,0,15,0x32,11,0x70},15},
    {{0x40,0x55,0x56,0x41,0x56,0x48,0x83,0xec,0x20,0x44,0x8b,0xf2,0x48,0x8b,0xe9},
     {1,15,4,0,9,0x32,5,0xe0,3,0x60,2,0x50},15},
    {{0x48,0x89,0x5c,0x24,8,0x48,0x89,0x6c,0x24,16,0x48,0x89,0x74,0x24,24},
     {1,15,6,0,15,0x64,3,0,10,0x54,2,0,5,0x34,1,0},15},
    {{0x48,0x8d,0x91,0x38,3,0,0,0x48,0x81,0xc1,0x60,0x40,0,0},
     {1,14,0,0},14},
    {{0x48,0x89,0x5c,0x24,8,0x48,0x89,0x74,0x24,16,0x57,0x48,0x83,0xec,0x20},
     {1,15,6,0,15,0x64,7,0,15,0x34,6,0,15,0x32,11,0x70},15},
    {{0x48,0x89,0x5c,0x24,8,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0xc7,0x81,0x90,0,0,0,1,0,0,0},
     {1,23,4,0,10,0x34,6,0,10,0x32,6,0x70},23},
    {{0x48,0x89,0x5c,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x83,0xea,1},
     {1,16,4,0,10,0x34,8,0,10,0x32,6,0x70},16}
};
static int exact(unsigned char *address,const unsigned char *expected,uint32_t size) {
    unsigned char actual[32];SIZE_T count=0;
    return size<=32 && ReadProcessMemory(GetCurrentProcess(),address,actual,size,&count) &&
        count==size && !memcmp(actual,expected,size);
}
static void absolute_jump(unsigned char *at,void *to) {
    uint64_t value=(uint64_t)(uintptr_t)to;
    at[0]=0xff;at[1]=0x25;memset(at+2,0,4);memcpy(at+6,&value,8);
}
uint32_t Kh02RestoreHookPrepare(Kh02RestoreHook *hook,unsigned char *target,uint32_t kind,void *wrapper) {
    const Prefix *prefix;HMODULE pinned;DWORD old;unsigned char *page;
    if(!hook || !target || !wrapper || kind>KH02_SAVE_RESTORE)return KH02_BAD_REQUEST;
    if(hook->target || hook->trampoline || hook->owned || hook->prepared)return KH02_BUSY;
    prefix=&prefixes[kind];
    if(!exact(target,prefix->bytes,prefix->size))return KH02_PATCH_CONFLICT;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        (LPCWSTR)(uintptr_t)wrapper,&pinned))return KH02_CLEANUP_UNCERTAIN;
    page=VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!page)return KH02_CLEANUP_UNCERTAIN;
    /* From here on retain the page/table. Never release potentially reachable code.
       Even a failed preparation is terminal for this single-use candidate entry. */
    hook->target=target;hook->trampoline=page;hook->owner_thread=GetCurrentThreadId();
    hook->size=prefix->size;memcpy(hook->original,prefix->bytes,hook->size);
    memcpy(page,prefix->bytes,hook->size);
    /* R11 is volatile, is not an argument, and none of these copied prefixes
       define it. The register jump avoids Windows' FF /4 memory-jump epilogue
       interpretation while retaining all argument/nonvolatile registers. */
    page[hook->size]=0x49;page[hook->size+1]=0xbb;
    { uint64_t resume=(uint64_t)(uintptr_t)(target+hook->size);
      memcpy(page+hook->size+2,&resume,8); }
    page[hook->size+10]=0x41;page[hook->size+11]=0xff;page[hook->size+12]=0xe3;
    page[hook->size+13]=0x90;
    memcpy(page+0x100,prefix->unwind,16);
    hook->unwind_table=(RUNTIME_FUNCTION*)(page+0x200);
    hook->unwind_table->BeginAddress=0;hook->unwind_table->EndAddress=hook->size+14;
    hook->unwind_table->UnwindData=0x100;
    memset(hook->patch,0x90,sizeof(hook->patch));absolute_jump(hook->patch,wrapper);
    if(!VirtualProtect(page,4096,PAGE_EXECUTE_READ,&old) ||
       !FlushInstructionCache(GetCurrentProcess(),page,hook->size+14))return KH02_CLEANUP_UNCERTAIN;
#ifdef KH02_RESTORE_HOOK_SELFTEST
    if(Kh02RestoreHookTestFailRegistration)return KH02_CLEANUP_UNCERTAIN;
#endif
    if(!RtlAddFunctionTable(hook->unwind_table,1,(DWORD64)(uintptr_t)page))return KH02_CLEANUP_UNCERTAIN;
    hook->registered=1;MemoryBarrier();hook->prepared=1;
    return KH02_OK; /* No target byte has been changed. */
}
uint32_t Kh02RestoreHookPublish(Kh02RestoreHook *hook) {
    DWORD ignored;
    if(!hook || !hook->prepared || !hook->registered || !hook->trampoline)return KH02_BAD_REQUEST;
    if(hook->owner_thread!=GetCurrentThreadId() || hook->owned)return KH02_BUSY;
    if(!exact(hook->target,hook->original,hook->size))return KH02_PATCH_CONFLICT;
#ifdef KH02_RESTORE_HOOK_SELFTEST
    if(Kh02RestoreHookTestPublishCountdown && !--Kh02RestoreHookTestPublishCountdown)return KH02_CLEANUP_UNCERTAIN;
#endif
    if(!VirtualProtect(hook->target,hook->size,PAGE_EXECUTE_READWRITE,&hook->protection))return KH02_CLEANUP_UNCERTAIN;
    hook->owned=1; /* Same ownership-before-first-write rule as the proven owner. */
    memcpy(hook->target,hook->patch,hook->size);
    if(!FlushInstructionCache(GetCurrentProcess(),hook->target,hook->size) ||
       !VirtualProtect(hook->target,hook->size,hook->protection,&ignored) ||
       !exact(hook->target,hook->patch,hook->size))return KH02_CLEANUP_UNCERTAIN;
    return KH02_OK;
}
uint32_t Kh02RestoreHookRestore(Kh02RestoreHook *hook) {
    DWORD ignored;
    if(!hook)return KH02_BAD_REQUEST;
    if(!hook->owned)return KH02_OK;
    if(hook->owner_thread!=GetCurrentThreadId())return KH02_BUSY;
    if(!exact(hook->target,hook->patch,hook->size) && !exact(hook->target,hook->original,hook->size))
        return KH02_PATCH_CONFLICT;
    if(!VirtualProtect(hook->target,hook->size,PAGE_EXECUTE_READWRITE,&ignored))return KH02_CLEANUP_UNCERTAIN;
    memcpy(hook->target,hook->original,hook->size);
    if(!FlushInstructionCache(GetCurrentProcess(),hook->target,hook->size) ||
       !VirtualProtect(hook->target,hook->size,hook->protection,&ignored) ||
       !exact(hook->target,hook->original,hook->size))return KH02_CLEANUP_UNCERTAIN;
    hook->owned=0;return KH02_OK;
}
