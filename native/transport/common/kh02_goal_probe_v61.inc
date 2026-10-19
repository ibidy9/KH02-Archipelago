/* Disposable development probe. No arbitrary addresses, names or stat writes.
 * Shared-owner callback only; twelve fixed native Zodiac registrations.
 * It is NOT production goal ownership/victory wiring. Never save this experiment. */
#include <stdio.h>
#include "kh02_cosmetic_snapshot_candidate.h"
typedef struct GoalProbeV61 {
    uint32_t magic,version,size;volatile uint32_t request,response;
    uint32_t command;volatile uint32_t status,entered;
    uint64_t cookie,epoch,owner,attributes;
    uint32_t stats[6],story,result;uint64_t event;
} GoalProbeV61;
_Static_assert(sizeof(GoalProbeV61)==104,"goal probe ABI");
static uint32_t goal_probe_entered, zodiac_failed, zodiac_refresh_mask;
static Kh02CosmeticSnapshot zodiac_before,zodiac_after;
static void goal_probe_process(GoalProbeV61 *b) {
    Kh02SessionSample sample;uint32_t seq,command,story;uint64_t cookie,epoch,event;
    char map[32];static const char hub[]="/Game/Levels/dw/dw_03/dw_03";
    if(!b || GetCurrentThreadId()!=owner_thread)return;
    seq=b->request;if(!seq || seq==b->response)return;
    MemoryBarrier();command=b->command;cookie=b->cookie;epoch=b->epoch;
    b->status=2;b->result=0;b->entered=goal_probe_entered;
    if(b->magic!=0x3638474b || b->version!=1 || b->size!=sizeof(*b) ||
       (command!=1 && command!=2 && command!=3 && command!=4 && (command<0x900 || command>0x90b)) || cookie!=session.gate.current.cookie ||
       epoch!=Kh02LifecycleEpoch(life) || !Kh02ControlledStartAuthorized() ||
       !is_active(NULL) || !session.context || !session.gate.current.observation ||
       InterlockedCompareExchange(&busy,1,0))goto reply;
    if(Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&sample)!=KH02_GATE_OK ||
       sample.context!=session.context || sample.world!=session.world ||
       sample.provider!=session.gate.current.provider || sample.attributes!=session.gate.current.attributes ||
       !read_local(NULL,sample.context+0x3338,&story,4) ||
       !read_local(NULL,sample.context+0x384c,map,sizeof(map)) || (memcmp(map,hub,sizeof(hub)) && memcmp(map,"/Game/Levels/dw/dw_02/dw_02",26) && memcmp(map,"/Game/Levels/dw/dw_09/dw_09",26) && memcmp(map,"/Game/Levels/dw/dw_10/dw_10",26) && memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26) && memcmp(map,"/Game/Levels/dw/dw_11/dw_11",26) && memcmp(map,"/Game/Levels/dw/dw_12/dw_12",26) && memcmp(map,"/Game/Levels/dw/dw_07/dw_07",26)) ||
       !read_local(NULL,sample.attributes+0x50,b->stats,sizeof(b->stats)))goto release;
    b->owner=sample.context;b->attributes=sample.attributes;b->story=story;
    if(command==1){b->status=1;goto release;}
    if(command==3) {
        b->result=50;
        if(boost_enabled || boost_failed || !boost_admit(&sample))goto release;
        if(boost_apply(&sample)){boost_enabled=1;b->status=1;b->result=1;}goto release;
    }
      if(command==4) {
          uint32_t now[3];
          b->result=51;boost_enabled=0;
          if(boost_attributes!=sample.attributes || boost_epoch!=Kh02LifecycleEpoch(life))goto release;
          if(!read_local(NULL,sample.attributes+0x5c,now,12))goto release;
          /* Preserve a newer native recalculation instead of restoring over it. */
          if(boost_pair(sample.attributes,now[0]==999?boost_strength:now[0],now[1]==999?boost_magic:now[1],now[2]==999?boost_defense:now[2])){b->status=1;b->result=1;}goto release;
    }
    if(memcmp(map,hub,sizeof(hub)) && memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26))goto release;
    if(command==2) {
        uint32_t bit;
        b->result=40;
        if(zodiac_failed || (!goal_probe_entered || goal_probe_entered>12))goto release;
        bit=1u<<(goal_probe_entered-1);
        if((zodiac_refresh_mask&bit) || !entries[9].owned ||
           memcmp(entries[9].target,entries[9].patch,entries[9].size))goto release;
        b->event=0;b->result=41;event=start_name(L"UpdataStarRelief",0);
        /* Fixed name may not yet exist before World Within has loaded. */
        if(!event){b->result=42;event=start_name(L"UpdataStarRelief",1);if(!event)goto release;}
        b->result=43;
        if(b->request!=seq || b->cookie!=cookie || b->epoch!=epoch || b->command!=command ||
           epoch!=Kh02LifecycleEpoch(life) || !Kh02ControlledStartAuthorized())goto release;
        zodiac_refresh_mask|=bit;b->event=event;MemoryBarrier();
        b->result=((uint8_t (__fastcall *)(void*,uint64_t))(game.base+0x438870))((void*)(uintptr_t)sample.context,event);
        b->status=1;goto release;
    }
    /* Fixed twelve-relic sequence; no arbitrary item or count. */
    if(zodiac_failed || goal_probe_entered>=12 || command!=0x900+goal_probe_entered)goto release;
    {
        uint64_t vt,fn,manager;uint32_t code,index=0x23a+goal_probe_entered,i;
        unsigned char bytes[24];
        static const wchar_t *keys[12]={L"KEY_ITEM_SIGN_ARIES",L"KEY_ITEM_SIGN_TAURUS",L"KEY_ITEM_SIGN_GEMINI",L"KEY_ITEM_SIGN_CANCER",L"KEY_ITEM_SIGN_LEO",L"KEY_ITEM_SIGN_VIRGO",L"KEY_ITEM_SIGN_LIBRA",L"KEY_ITEM_SIGN_SCORPIO",L"KEY_ITEM_SIGN_SAGITTARIUS",L"KEY_ITEM_SIGN_CAPRICORN",L"KEY_ITEM_SIGN_AQUARIUS",L"KEY_ITEM_SIGN_PISCES"};
        static const unsigned char award[]={0x85,0xd2,0x0f,0x84,0x96,0,0,0,0x48,0x89,0x74,0x24,0x10,0x48,0x89,0x7c,0x24,0x18,0x41,0x56,0x48,0x83,0xec,0x20};
        static const unsigned char lookup[]={0x48,0x83,0x79,0x40,0,0x4c,0x8b,0xd1};
        b->result=10;if(!read_local(NULL,sample.context,&vt,8) || !read_local(NULL,vt+0x428,&fn,8) || fn!=game.base+0x446aa0)goto release;
        b->result=11;if(!read_local(NULL,sample.provider,&vt,8) || !read_local(NULL,vt+0x1500,&fn,8) || fn!=game.base+0x316fe0)goto release;
        b->result=12;if(!read_local(NULL,vt+0x8c0,&fn,8) || fn!=game.base+0x31e450)goto release;
        b->result=13;if(!read_local(NULL,game.base+0x446aa0,bytes,sizeof(award)) || memcmp(bytes,award,sizeof(award)))goto release;
        b->result=14;if(!read_local(NULL,game.base+0x4841f0,bytes,sizeof(lookup)) || memcmp(bytes,lookup,sizeof(lookup)))goto release;
        b->result=15;if(!read_local(NULL,sample.context+0x7e08,&manager,8) || manager<0x10000 || manager>0x7fffffffffffULL-0x200 || (manager&7))goto release;
        b->result=16;if(Kh02ReadCosmeticSnapshot(read_local,NULL,sample.world,&zodiac_before)!=KH02_GATE_OK || zodiac_before.owner!=sample.context)goto release;
        b->result=17;if(zodiac_before.primary.items[index][0] || zodiac_before.primary.items[index][1])goto release;
        b->result=18;if(zodiac_before.reference.items[index][0] || zodiac_before.reference.items[index][1])goto release;
        b->result=25;event=start_name(keys[goal_probe_entered],0);if(!event)goto release;
        code=((uint32_t (__fastcall *)(void*,uint64_t))(game.base+0x4841f0))((void*)(uintptr_t)manager,event);
        b->result=26;b->event=code;
        if(code!=command || b->request!=seq || b->cookie!=cookie || b->epoch!=epoch || b->command!=command ||
           epoch!=Kh02LifecycleEpoch(life) || !Kh02ControlledStartAuthorized())goto release;
        /* Consume before calling: an uncertain award is never retried. */
        zodiac_failed=1;goal_probe_entered++;b->entered=goal_probe_entered;b->event=code;MemoryBarrier();
        ((void (__fastcall *)(void*,uint32_t,int32_t,unsigned char,uint32_t))(game.base+0x446aa0))(
            (void*)(uintptr_t)sample.context,code,1,1,0);
        b->result=30;
        if(epoch!=Kh02LifecycleEpoch(life) || !Kh02ControlledStartAuthorized() ||
           Kh02ReadCosmeticSnapshot(read_local,NULL,sample.world,&zodiac_after)!=KH02_GATE_OK ||
           zodiac_after.owner!=zodiac_before.owner || memcmp(&zodiac_before.reference,&zodiac_after.reference,sizeof(zodiac_before.reference)))goto release;
        b->result=31;
        for(i=0;i<KH02_NATIVE_ITEM_RECORDS;i++) {
            if(i==index) {if(zodiac_after.primary.items[i][0]!=1 || zodiac_after.primary.items[i][1]!=3)goto release;}
            else if(memcmp(zodiac_before.primary.items[i],zodiac_after.primary.items[i],8))goto release;
        }
        zodiac_failed=0;b->result=1;b->status=1;
    }

release:
    InterlockedExchange(&busy,0);
reply:
    MemoryBarrier();b->response=seq;
}
static void Kh02GoalProbeService(void) {
    wchar_t name[96];HANDLE mapping;GoalProbeV61 *b;
    if(GetCurrentThreadId()!=owner_thread || !active_flag || fault)return;
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APZodiacProbeV68-%lu",GetCurrentProcessId());
    mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);if(!mapping)return;
    b=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(*b));
    if(b){goal_probe_process(b);UnmapViewOfFile(b);}CloseHandle(mapping);
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02GoalProbeTestProcess(GoalProbeV61 *b){goal_probe_process(b);}
#endif
