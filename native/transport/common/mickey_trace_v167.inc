/* Development observation only: exact SetFriend call, unchanged arguments/result. */
#include <stdio.h>
static int zodiac_name(uint64_t,char[128],uint64_t*);
typedef struct MickeyRow167 {
    uint64_t manager,klass,caller,player,owner,epoch;
    uint32_t slot,thread,reason,before,after,result;
    char map[32];
    char class_name[128];
} MickeyRow167;
typedef struct MickeyTrace167 {
    uint32_t magic,version,size,capacity;
    volatile LONG sequence,count,lock,reserved;
    volatile LONG64 raw,forwarded,accepted,dropped;
    MickeyRow167 rows[64];
} MickeyTrace167;
_Static_assert(sizeof(MickeyRow167)==232,"Mickey row ABI");
_Static_assert(sizeof(MickeyTrace167)==14912,"Mickey trace ABI");
static MickeyTrace167 *mickey_trace167;
static HANDLE mickey_mapping167;
static void *mickey_trampoline167;
/* 15 bytes, complete instructions, no RIP-relative operand or branch. */
static const unsigned char mickey_prefix167[15]={
    0x4c,0x89,0x44,0x24,0x18,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56};
static int mickey_init167(void) {
    wchar_t name[96];
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APMickeyTraceV167-%lu",GetCurrentProcessId());
    mickey_mapping167=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(MickeyTrace167),name);
    if(!mickey_mapping167 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
    mickey_trace167=MapViewOfFile(mickey_mapping167,FILE_MAP_ALL_ACCESS,0,0,sizeof(MickeyTrace167));
    if(!mickey_trace167)return 0;
    mickey_trace167->magic=0x37364b4d;mickey_trace167->version=1;
    mickey_trace167->size=sizeof(MickeyTrace167);mickey_trace167->capacity=64;
    return 1;
}
static int mickey_ptr167(uint64_t p){return p>=0x10000 && p<0x7fffffff0000ULL && !(p&7);}
static void mickey_before167(MickeyRow167 *r) {
    uint64_t root=0,instance=0,table=0,controller=0,manager=0,fn=0;
    r->before=r->after=UINT32_MAX;r->reason=1;
    if(r->thread!=owner_thread)return;
    r->reason=2;
    if(!active_flag || fault || !Kh02ControlledStartAuthorized())return;
    r->epoch=Kh02LifecycleEpoch(life);r->reason=3;
    if(!read_local(NULL,game.base+0x480b5d0,&root,8) || !mickey_ptr167(root) ||
       !read_local(NULL,root+0x230,&instance,8) || !mickey_ptr167(instance) ||
       !read_local(NULL,instance+0x50,&r->owner,8) || !mickey_ptr167(r->owner) ||
       !read_local(NULL,r->owner,&table,8) || table!=game.base+0x36df228)return;
    if(!read_local(NULL,r->owner+0x384c,r->map,32))return;
    r->reason=4;
    if(!mickey_ptr167(r->manager) ||
       !read_local(NULL,r->manager+0x210,&controller,8) || !mickey_ptr167(controller) ||
       !read_local(NULL,controller+0x3e8,&r->player,8) || !mickey_ptr167(r->player) ||
       !read_local(NULL,r->player,&table,8) || !mickey_ptr167(table) ||
       !read_local(NULL,table+0x19e8,&fn,8) || fn!=game.base+0x33ec50 ||
       !read_local(NULL,r->player+0x9e8,&manager,8) || manager!=r->manager ||
       !read_local(NULL,r->manager+0x238,&r->before,4) || r->before>4)return;
    r->reason=5;

}
#include "mickey_control_v168.inc"
static unsigned char __fastcall mickey_observe167(void *manager,int slot,void *klass) {
    typedef unsigned char (__fastcall *Fn)(void*,int,void*);
    MickeyRow167 r={0};unsigned char result;
    r.manager=(uintptr_t)manager;r.klass=(uintptr_t)klass;r.slot=(uint32_t)slot;
    r.thread=GetCurrentThreadId();r.caller=(uintptr_t)__builtin_return_address(0);
    InterlockedIncrement(&flight);
    if(mickey_trace167)InterlockedIncrement64(&mickey_trace167->raw);
    mickey_before167(&r);
    if(mickey_block168(&r))result=0;
    else {result=((Fn)mickey_trampoline167)(manager,slot,klass);
        if(mickey_trace167)InterlockedIncrement64(&mickey_trace167->forwarded);}
    if(r.klass && r.reason==5)mickey_fname168(r.klass,r.class_name);
    r.result=result;
    if((r.reason==5 || r.reason==6) && !read_local(NULL,r.manager+0x238,&r.after,4))r.after=UINT32_MAX;
    if(mickey_trace167) {
        if(r.reason==5 || r.reason==6)InterlockedIncrement64(&mickey_trace167->accepted);
        if(InterlockedCompareExchange(&mickey_trace167->lock,1,0))
            InterlockedIncrement64(&mickey_trace167->dropped);
        else {
            InterlockedIncrement(&mickey_trace167->sequence);
            if(mickey_trace167->count<64)mickey_trace167->rows[mickey_trace167->count++]=r;
            else InterlockedIncrement64(&mickey_trace167->dropped);
            MemoryBarrier();InterlockedIncrement(&mickey_trace167->sequence);
            InterlockedExchange(&mickey_trace167->lock,0);
        }
    }
    InterlockedDecrement(&flight);return result;
}
