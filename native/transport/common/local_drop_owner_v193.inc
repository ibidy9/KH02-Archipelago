/* Test candidate: optional Forest Darksides only. Existing AP masks untouched. */
#include "local_drop_policy_v193.h"
typedef struct LocalDropRow193 {
 LocalDropEvent193 event;
 uint32_t status,entered,before,after;
} LocalDropRow193;
typedef struct LocalDropTrace193 {
 uint32_t magic,version,size,capacity;
 volatile LONG sequence,count,lock,reserved;
 uint64_t accepted,duplicates,expired,dropped;
 LocalDropRow193 rows[64];
} LocalDropTrace193;
_Static_assert(sizeof(LocalDropRow193)==48,"local row ABI");
_Static_assert(sizeof(LocalDropTrace193)==3136,"local trace ABI");
static LocalDropTrace193 *local_trace193;
static HANDLE local_mapping193;
static LocalDropPolicy193 local_policy193;
/* Preserve a pending reward only through repeated calls for an already
 * admitted scripted hop. This is NOT grant authority; normal readmission is
 * still required by local_service193. Never persists across process/load. */
static struct {
 uint64_t cookie,epoch,owner,callback,map,spawn,caller;
 uint32_t stage,group,fade;
} local_hop202;
static int local_init193(void) {
 wchar_t name[96];
 _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APLocalDropsV193-%lu",GetCurrentProcessId());
 local_mapping193=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(LocalDropTrace193),name);
 if(!local_mapping193 || GetLastError()==ERROR_ALREADY_EXISTS)return 0;
 local_trace193=MapViewOfFile(local_mapping193,FILE_MAP_ALL_ACCESS,0,0,sizeof(LocalDropTrace193));
 if(!local_trace193)return 0;
 local_trace193->magic=0x3339444c;local_trace193->version=1;
 local_trace193->size=sizeof(LocalDropTrace193);local_trace193->capacity=64;return 1;
}
static void local_log193(const LocalDropEvent193 *event,uint32_t status,const Kh02ReceiptGrant *packet) {
 LocalDropRow193 row={0};if(!local_trace193)return;
 row.event=*event;row.status=status;row.before=row.after=UINT32_MAX;
 if(packet){row.entered=packet->entered;row.before=packet->observed_before;row.after=packet->observed_after;}
 InterlockedIncrement(&local_trace193->sequence);
 local_trace193->accepted=local_policy193.accepted;local_trace193->duplicates=local_policy193.duplicates;
 local_trace193->expired=local_policy193.expired;
 if(local_trace193->count<64)local_trace193->rows[local_trace193->count++]=row;else local_trace193->dropped++;
 MemoryBarrier();InterlockedIncrement(&local_trace193->sequence);
}
static int local_source198(uint64_t context,uint32_t group) {
 static const unsigned maps[10]={8,8,8,2,7,8,8,9,10,7};
 uint64_t root=0,instance=0,current=0,table=0;char map[32],expected[32];unsigned index=0;
 if(!group || group>8192 || (group&(group-1)))return 0;
 while((1u<<index)!=group)index++;
 if(index<3)return darkside_source_v144(context);
 if(index>=10) {
  uint32_t stage,again;unsigned expected_map;
  if(!context || !read_local(NULL,context+0x3340,&stage,4) || stage>40 || stage%10 ||
     stage<(index-10)*10 || !read_local(NULL,context+0x3340,&again,4) || stage!=again)return 0;
  expected_map=stage==40?7:stage==0||stage==30?11:12;
  _snprintf_s(expected,sizeof(expected),_TRUNCATE,"/Game/Levels/dw/dw_%02u/dw_%02u",expected_map,expected_map);
 } else
 _snprintf_s(expected,sizeof(expected),_TRUNCATE,"/Game/Levels/dw/dw_%02u/dw_%02u",maps[index],maps[index]);
 return context && !(context&7) && read_local(NULL,game.base+0x480b5d0,&root,8) && root &&
  read_local(NULL,root+0x230,&instance,8) && instance &&
  read_local(NULL,instance+0x50,&current,8) && current==context &&
  read_local(NULL,context,&table,8) && table==game.base+0x36df228 &&
  read_local(NULL,context+0x384c,map,sizeof(map)) && (!memcmp(map,expected,26) || (index>=10 &&
    ((index==10 && !memcmp(map,"/Game/Levels/dw/dw_11/dw_11",26)) ||
     (index==11 && !memcmp(map,"/Game/Levels/dw/dw_12/dw_12",26)) ||
     (index==12 && !memcmp(map,"/Game/Levels/dw/dw_12/dw_12",26)) ||
     (index==13 && !memcmp(map,"/Game/Levels/dw/dw_11/dw_11",26)))));
}
static int local_authorized193(uint64_t cookie,uint64_t epoch,uint32_t group) {
 Kh02ReceiptOwner *receipt=Kh02LifecycleCurrent(life);
 return GetCurrentThreadId()==owner_thread && active_flag && !fault && !busy &&
  cookie && cookie==session.gate.current.cookie && epoch && epoch==seen_epoch &&
  epoch==Kh02LifecycleEpoch(life) && !session.stopped && !session.uncertain && !session.gate.busy &&
  receipt && Kh02ControlledStartAuthorized() && Kh02ReceiptOwnerCheck(receipt)==KH02_MEMBER_OK &&
  local_source198(session.context,group);
}
static void local_child193(const WipeoutRowV143 *r) {
 BroadcastRow192 *scope=broadcast_scope192;
 /* darkside_complete_v144 just ran for THIS child. Its reason resets to1
  * before authority checks;4 means both existing bounded checks succeeded. */
 if(GetCurrentThreadId()!=owner_thread || !scope ||
  (r->match!=1 && r->match!=2 && r->match!=4) || darkside_reason_v144!=4 ||
  r->group!=scope->group || r->enemy!=scope->enemy || r->epoch!=scope->epoch ||
  (r->flags&63u)!=63u || r->context!=session.context ||
  !local_authorized193(scope->cookie,scope->epoch,r->match))return;
 scope->reserved|=r->match; /* v193: validated group mask, same bounded row ABI. */
}
static void local_parent193(const BroadcastRow192 *r) {
 if(!local_authorized193(r->cookie,r->epoch,r->reserved))return;
 if(local_drop_admit193(&local_policy193,r->cookie,r->epoch,r->serial,r->reserved))
  local_log193(&local_policy193.pending[local_policy193.count-1],1,NULL);
}
/* Bounded read-only cinematic snapshots for popup timing; no new hook.
 * status100=sample,101=read/identity unavailable. before=flags, after=rush stage.
 * Observe only a delivered rush reward, at most 16 changes per process. */
static LocalDropEvent193 popup_observed206;
static uint64_t popup_context206;
static uint32_t popup_count206,popup_flags206,popup_stage206,popup_valid206;
static void popup_observe206(void) {
 Kh02ReceiptGrant sample={0};uint64_t vt=0,fn=0;uint32_t flags=0,stage=0;
 unsigned char code[11];
 static const unsigned char expected[]={0x8b,0x81,0x48,0x03,0,0,0xd1,0xe8,0x24,1,0xc3};
 if(!popup_observed206.serial || popup_count206>=16 || GetCurrentThreadId()!=owner_thread)return;
 if(popup_observed206.epoch!=Kh02LifecycleEpoch(life) ||
    popup_observed206.cookie!=session.gate.current.cookie){popup_observed206.serial=0;return;}
 int valid=session.context && read_local(NULL,session.context,&vt,8) && vt==game.base+0x36df228 &&
  read_local(NULL,vt+0x270,&fn,8) && fn==game.base+0x44f2f0 &&
  read_local(NULL,fn,code,11) && !memcmp(code,expected,11) &&
  read_local(NULL,session.context+0x348,&flags,4) &&
  read_local(NULL,session.context+0x3340,&stage,4);
 if(popup_count206 && popup_valid206==(uint32_t)valid &&
    popup_context206==session.context && popup_flags206==flags && popup_stage206==stage)return;
 popup_count206++;popup_valid206=valid;popup_context206=session.context;
 popup_flags206=flags;popup_stage206=stage;
 sample.observed_before=flags;sample.observed_after=stage;
 local_log193(&popup_observed206,valid?100:101,&sample);
}
/* Delay rush grant+native popup together until the observed cinematic ends.
 * Read-only exact-build predicate, no timer or extra grant/notification path. */
static int local_popup_ready207(void) {
 return popup_ready273(read_local,NULL,game.base,session.context);
}
static void local_service193(void) {
 Kh02ReceiptGrant packet;Kh02SessionSample sample;LocalDropEvent193 event;uint32_t result;
 uint64_t cookie=session.gate.current.cookie,epoch=Kh02LifecycleEpoch(life);
 if(GetCurrentThreadId()!=owner_thread || busy || session.gate.busy)return;
 popup_observe206();
 if(local_policy193.count && (cookie!=local_policy193.cookie || epoch!=local_policy193.epoch)) {
  while(local_drop_take193(&local_policy193,&event)){local_policy193.expired++;local_log193(&event,4,NULL);}
 }
 if(!local_policy193.count || !local_authorized193(cookie,epoch,local_policy193.pending[0].group))return;
 if(!local_popup_ready207())return;
 if(InterlockedCompareExchange(&busy,1,0))return;
 if(Kh02GameGrantPrepare(&game,Kh02LifecycleCurrent(life),1,&packet,&sample)!=KH02_MEMBER_OK)goto done;
 /* Preparation can observe a new owner: never carry an event across it. */
 if(cookie!=session.gate.current.cookie || epoch!=Kh02LifecycleEpoch(life) || epoch!=seen_epoch)goto done;
 memset(&local_hop202,0,sizeof(local_hop202));
 if(!local_drop_take193(&local_policy193,&event))goto done; /* consumed BEFORE native execution */
 if(packet.before.counts[event.battle]>=99){
  packet.observed_before=packet.observed_after=packet.before.counts[event.battle];local_log193(&event,3,&packet);goto done;
 }
 result=Kh02LifecycleLocalGrant193(life,event.epoch,event.battle,&packet);
 local_log193(&event,result?2u:(packet.entered?6u:5u),&packet);
 if(result && event.group>=1024){popup_observed206=event;popup_observe206();}
done:
 InterlockedExchange(&busy,0);
}

/* Invoked only at the existing owner-thread native travel capture, after the
 * proven transition guard accepted the call. Does not award or touch AP state. */
static void local_travel195(void) {
 LocalDropEvent193 event={0};
 if(GetCurrentThreadId()!=owner_thread)return;
 memset(&local_hop202,0,sizeof(local_hop202));
 event.cookie=session.gate.current.cookie;event.epoch=Kh02LifecycleEpoch(life);
 local_drop_travel195(&local_policy193);
 local_log193(&event,7,NULL);
}

/* Nested game callbacks may queue, never execute a grant. Owner/source/epoch checks
 * remain mandatory; the authenticated dispatcher alone performs delivery.
 * Only a successfully suppressed named native award, never a stat rebuild.
 * Registered names are read twice; no fixed runtime FName IDs.
 * Keeps original suppression and AP check flags unchanged. */
static uint64_t local_named_serial198;
static void local_named198(const StatTraceRowV108 *r) {
 uint64_t names[7],again[7],cookie=session.gate.current.cookie,epoch=Kh02LifecycleEpoch(life);
 unsigned index=0;uint32_t group;
 if(r->kind!=1 || !(r->flags&256u) || r->objective!=UINT32_MAX || !r->name ||
    r->thread!=owner_thread || GetCurrentThreadId()!=owner_thread || flight<1 ||
    r->valid_before!=7 || r->valid_after!=7 || r->context!=session.context ||
    r->subrecord!=r->context+0x2930 || r->epoch_before!=epoch || r->epoch_after!=epoch ||
    !read_local(NULL,game.base+0x45342b0+42*8,names,sizeof(names)))return;
 for(unsigned i=0;i<7;i++) {
  if(!names[i])return;
  for(unsigned j=0;j<i;j++)if(names[i]==names[j])return;
 }
 while(index<7 && names[index]!=r->name)index++;
 if(index==7)return;
 group=1u<<(index+3);
 if(!local_authorized193(cookie,epoch,group) ||
    !read_local(NULL,game.base+0x45342b0+42*8,again,sizeof(again)) ||
    memcmp(names,again,sizeof(names)) || !local_authorized193(cookie,epoch,group))return;
 if(local_named_serial198==UINT64_MAX)return;
 if(local_drop_admit193(&local_policy193,cookie,epoch,++local_named_serial198,group))
  local_log193(&local_policy193.pending[local_policy193.count-1],1,NULL);
}

static uint64_t local_rush_serial200;
static void local_rush200(uint64_t cookie,uint64_t epoch,unsigned round) {
 uint32_t group;
 if(round<1 || round>4)return;
 group=1u<<(round+9);
 if(!local_authorized193(cookie,epoch,group) || local_rush_serial200==UINT64_MAX)return;
 if(local_drop_admit193(&local_policy193,cookie,epoch,++local_rush_serial200,group))
  local_log193(&local_policy193.pending[local_policy193.count-1],1,NULL);
}
/* Only exact scripted arena hops preserve rush rewards. No epoch/load carry. */
static int local_rush_transition200(uint64_t callback,uint64_t map,uint64_t spawn,uint32_t fade,uint64_t caller) {
 uint32_t stage,again,bit;unsigned destination;wchar_t mapname[64],spawnname[64];
 uint64_t cookie=session.gate.current.cookie,epoch=Kh02LifecycleEpoch(life);
 /* SessionTravel deliberately clears session.context at the first callback.
  * Do not mistake an identical nested callback for an unrelated teleport.
  * Revalidate the captured owner through the fixed native root/source chain;
  * no delivery is permitted using this captured identity. */
 if(!session.context && local_policy193.count && local_hop202.owner &&
    GetCurrentThreadId()==owner_thread && active_flag && !fault && !busy &&
    !session.stopped && !session.uncertain && !session.gate.busy &&
    cookie && cookie==local_hop202.cookie && cookie==local_policy193.cookie &&
    epoch && epoch==local_hop202.epoch && epoch==local_policy193.epoch && epoch==seen_epoch &&
    callback==local_hop202.callback && map==local_hop202.map &&
    spawn==local_hop202.spawn &&
    (fade==local_hop202.fade ||
     (local_hop202.caller==game.base+0x43eca3 && caller==game.base+0x458996 &&
      local_hop202.fade==2 && fade==0)) &&
    (local_policy193.mask&local_hop202.group) &&
    local_source198(local_hop202.owner,local_hop202.group) &&
    read_local(NULL,local_hop202.owner+0x3340,&stage,4) && stage==local_hop202.stage &&
    read_local(NULL,local_hop202.owner+0x3340,&again,4) && stage==again) {
     /* Consume the evidenced fade continuation; never allow 0 -> 2 revival. */
     local_hop202.fade=fade;local_hop202.caller=caller;return 1;
    }
 memset(&local_hop202,0,sizeof(local_hop202));
 if(local_policy193.cookie!=cookie || local_policy193.epoch!=epoch ||
    !read_local(NULL,session.context+0x3340,&stage,4))return 0;
 if(stage==10){bit=1024;destination=12;}
 else if(stage==30){bit=4096;destination=11;}
 else if(stage==40){bit=8192;destination=7;}
 else return 0;
 if(!(local_policy193.mask&bit) || !local_authorized193(cookie,epoch,bit))return 0;
 _snwprintf_s(mapname,64,_TRUNCATE,L"/Game/Levels/dw/dw_%02u/dw_%02u",destination,destination);
 _snwprintf_s(spawnname,64,_TRUNCATE,L"dw_%02u_Lv_Start_01",destination);
 /* At this boundary source map is still the previous arena, so source-stage
    admission is checked separately below by the existing current-owner guards. */
 int accepted=map && spawn && map==start_name(mapname,0) && spawn==start_name(spawnname,0) &&
   read_local(NULL,session.context+0x3340,&again,4) && stage==again &&
   cookie==session.gate.current.cookie && epoch==Kh02LifecycleEpoch(life);
 if(accepted) {
  local_hop202.cookie=cookie;local_hop202.epoch=epoch;local_hop202.owner=session.context;
  local_hop202.callback=callback;local_hop202.map=map;local_hop202.spawn=spawn;
  local_hop202.caller=caller;local_hop202.stage=stage;local_hop202.group=bit;local_hop202.fade=fade;
 }
 return accepted;
}
static void local_travel200(int keep) {
 if(!keep){local_travel195();return;}
 LocalDropEvent193 event={0};event.cookie=session.gate.current.cookie;event.epoch=Kh02LifecycleEpoch(life);
 local_drop_rush_travel200(&local_policy193);local_log193(&event,9,NULL);
}
