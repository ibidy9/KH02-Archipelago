/* Bounded, read-only native confirmation observer. No UI or travel writes. */
typedef struct ConfirmRowV86 {
    uint64_t object,owner,epoch,cookie,caller;
    uint32_t kind,arg1,arg2,flags,event,value,selected;
    char map[32];uint16_t message[128],button0[128],button1[128];
} ConfirmRowV86;
typedef struct ConfirmTraceV86 {
    uint32_t magic,version,size,capacity;volatile LONG sequence,count;
    volatile LONG64 raw,forwarded,wrong_thread,rejects,overflow;
    ConfirmRowV86 rows[64];
} ConfirmTraceV86;
_Static_assert(sizeof(ConfirmRowV86)==872,"confirm row ABI");
_Static_assert(sizeof(ConfirmTraceV86)==55872,"confirm trace ABI");
static ConfirmTraceV86 *confirm_trace;
static HANDLE confirm_mapping;
static void *confirm_open_trampoline,*confirm_event_trampoline,*confirm_phase_trampoline;
static const unsigned char confirm_open_prefix[14]={0x44,0x89,0x44,0x24,0x18,0x89,0x54,0x24,0x10,0x48,0x89,0x4c,0x24,0x08};
static const unsigned char confirm_event_prefix[15]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x70};
static const unsigned char confirm_phase_prefix[15]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20};
static int confirm_init_v86(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APConfirmV86-%lu",GetCurrentProcessId());
    confirm_mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(ConfirmTraceV86),name);
    if(!confirm_mapping || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    confirm_trace=MapViewOfFile(confirm_mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(ConfirmTraceV86));
    if(!confirm_trace)return 0;
    confirm_trace->magic=0x3836434b;confirm_trace->version=1;
    confirm_trace->size=sizeof(ConfirmTraceV86);confirm_trace->capacity=64;return 1;
}
static int confirm_text_v86(uint64_t address,uint16_t out[128]) {
    struct {uint64_t data;int32_t count,capacity;} s;
    if(!zodiac_read(address,&s,sizeof(s)) || s.count<0 || s.count>128 || s.capacity<s.count)return 0;
    if(!s.count)return 1;
    if(!zodiac_read(s.data,out,(uint32_t)s.count*2) || out[s.count-1]) {
        memset(out,0,256);return 0;
    }
    return 1;
}
static int confirm_begin_v86(ConfirmRowV86 *r,void *object,uint32_t kind,uintptr_t caller) {
    uint64_t table;
    if(!confirm_trace)return 0;
    InterlockedIncrement64(&confirm_trace->raw);
    if(GetCurrentThreadId()!=owner_thread){InterlockedIncrement64(&confirm_trace->wrong_thread);return 0;}
    r->kind=kind;r->object=(uintptr_t)object;r->caller=caller-game.base;
    r->flags=1;r->owner=session.context;r->epoch=Kh02LifecycleEpoch(life);r->cookie=session.gate.current.cookie;
    if(!zodiac_read(r->object,&table,8) || table!=game.base+0x3788c60) {
        InterlockedIncrement64(&confirm_trace->rejects);return 0;
    }
    r->flags|=2;
    if(r->owner && zodiac_read(r->owner+0x384c,r->map,sizeof(r->map)))r->flags|=32;
    if(!zodiac_read(r->object+0x230,&r->selected,4))r->selected=0x80000000;
    return 1;
}
static void confirm_record_v86(const ConfirmRowV86 *r) {
    InterlockedIncrement(&confirm_trace->sequence);
    if(confirm_trace->count<64) {
        confirm_trace->rows[confirm_trace->count]=*r;InterlockedIncrement(&confirm_trace->count);
    } else InterlockedIncrement64(&confirm_trace->overflow);
    MemoryBarrier();InterlockedIncrement(&confirm_trace->sequence);
}
static unsigned char __fastcall confirm_open_v86(void *object,uint32_t bg,uint32_t title,void *message,void *button0,void *button1) {
    typedef unsigned char (__fastcall *Fn)(void*,uint32_t,uint32_t,void*,void*,void*);
    ConfirmRowV86 r={0};unsigned char result;
    int custom=tide_choice_customize(object,message,&button0,&button1,(uintptr_t)__builtin_return_address(0));
    InterlockedIncrement(&flight);
    if(confirm_begin_v86(&r,object,1,(uintptr_t)__builtin_return_address(0))) {
        r.arg1=bg;r.arg2=title;
        if(confirm_text_v86((uintptr_t)message,r.message))r.flags|=4;
        if(confirm_text_v86((uintptr_t)button0,r.button0))r.flags|=8;
        if(confirm_text_v86((uintptr_t)button1,r.button1))r.flags|=16;
        confirm_record_v86(&r);
    }
    result=((Fn)confirm_open_trampoline)(object,bg,title,message,button0,button1);
    if(custom)tide_choice_configured(object,result);
    if(confirm_trace)InterlockedIncrement64(&confirm_trace->forwarded);
    InterlockedDecrement(&flight);return result;
}
static unsigned char __fastcall confirm_event_v86(void *object,void *event) {
    typedef unsigned char (__fastcall *Fn)(void*,void*);
    ConfirmRowV86 r={0};unsigned char result;uint32_t words[3];
    InterlockedIncrement(&flight);
    if(confirm_begin_v86(&r,object,2,(uintptr_t)__builtin_return_address(0))) {
        if(zodiac_read((uintptr_t)event+0x10,words,sizeof(words))) {
            r.event=words[0];r.value=words[2];
            tide_choice_event(object,r.event,r.value);
            if(r.event==5 || r.event==7 || r.event==8)confirm_record_v86(&r);
        } else InterlockedIncrement64(&confirm_trace->rejects);
    }
    result=((Fn)confirm_event_trampoline)(object,event);
    if(confirm_trace)InterlockedIncrement64(&confirm_trace->forwarded);
    InterlockedDecrement(&flight);return result;
}
static uint32_t __fastcall confirm_phase_v86(void *object,uint32_t phase) {
    typedef uint32_t (__fastcall *Fn)(void*,uint32_t);
    ConfirmRowV86 r={0};uint32_t result;
    InterlockedIncrement(&flight);
    if(confirm_begin_v86(&r,object,3,(uintptr_t)__builtin_return_address(0))) {
        r.arg1=phase;if(phase<=1)confirm_record_v86(&r);
    }
    result=((Fn)confirm_phase_trampoline)(object,phase);
    if(r.flags&2)tide_choice_closed(object,phase,r.selected);
    if(confirm_trace)InterlockedIncrement64(&confirm_trace->forwarded);
    InterlockedDecrement(&flight);return result;
}
