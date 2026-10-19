/* v109: bounded boss suppression; original forwarding retained on refusal. */
#include "kh02_stat_suppression_v109.h"
typedef struct StatTraceRowV108 {
    uint64_t context,subrecord,name,epoch_before,epoch_after,cached_context;
    uint32_t kind,thread,owner_thread,flags,objective;
    uint32_t valid_before,valid_after,reserved;
    unsigned char bonuses_before[32],bonuses_after[32];
    unsigned char completion_before[9],completion_after[9];
    unsigned char padding[6];
} StatTraceRowV108;
typedef struct StatTraceV108 {
    uint32_t magic,version,size,capacity;
    volatile LONG sequence,count,lock,reserved;
    volatile LONG64 raw,forwarded,wrong_thread,dropped;
    StatTraceRowV108 rows[128];
} StatTraceV108;
_Static_assert(sizeof(StatTraceRowV108)==168,"stat trace row ABI");
_Static_assert(sizeof(StatTraceV108)==21568,"stat trace ABI");
static StatTraceV108 *stat_trace_v108;
static HANDLE stat_trace_mapping_v108;
static void *stat_award_trampoline_v108,*stat_rebuild_trampoline_v108;
static const unsigned char stat_award_prefix_v108[14]={0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x56,0x48,0x8d,0x6c,0x24,0xd1};
/* Whole-instruction prefixes verified against the supported executable. */
static const unsigned char stat_rebuild_prefix_v108[17]={0x48,0x8b,0xc4,0x55,0x56,0x57,0x48,0x8d,0x68,0xa1,0x48,0x81,0xec,0xc0,0x0,0x0,0x0};
static int stat_trace_init_v108(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APStatTraceV108-%lu",GetCurrentProcessId());
    stat_trace_mapping_v108=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(StatTraceV108),name);
    if(!stat_trace_mapping_v108 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    stat_trace_v108=MapViewOfFile(stat_trace_mapping_v108,FILE_MAP_ALL_ACCESS,0,0,sizeof(StatTraceV108));
    if(!stat_trace_v108)return 0;
    stat_trace_v108->magic=0x38315453;stat_trace_v108->version=1;
    stat_trace_v108->size=sizeof(StatTraceV108);stat_trace_v108->capacity=128;
    return 1;
}
static uint32_t stat_trace_read_v108(StatTraceRowV108 *r,int after) {
    uint64_t table=0;uint32_t result=0;
    if(r->thread!=r->owner_thread || r->context<0x10000 ||
       r->context>0x00007fffffff8000ULL ||
       !read_local(NULL,r->context,&table,8) || table!=game.base+0x36df228)return 0;
    result=1;
    if(r->subrecord==r->context+0x2930 &&
       read_local(NULL,r->subrecord+0x64,after?r->bonuses_after:r->bonuses_before,32))result|=2;
    if(read_local(NULL,r->context+0x3538,after?r->completion_after:r->completion_before,9))result|=4;
    return result;
}
static void stat_trace_before_v108(StatTraceRowV108 *r) {
    r->thread=GetCurrentThreadId();r->owner_thread=owner_thread;
    if(!stat_trace_v108)return;
    InterlockedIncrement64(&stat_trace_v108->raw);
    if(r->thread!=owner_thread){InterlockedIncrement64(&stat_trace_v108->wrong_thread);return;}
    r->epoch_before=Kh02LifecycleEpoch(life);r->cached_context=session.context;
    r->flags=(active_flag?1u:0u)|(!fault?2u:0u)|(Kh02ControlledStartAuthorized()?4u:0u);
    r->valid_before=stat_trace_read_v108(r,0);
}
static void stat_trace_after_v108(StatTraceRowV108 *r) {
    if(!stat_trace_v108)return;
    if(!(r->flags&256u))InterlockedIncrement64(&stat_trace_v108->forwarded);
    if(r->thread==owner_thread){r->epoch_after=Kh02LifecycleEpoch(life);r->valid_after=stat_trace_read_v108(r,1);}
    if(InterlockedCompareExchange(&stat_trace_v108->lock,1,0)) {
        InterlockedIncrement64(&stat_trace_v108->dropped);return;
    }
    InterlockedIncrement(&stat_trace_v108->sequence);
    if(stat_trace_v108->count<128)stat_trace_v108->rows[stat_trace_v108->count++]=*r;
    else InterlockedIncrement64(&stat_trace_v108->dropped);
    MemoryBarrier();InterlockedIncrement(&stat_trace_v108->sequence);
    InterlockedExchange(&stat_trace_v108->lock,0);
}
static int stat_suppress_v109(const StatTraceRowV108 *row) {
    uint64_t epoch;
    if(GetCurrentThreadId()!=owner_thread)return 0;
    epoch=Kh02LifecycleEpoch(life);
    if(!active_flag || fault || !session.gate.current.cookie ||
       !epoch || !Kh02ControlledStartAuthorized())return 0;
    Kh02BonusRequestV109 q={game.base,row->context,row->subrecord,row->name,row->kind,1,(int32_t)row->objective};
    return Kh02BonusSuppressV109(read_local,NULL,&q) && epoch==Kh02LifecycleEpoch(life) &&
        active_flag && !fault && Kh02ControlledStartAuthorized();
}
static void local_named198(const StatTraceRowV108*);
static void __fastcall stat_award_observe_v108(void *context,uint64_t name,int objective) {
    typedef void (__fastcall *Fn)(void*,uint64_t,int);
    StatTraceRowV108 r={0};r.kind=1;r.context=(uintptr_t)context;
    r.subrecord=r.context+0x2930;r.name=name;r.objective=(uint32_t)objective;
    InterlockedIncrement(&flight);stat_trace_before_v108(&r);
    if(stat_suppress_v109(&r))r.flags|=256u;
    else ((Fn)stat_award_trampoline_v108)(context,name,objective);
    r.reserved=(uint32_t)flight;stat_trace_after_v108(&r);local_named198(&r);InterlockedDecrement(&flight);
}
static unsigned char __fastcall stat_rebuild_observe_v108(void *subrecord,void *context) {
    typedef unsigned char (__fastcall *Fn)(void*,void*);
    unsigned char result;StatTraceRowV108 r={0};r.kind=2;
    r.context=(uintptr_t)context;r.subrecord=(uintptr_t)subrecord;
    InterlockedIncrement(&flight);stat_trace_before_v108(&r);
    if(stat_suppress_v109(&r)){r.flags|=256u;result=1;}
    else result=((Fn)stat_rebuild_trampoline_v108)(subrecord,context);
    stat_trace_after_v108(&r);InterlockedDecrement(&flight);return result;
}
