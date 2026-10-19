/* Reflected all-Zodiac query observer plus scoped v85 challenge gate.
 * Only fixed FFrame fields, three UObject names, and admitted map are read. */
#include <stdio.h>
typedef struct ZodiacQueryV83 {
    uint64_t node,object,code,owner,epoch,cookie,calls;
    uint32_t result,flags;
    char node_name[128],node_outer[128],object_name[128],map[32];
} ZodiacQueryV83;
typedef struct ZodiacTraceV83 {
    uint32_t magic,version,size,capacity;volatile LONG sequence,count;
    volatile LONG64 raw,forwarded,wrong_thread,frame_rejects,overflow;
    ZodiacQueryV83 rows[32];
} ZodiacTraceV83;
_Static_assert(sizeof(ZodiacQueryV83)==480,"query row ABI");
_Static_assert(sizeof(ZodiacTraceV83)==15424,"query trace ABI");
static ZodiacTraceV83 *zodiac_trace;
static HANDLE zodiac_mapping;
static void *zodiac_query_trampoline;
static const unsigned char zodiac_query_prefix[15]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
static int zodiac_read(uint64_t p,void *out,uint32_t n) {
    return p>=0x10000 && p<0x0000800000000000ULL-n && read_local(NULL,p,out,n);
}
static int zodiac_name(uint64_t object,char out[128],uint64_t *outer) {
    uint64_t words[6],pool,chunk,entry;uint32_t index;uint16_t head;
    unsigned char text[256];unsigned i;
    memset(out,0,128);if(outer)*outer=0;
    if(!zodiac_read(object,words,sizeof(words)) || words[0]<game.base ||
       words[0]>=game.base+0x04f5f000)return 0;
    index=(uint32_t)words[4];
    if(index>=0x200000 || !zodiac_read(game.base+0x4695040,&pool,8) ||
       !zodiac_read(pool+(index>>14)*8,&chunk,8) ||
       !zodiac_read(chunk+(index&0x3fff)*8,&entry,8) ||
       !zodiac_read(entry,&head,2) || !zodiac_read(entry+0x10,text,sizeof(text)))return 0;
    for(i=0;i<127;i++) {
        uint16_t ch=(head&1)?(uint16_t)(text[2*i]|((uint16_t)text[2*i+1]<<8)):text[i];
        if(!ch){if(outer)*outer=words[5];return 1;}
        out[i]=ch<128?(char)ch:'?';
    }
    memset(out,0,128);return 0;
}
#include "mirror_gate_v85.inc"
static int zodiac_observer_init(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APZodiacQueryV83-%lu",GetCurrentProcessId());
    zodiac_mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(ZodiacTraceV83),name);
    if(!zodiac_mapping || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    zodiac_trace=MapViewOfFile(zodiac_mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(ZodiacTraceV83));
    if(!zodiac_trace)return 0;
    zodiac_trace->magic=0x38335a4b;zodiac_trace->version=1;
    zodiac_trace->size=sizeof(ZodiacTraceV83);zodiac_trace->capacity=32;
    return 1;
}
static void __fastcall zodiac_query_observe(void *object,void *frame,void *result) {
    typedef void (__fastcall *Fn)(void*,void*,void*);
    uint64_t fields[5]={0},outer=0;ZodiacQueryV83 row={0};unsigned char answer=0;
    int admitted=GetCurrentThreadId()==owner_thread,frame_ok=0;
    InterlockedIncrement(&flight);
    if(zodiac_trace) {
        InterlockedIncrement64(&zodiac_trace->raw);
        if(!admitted)InterlockedIncrement64(&zodiac_trace->wrong_thread);
        else frame_ok=zodiac_read((uintptr_t)frame,fields,sizeof(fields));
    }
    ((Fn)zodiac_query_trampoline)(object,frame,result);
    if(zodiac_trace)InterlockedIncrement64(&zodiac_trace->forwarded);
    if(!zodiac_trace || !admitted)goto done;
    if(!frame_ok || !zodiac_read((uintptr_t)result,&answer,1)) {
        InterlockedIncrement64(&zodiac_trace->frame_rejects);goto done;
    }
    row.node=fields[2];row.object=fields[3];row.code=fields[4];row.result=answer;
    row.owner=session.context;row.epoch=Kh02LifecycleEpoch(life);row.cookie=session.gate.current.cookie;
    row.flags=(active_flag?1u:0u)|(!fault?2u:0u)|(Kh02ControlledStartAuthorized()?4u:0u);
    if(zodiac_name(row.node,row.node_name,&outer))row.flags|=8;
    if(zodiac_name(outer,row.node_outer,NULL))row.flags|=16;
    if(zodiac_name(row.object,row.object_name,NULL))row.flags|=32;
    if(row.owner && zodiac_read(row.owner+0x384c,row.map,sizeof(row.map)))row.flags|=64;
    mirror_gate_v85(&row,result);
    /* Aggregate identical calls to keep long idle periods bounded. */
    InterlockedIncrement(&zodiac_trace->sequence);
    for(LONG i=0;i<zodiac_trace->count;i++) {
        ZodiacQueryV83 *p=&zodiac_trace->rows[i];
        if(p->node==row.node && p->object==row.object && p->code==row.code &&
           p->epoch==row.epoch && p->result==row.result && p->flags==row.flags &&
           !memcmp(p->map,row.map,sizeof(row.map))) {p->calls++;goto publish;}
    }
    if(zodiac_trace->count<32) {
        row.calls=1;zodiac_trace->rows[zodiac_trace->count]=row;
        InterlockedIncrement(&zodiac_trace->count);
    } else InterlockedIncrement64(&zodiac_trace->overflow);
publish:
    MemoryBarrier();InterlockedIncrement(&zodiac_trace->sequence);
done:
    InterlockedDecrement(&flight);
}
