/* Read-only game-state observation inside the existing owner callbacks.
 * The only writes are to our diagnostic mapping. No hooks, scans or routing. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "kh02_opening_trace_v54.h"
#define CAPACITY 128u
typedef struct State {uint64_t address;uint32_t readable,story;char map[64];} State;
typedef struct Event {
    uint32_t kind,thread;uint64_t caller,owner,map,spawn;
    uint32_t fade,reserved;State source,destination;
} Event;
typedef struct Trace {
    uint32_t magic,version,size,capacity;volatile LONG count,overflow,busy,reserved;
    Event events[CAPACITY];
} Trace;
static Trace *trace;
static HANDLE mapping;
static volatile LONG attempted;
static int read_exact(uintptr_t p,void *out,SIZE_T size) {
    SIZE_T got=0;
    return p>=0x10000 && p<0x0000800000000000ULL-size &&
        ReadProcessMemory(GetCurrentProcess(),(void*)p,out,size,&got) && got==size;
}
static void state(State *out,uintptr_t address) {
    out->address=address;
    if(address>0x0000800000000000ULL-0x3d28)return;
    if(!read_exact(address+0x3000,&out->story,4) ||
       !read_exact(address+0x3514,out->map,sizeof(out->map)))return;
    out->readable=1;
}
static Event *begin(uint32_t kind,uintptr_t base,uintptr_t caller) {
    wchar_t name[96];LONG count;
    if(!trace) {
        if(InterlockedCompareExchange(&attempted,1,0))return NULL;
        _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APOpeningTrace-%lu",GetCurrentProcessId());
        mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(Trace),name);
        if(!mapping || GetLastError()==ERROR_ALREADY_EXISTS)return NULL;
        trace=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Trace));
        if(!trace)return NULL;
        trace->magic=0x5434354b;trace->version=1;trace->size=sizeof(Trace);trace->capacity=CAPACITY;
    }
    if(InterlockedCompareExchange(&trace->busy,1,0)) {
        InterlockedIncrement(&trace->overflow);return NULL;
    }
    count=trace->count;
    if(count<0 || (uint32_t)count>=CAPACITY) {
        InterlockedIncrement(&trace->overflow);InterlockedExchange(&trace->busy,0);return NULL;
    }
    Event *event=&trace->events[count];memset(event,0,sizeof(*event));
    event->kind=kind;event->thread=GetCurrentThreadId();
    event->caller=caller>=base?caller-base:UINT64_MAX;
    return event;
}
static void end(void) {
    MemoryBarrier();InterlockedIncrement(&trace->count);InterlockedExchange(&trace->busy,0);
}
/* v56 diagnostic only: kind4, map=plan phase, fade=reason. Same fixed ring ABI. */
void Kh02OpeningTraceStart(uintptr_t base,uintptr_t caller,uintptr_t owner,uint32_t phase,uint32_t reason) {
    Event *e=begin(4,base,caller);if(!e)return;
    e->owner=owner;e->map=phase;e->fade=reason;
    if(owner && owner<0x0000800000000000ULL-0x8000) {
        state(&e->source,owner+0x338);state(&e->destination,owner+0x4060);
    }
    end();
}
/* kind5: map=remote event, spawn=broadcast context, fade=reason. */
void Kh02OpeningTraceRemote(uintptr_t base,uintptr_t owner,uintptr_t context,uint64_t event,uint32_t reason) {
    Event *e=begin(5,base,base+0x438870);if(!e)return;
    e->owner=owner;e->map=event;e->spawn=context;e->fade=reason;
    if(owner && owner<0x0000800000000000ULL-0x8000) {
        state(&e->source,owner+0x338);state(&e->destination,owner+0x4060);
    }
    end();
}
void Kh02OpeningTraceCopy(uint32_t phase,uintptr_t base,uintptr_t caller,
                          uintptr_t destination,uintptr_t source) {
    Event *e=begin(phase,base,caller);if(!e)return;
    state(&e->source,source);state(&e->destination,destination);end();
}
void Kh02OpeningTraceRoute(uintptr_t base,uintptr_t caller,uintptr_t owner,
                           uint64_t map,uint64_t spawn,uint32_t fade) {
    Event *e=begin(3,base,caller);if(!e)return;
    e->owner=owner;e->map=map;e->spawn=spawn;e->fade=fade;
    if(owner<0x0000800000000000ULL-0x8000) {
        state(&e->source,owner+0x338);state(&e->destination,owner+0x4060);
    }
    end();
}

/* kind6: original request, before goal refusal or start adaptation. Diagnostic
 * only: raw receiver is not authenticated AP completion evidence. */
void Kh02OpeningTraceOriginal(uintptr_t base,uintptr_t caller,const uint64_t *frame) {
    Event *e;if(!frame)return;
    e=begin(6,base,caller);if(!e)return;
    e->owner=frame[0];e->map=frame[1];e->spawn=frame[2];e->fade=(uint32_t)frame[3];
    if(e->owner>=0x10000 && e->owner<0x0000800000000000ULL-0x8000) {
        state(&e->source,e->owner+0x338);state(&e->destination,e->owner+0x4060);
    }
    end();
}
