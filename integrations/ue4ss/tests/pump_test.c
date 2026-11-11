#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../native/sdk_pump_api.h"
#include "../native/sdk_pump_policy.h"
#define MEMBER_API
static uint32_t thread=7;
static uint32_t GetCurrentThreadId(void){return thread;}
static uint32_t GetCurrentProcessId(void){return 99;}
static long InterlockedCompareExchange(volatile long* p,long to,long expected){long old=*p;if(old==expected)*p=to;return old;}
struct Block {struct {uint32_t request_sequence,status;}transport;};
struct Router {volatile long closed;};
static struct Block block,*fresh_map;
static struct Router router,*harness_router;
static uint64_t fresh_cookie;
static uint32_t fresh_thread,fresh_served,sdk_setup_ok,effects;
static volatile long fresh_servicing;
static Kh02SdkDispatch sdk_dispatch;
static void fresh_service(struct Block* b){if(b->transport.request_sequence>fresh_served){fresh_served=b->transport.request_sequence;b->transport.status=1;++effects;}}
#include "../native/sdk_core_pump.inc"
int main(void){
 Kh02SdkPumpStatus out;_Static_assert(sizeof(out)==48,"pump ABI");
 assert(!Kh02SdkOwnerPump(2,sizeof(out),&out));assert(!Kh02SdkOwnerPump(1,47,&out));assert(!Kh02SdkOwnerPump(1,48,0));
 assert(!Kh02SdkOwnerPump(1,48,&out));
 fresh_map=&block;harness_router=&router;fresh_cookie=10;fresh_thread=7;
 assert(!Kh02SdkOwnerPump(1,48,&out));sdk_setup_ok=1;
 thread=8;assert(!Kh02SdkOwnerPump(1,48,&out));thread=7;
 fresh_servicing=1;assert(!Kh02SdkOwnerPump(1,48,&out));fresh_servicing=0;
 router.closed=1;assert(!Kh02SdkOwnerPump(1,48,&out));router.closed=0;
 assert(Kh02SdkWindowAllowed(&sdk_dispatch,1));assert(Kh02SdkWindowAllowed(&sdk_dispatch,2));assert(!Kh02SdkWindowAllowed(&sdk_dispatch,5));
 block.transport.request_sequence=3;
 assert(Kh02SdkOwnerPump(1,48,&out)==1 && out.bound==1 && out.sequence==3 && out.sdk_commands==1 && effects==1);
 assert(out.pid==99 && out.thread==7 && out.size==48 && out.version==1);
 assert(!Kh02SdkWindowAllowed(&sdk_dispatch,1) && !Kh02SdkWindowAllowed(&sdk_dispatch,2) && !Kh02SdkWindowAllowed(&sdk_dispatch,5));
 assert(Kh02SdkOwnerPump(1,48,&out)==1 && out.sdk_commands==1 && effects==1);
 block.transport.request_sequence=4;assert(Kh02SdkOwnerPump(1,48,&out)==1 && effects==2);
 router.closed=1;block.transport.request_sequence=5;assert(Kh02SdkOwnerPump(1,48,&out)==1 && effects==3); // cleanup remains serviceable after binding
 puts("PASS SDK pump: ABI, thread/setup/reentry/closed-before-bind guards, no legacy receipt fallback, handoff, duplicate sequence, cleanup");
 return 0;
}
