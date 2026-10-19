/* Read-only bounded broadcast correlation. No reward authorization. */
#include "boss_broadcast_row_v194.h"
static void local_parent193(const BroadcastRow192*);
typedef struct BroadcastTrace192 {
 uint32_t magic,version,size,capacity;
 volatile LONG sequence,count,lock,reserved;
 volatile LONG64 raw,forwarded,dropped,unscoped;
 BroadcastRow192 rows[256];
} BroadcastTrace192;
_Static_assert(sizeof(BroadcastRow192)==88,"broadcast row ABI");
_Static_assert(sizeof(BroadcastTrace192)==22592,"broadcast trace ABI");
static BroadcastTrace192 *broadcast_trace192;
static HANDLE broadcast_mapping192;
static void *broadcast_trampoline192;
/* TLS makes nested broadcasts and non-owner threads independent. */
static _Thread_local BroadcastRow192 *broadcast_scope192;
static const unsigned char broadcast_prefix192[15]={0x40,0x53,0x55,0x41,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x01,0x49,0x8b,0xe9};
static int broadcast_init192(void) {
 wchar_t name[96];
 _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APBroadcastV192-%lu",GetCurrentProcessId());
 broadcast_mapping192=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(BroadcastTrace192),name);
 if(!broadcast_mapping192 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
 broadcast_trace192=MapViewOfFile(broadcast_mapping192,FILE_MAP_ALL_ACCESS,0,0,sizeof(BroadcastTrace192));
 if(!broadcast_trace192)return 0;
 broadcast_trace192->magic=0x32394242;broadcast_trace192->version=1;
 broadcast_trace192->size=sizeof(BroadcastTrace192);broadcast_trace192->capacity=256;return 1;
}
static void broadcast_child192(uint64_t group,uint64_t enemy,uint32_t flags,uint32_t match) {
 BroadcastRow192 *r=broadcast_scope192;
 if(!r){if(broadcast_trace192)InterlockedIncrement64(&broadcast_trace192->unscoped);return;}
 r->children++;
 if(group!=r->group || enemy!=r->enemy)r->mismatch++;
 if((flags&63u)==63u && match)r->matched++;else r->rejected++;
}
static void __fastcall broadcast_observe192(void *receiver,uint64_t group,void *enemy,void *killer) {
 typedef void (__fastcall *Fn)(void*,uint64_t,void*,void*);
 BroadcastRow192 r={0},*previous=broadcast_scope192;
 InterlockedIncrement(&flight);
 r.serial=(uint64_t)InterlockedIncrement64(&broadcast_trace192->raw);
 r.parent=previous?previous->serial:0;r.receiver=(uintptr_t)receiver;r.group=group;
 r.enemy=(uintptr_t)enemy;r.killer=(uintptr_t)killer;r.thread=GetCurrentThreadId();
 if(r.thread==owner_thread){r.cookie=session.gate.current.cookie;r.epoch=Kh02LifecycleEpoch(life);}
 broadcast_scope192=&r;
 ((Fn)broadcast_trampoline192)(receiver,group,enemy,killer);
 broadcast_scope192=previous;
 local_parent193(&r);
 InterlockedIncrement64(&broadcast_trace192->forwarded);
 if(InterlockedCompareExchange(&broadcast_trace192->lock,1,0))InterlockedIncrement64(&broadcast_trace192->dropped);
 else {
  InterlockedIncrement(&broadcast_trace192->sequence);
  if(broadcast_trace192->count<256)broadcast_trace192->rows[broadcast_trace192->count++]=r;
  else InterlockedIncrement64(&broadcast_trace192->dropped);
  MemoryBarrier();InterlockedIncrement(&broadcast_trace192->sequence);InterlockedExchange(&broadcast_trace192->lock,0);
 }
 InterlockedDecrement(&flight);
}
