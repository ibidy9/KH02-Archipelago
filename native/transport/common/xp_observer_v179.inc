/* v180 diagnostic: authenticated 2x earned delta, bounded fixed capture.
 * Whole 41-byte leaf relocation was executed against 10,040 synthetic cases.
 * Never copy its first 18 bytes into the ordinary prefix trampoline. */
#include "kh02_xp_policy_v179.h"
typedef struct XpRow179 {
    uint64_t receiver,caller,context,cookie,epoch_before,epoch_after;
    int32_t earned,before,after,result;
    uint32_t reason,thread;
} XpRow179;
typedef struct XpTrace179 {
    uint32_t magic,version,size,capacity;
    volatile LONG sequence,count,lock,reserved;
    volatile LONG64 raw,forwarded,dropped;
    volatile LONG64 reasons[9];
    XpRow179 rows[256];
} XpTrace179;
_Static_assert(sizeof(XpRow179)==72,"XP row ABI");
_Static_assert(sizeof(XpTrace179)==18560,"XP trace ABI");
static XpTrace179 *xp_trace179;
static HANDLE xp_mapping179;
static void *xp_original179;
static const unsigned char xp_leaf179[41]={
    0x44,0x8b,0x49,0x14,0x45,0x8d,0x04,0x11,0x45,0x85,0xc0,0x79,0x09,
    0x33,0xc0,0x89,0x41,0x14,0x41,0x2b,0xc1,0xc3,0xb8,0x7f,0x96,0x98,
    0x00,0x44,0x3b,0xc0,0x41,0x0f,0x4c,0xc0,0x89,0x41,0x14,0x41,0x2b,0xc1,0xc3};
static int xp_init179(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APXpV185-%lu",GetCurrentProcessId());
    xp_mapping179=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(XpTrace179),name);
    if(!xp_mapping179 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    xp_trace179=MapViewOfFile(xp_mapping179,FILE_MAP_ALL_ACCESS,0,0,sizeof(XpTrace179));
    if(!xp_trace179)return 0;
    xp_trace179->magic=0x39375058;xp_trace179->version=3;xp_trace179->reserved=0;
    xp_trace179->size=sizeof(XpTrace179);xp_trace179->capacity=256;return 1;
}
static int32_t __fastcall xp_observe179(void *receiver,int32_t earned) {
    typedef int32_t (__fastcall *Fn)(void*,int32_t);
    XpRow179 row={0};Kh02ReceiptOwner *owner=NULL;uint64_t table=0;int32_t applied;
    row.receiver=(uintptr_t)receiver;row.caller=(uintptr_t)__builtin_return_address(0);
    row.earned=earned;row.thread=GetCurrentThreadId();
    InterlockedIncrement(&flight);
    if(xp_trace179)InterlockedIncrement64(&xp_trace179->raw);
    if(row.thread!=owner_thread){row.reason=1;goto forward;}
    if(!active_flag || fault || !life){row.reason=2;goto forward;}
    row.context=session.context;row.cookie=session.gate.current.cookie;
    row.epoch_before=Kh02LifecycleEpoch(life);owner=Kh02LifecycleCurrent(life);
    if(!xp_halves185 || !row.cookie || !row.epoch_before || row.epoch_before!=seen_epoch || !owner ||
       !session.gate.current.observation || !Kh02ControlledStartAuthorized() ||
       Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK){row.reason=3;goto forward;}
    if(row.caller!=game.base+0x4469bc){row.reason=4;goto forward;}
    if(!row.context || row.receiver!=row.context+0x538){row.reason=5;goto forward;}
    if(!read_local(NULL,row.context,&table,8) || table!=game.base+0x36df228 ||
       !read_local(NULL,row.receiver+0x14,&row.before,4)){row.reason=6;goto forward;}
forward:
    applied=kh02_xp_delta179(row.reason==0,xp_halves185,row.before,earned);
    row.result=((Fn)xp_original179)(receiver,applied);
    if(row.thread==owner_thread && life)row.epoch_after=Kh02LifecycleEpoch(life);
    if(!row.reason) {
        if(row.epoch_after!=row.epoch_before || row.context!=session.context ||
           row.cookie!=session.gate.current.cookie || owner!=Kh02LifecycleCurrent(life) ||
           !active_flag || fault || !Kh02ControlledStartAuthorized() ||
           Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK)row.reason=7;
        else if(!read_local(NULL,row.receiver+0x14,&row.after,4))row.reason=8;
    }
    if(xp_trace179) {
        InterlockedIncrement64(&xp_trace179->forwarded);
        InterlockedIncrement64(&xp_trace179->reasons[row.reason]);
        if(InterlockedCompareExchange(&xp_trace179->lock,1,0))InterlockedIncrement64(&xp_trace179->dropped);
        else {
            InterlockedIncrement(&xp_trace179->sequence);
            if(xp_trace179->count<256)xp_trace179->rows[xp_trace179->count++]=row;
            else InterlockedIncrement64(&xp_trace179->dropped);
            MemoryBarrier();InterlockedIncrement(&xp_trace179->sequence);
            InterlockedExchange(&xp_trace179->lock,0);
        }
    }
    InterlockedDecrement(&flight);return row.result;
}
static uint32_t xp_install179(Entry *entry,unsigned char *target) {
    DWORD old,ignored;HMODULE pin;
    if(memcmp(target,xp_leaf179,sizeof(xp_leaf179)))return KH02_PATCH_CONFLICT;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
       (LPCWSTR)(uintptr_t)xp_observe179,&pin))return KH02_CLEANUP_UNCERTAIN;
    xp_original179=VirtualAlloc(NULL,sizeof(xp_leaf179),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!xp_original179)return KH02_CLEANUP_UNCERTAIN;
    memcpy(xp_original179,xp_leaf179,sizeof(xp_leaf179));
    if(!VirtualProtect(xp_original179,sizeof(xp_leaf179),PAGE_EXECUTE_READ,&old) ||
       !FlushInstructionCache(GetCurrentProcess(),xp_original179,sizeof(xp_leaf179)))return KH02_CLEANUP_UNCERTAIN;
    /* Retain clone/mapping with the existing process-lifetime owner, including
       partial failures. No per-hit allocations, files, waits, or worker threads. */
    entry->target=target;entry->size=18;memcpy(entry->original,xp_leaf179,18);
    memset(entry->patch,0x90,18);jump(entry->patch,xp_observe179);
    if(!VirtualProtect(target,18,PAGE_EXECUTE_READWRITE,&entry->protection))return KH02_CLEANUP_UNCERTAIN;
    entry->owned=1;memcpy(target,entry->patch,18);
    if(!FlushInstructionCache(GetCurrentProcess(),target,18) ||
       !VirtualProtect(target,18,entry->protection,&ignored))return KH02_CLEANUP_UNCERTAIN;
    return memcmp(target,entry->patch,18)?KH02_PATCH_CONFLICT:KH02_OK;
}
