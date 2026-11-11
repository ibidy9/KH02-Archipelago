/* Fake interface provider for a separate workspace process; not a UE4SS mod. */
#include "../native/bridge_api.h"
static uint32_t queries;
static uint32_t check(void *p,uint64_t *world){(void)p;if(world)*world=0;return 0;}
static uint32_t invoke(void *p,uint32_t op,void *frame,uint32_t size){(void)p;(void)op;(void)frame;(void)size;return 0;}
__declspec(dllexport) uint32_t Kh02UE4SSGetApiV1(uint32_t version,uint32_t size,Kh02UE4SSApi *out) {
 ++queries;
 if(version!=1 || size!=sizeof(*out) || !out)return 0;
 *out=(Kh02UE4SSApi){sizeof(*out),1,sizeof(Kh02UE4SSRequest),7,check,invoke};return 1;
}
__declspec(dllexport) uint32_t FixtureQueries(void){return queries;}
