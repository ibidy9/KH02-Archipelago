/* Development-only fixed event capture. Always forwards the exact original arguments.
 * Group IDs are candidates until user-labelled defeats corroborate them. */
typedef struct WipeoutRowV143 {
    uint64_t receiver,group,enemy,killer,epoch,context,receiver_vtable;
    uint64_t candidates[3];
    uint32_t thread,owner_thread,flags,match;
    char map[64];
} WipeoutRowV143;
#ifdef KH02_GRANT_OWNER_LIVE
static void local_child193(const WipeoutRowV143*);
#endif
typedef struct WipeoutTraceV143 {
    uint32_t magic,version,size,capacity;
    volatile LONG sequence,count,lock,reserved;
    volatile LONG64 raw,forwarded,wrong_thread,dropped;
    WipeoutRowV143 rows[256];
} WipeoutTraceV143;
_Static_assert(sizeof(WipeoutRowV143)==160,"wipeout row ABI");
_Static_assert(sizeof(WipeoutTraceV143)==41024,"wipeout trace ABI");
static WipeoutTraceV143 *wipeout_trace_v143;
static HANDLE wipeout_mapping_v143;
static void *wipeout_trampoline_v143;
/* Three complete home-space stores, no relative operands. */
static const unsigned char wipeout_prefix_v143[15]={0x4c,0x89,0x4c,0x24,0x20,0x4c,0x89,0x44,0x24,0x18,0x48,0x89,0x54,0x24,0x10};
static int wipeout_init_v143(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APWipeoutV143-%lu",GetCurrentProcessId());
    wipeout_mapping_v143=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(WipeoutTraceV143),name);
    if(!wipeout_mapping_v143 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    wipeout_trace_v143=MapViewOfFile(wipeout_mapping_v143,FILE_MAP_ALL_ACCESS,0,0,sizeof(WipeoutTraceV143));
    if(!wipeout_trace_v143)return 0;
    wipeout_trace_v143->magic=0x33345057;wipeout_trace_v143->version=1;
    wipeout_trace_v143->size=sizeof(WipeoutTraceV143);wipeout_trace_v143->capacity=256;
    return 1;
}
#include "kh02_darkside_checks_v144.inc"
static void __fastcall wipeout_observe_v143(void *receiver,uint64_t group,void *enemy,void *killer) {
    typedef void (__fastcall *Fn)(void*,uint64_t,void*,void*);
    WipeoutRowV143 r={0};
    r.receiver=(uintptr_t)receiver;r.group=group;r.enemy=(uintptr_t)enemy;r.killer=(uintptr_t)killer;
    r.thread=GetCurrentThreadId();r.owner_thread=owner_thread;
    InterlockedIncrement(&flight);
    if(wipeout_trace_v143)InterlockedIncrement64(&wipeout_trace_v143->raw);
    if(r.thread==owner_thread) {
        r.flags=1u|(active_flag?2u:0u)|(!fault?4u:0u);
        r.epoch=Kh02LifecycleEpoch(life);r.context=session.context;
        if(Kh02ControlledStartAuthorized())r.flags|=8u;
        if(read_local(NULL,r.receiver,&r.receiver_vtable,8))r.flags|=16u;
        /* Only resolve known names on the authenticated owner thread. Never add names. */
        if((r.flags&15u)==15u && r.epoch && session.gate.current.cookie) {
            r.candidates[0]=start_name(L"dw_08_40_en_Group_DarkSide",0);
            r.candidates[1]=start_name(L"dw_08_42_en_Group_DarkSide",0);
            r.candidates[2]=start_name(L"dw_08_48_en_Group_DarkSide",0);
            for(uint32_t i=0;i<3;i++)if(r.candidates[i] && group==r.candidates[i])r.match|=1u<<i;
            if(r.context && read_local(NULL,r.context+0x384c,r.map,sizeof(r.map)))r.flags|=32u;
        }
    } else if(wipeout_trace_v143)InterlockedIncrement64(&wipeout_trace_v143->wrong_thread);
    ((Fn)wipeout_trampoline_v143)(receiver,group,enemy,killer);
    darkside_complete_v144(&r);
#ifdef KH02_GRANT_OWNER_LIVE
    broadcast_child192(r.group,r.enemy,r.flags,r.match);
    local_child193(&r);
#endif
    if(wipeout_trace_v143) {
        InterlockedIncrement64(&wipeout_trace_v143->forwarded);
        if(InterlockedCompareExchange(&wipeout_trace_v143->lock,1,0))InterlockedIncrement64(&wipeout_trace_v143->dropped);
        else {
            InterlockedIncrement(&wipeout_trace_v143->sequence);
            if(wipeout_trace_v143->count<256)wipeout_trace_v143->rows[wipeout_trace_v143->count++]=r;
            else InterlockedIncrement64(&wipeout_trace_v143->dropped);
            MemoryBarrier();InterlockedIncrement(&wipeout_trace_v143->sequence);
            InterlockedExchange(&wipeout_trace_v143->lock,0);
        }
    }
    InterlockedDecrement(&flight);
}

