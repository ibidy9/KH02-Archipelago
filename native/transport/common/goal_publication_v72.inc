/* Read-only diagnostic publication under the existing stat callback.
 * Both facts are observed; only authenticated host goal selection may report a
 * win. No mapping fields are commands, and no game field is written here. */
#include "goal_witness.h"
typedef struct GoalPublicationV72 {
    uint32_t magic,version,size;volatile LONG sequence;
    uint64_t tick;uint32_t admitted,reserved;
    Kh02GoalWitness phantom,tide;
} GoalPublicationV72;
_Static_assert(sizeof(Kh02GoalWitness)==72,"witness ABI");
_Static_assert(sizeof(GoalPublicationV72)==176,"goal publication ABI");
static Kh02GoalWitness goal_phantom,goal_tide;
static uint64_t goal_epoch,goal_load,goal_cookie;
static Kh02ReceiptOwner *goal_receipt_owner;


static void Kh02GoalObservationGap(void) {
    Kh02GoalWitnessGap(&goal_phantom);Kh02GoalWitnessGap(&goal_tide);
}
/* Called only after the existing authenticated receipt probe succeeds. */
static void Kh02GoalObservationBind(uint64_t epoch,const unsigned char *data,uint32_t size) {
    uint64_t cookie,load;Kh02ReceiptOwner *owner;
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL) ||
       !Kh02ControlledStartAuthorized() || epoch!=Kh02LifecycleEpoch(life) ||
       !data || size<56 || memcmp(data,"KH02RSET",8))return;
    owner=Kh02LifecycleCurrent(life);
    if(!owner || Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK)return;
    memcpy(&cookie,data+16,8);memcpy(&load,data+24,8);
    if(!cookie || !load || cookie!=session.gate.current.cookie)return;
    if(goal_receipt_owner!=owner || goal_epoch!=epoch || goal_load!=load)Kh02GoalObservationGap();
    goal_receipt_owner=owner;goal_epoch=epoch;goal_load=load;goal_cookie=cookie;
}
static uint32_t Kh02GoalObservationRead(void) {
    Kh02SessionSample sample;Kh02ReceiptOwner *owner;
    uint64_t epoch;uint32_t objective;unsigned char bonus;
    char map[32];Kh02GoalFrame f={0};
    if(!is_active(NULL) || !session.context ||
       !Kh02ControlledStartAuthorized())return 0;
    owner=Kh02LifecycleCurrent(life);epoch=Kh02LifecycleEpoch(life);
    if(!owner || Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK)return 0;
    if(goal_receipt_owner!=owner || goal_epoch!=epoch || !goal_load)return 0;
    if(Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&sample)!=KH02_GATE_OK ||
       sample.phase!=3 || sample.title_flags || sample.pending_map || (sample.travel_flags&1) || sample.world_flag ||
       sample.context!=session.context || sample.world!=session.world ||
       sample.provider!=session.gate.current.provider || sample.attributes!=session.gate.current.attributes ||
       !read_local(NULL,sample.context+0x338+0xc20+41*4,&objective,4) ||
       !read_local(NULL,sample.context+0x353d,&bonus,1) ||
       !read_local(NULL,sample.context+0x384c,map,sizeof(map)) ||
       !is_active(NULL) || !Kh02ControlledStartAuthorized() ||
       Kh02LifecycleCurrent(life)!=owner || Kh02LifecycleEpoch(life)!=epoch ||
       Kh02ReceiptOwnerCheck(owner)!=KH02_MEMBER_OK)return 0;
    if((objective&15)>2)return 0;
    f.cookie=goal_cookie;f.epoch=epoch;f.load_id=goal_load;f.owner=sample.context;
    f.admitted=1;f.objective42=objective&15;f.tide_bonus=bonus;
    f.in_tide=!memcmp(map,"/Game/Levels/dw/dw_10/dw_10",sizeof("/Game/Levels/dw/dw_10/dw_10"));
    f.selected_goal=1;Kh02GoalWitnessStep(&goal_phantom,&f);
    f.selected_goal=2;Kh02GoalWitnessStep(&goal_tide,&f);
    return 1;
}
static void Kh02GoalObservationService(void) {
    GoalPublicationV72 value={0},*block;HANDLE h;wchar_t name[96];
    /* No host mapping: no probe/lease work. Existing pending proof survives. */
    _snwprintf_s(name,96,_TRUNCATE,L"Local\\KH02APGoalObservationV72-%lu",GetCurrentProcessId());
    h=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);
    if(!h){Kh02GoalObservationGap();return;}
    block=MapViewOfFile(h,FILE_MAP_ALL_ACCESS,0,0,sizeof(*block));
    if(block && block->magic==0x3237474b && block->version==1 && block->size==sizeof(*block)) {
        value.admitted=Kh02GoalObservationRead();
        if(!value.admitted)Kh02GoalObservationGap();
        value.tick=GetTickCount64();value.phantom=goal_phantom;value.tide=goal_tide;
        InterlockedIncrement(&block->sequence);MemoryBarrier();
        memcpy((unsigned char*)block+16,(unsigned char*)&value+16,sizeof(value)-16);
        MemoryBarrier();InterlockedIncrement(&block->sequence);
    } else Kh02GoalObservationGap();
    if(block)UnmapViewOfFile(block);CloseHandle(h);
}
