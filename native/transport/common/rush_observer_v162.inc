/* Uses the existing remote-event callback. No additional hook or game write. */
#include "rush_witness_v162.h"
static RushWitness162 rush162;
static void local_rush200(uint64_t,uint64_t,unsigned);
static void rush_event162(void *context,uint64_t event) {
    static const wchar_t *names[]={
        L"dw_11_camera_REvt_Battle01_EndCamera_Finish",
        L"dw_12_camera_REvt_Battle02_EndCamera_Finish",
        L"dw_12_camera_REvt_Battle03_EndCamera_Finish",
        L"dw_11_camera_REvt_Battle04_EndCamera_Finish"};
    Kh02ReceiptOwner *receipt;uint64_t cookie,epoch,root,instance,current,table;
    uint32_t stage,again;char map[32];unsigned round=0,number=0,i;
    (void)context; /* Broadcast sender is a level script, not the save owner. */
    if(GetCurrentThreadId()!=owner_thread || !event)return;
    for(i=0;i<4;i++)if(event==start_name(names[i],0)){round=i+1;break;}
    if(!round)return;
    rush162.hits++;rush162.reason=1;
    cookie=session.gate.current.cookie;epoch=Kh02LifecycleEpoch(life);
    receipt=Kh02LifecycleCurrent(life);
    if(!active_flag || fault || busy || flight!=1 || !cookie || !epoch || !receipt ||
       !Kh02ControlledStartAuthorized() || Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK ||
       !session.context || seen_epoch!=epoch)return;
    rush162.reason=2;
    if(!read_local(NULL,game.base+0x480b5d0,&root,8) || !root ||
       !read_local(NULL,root+0x230,&instance,8) || !instance ||
       !read_local(NULL,instance+0x50,&current,8) || current!=session.context ||
       !read_local(NULL,current,&table,8) || table!=game.base+0x36df228 ||
       !read_local(NULL,current+0x3340,&stage,4) ||
       !read_local(NULL,current+0x384c,map,sizeof(map)))return;
    if(!memcmp(map,"/Game/Levels/dw/dw_11/dw_11",26))number=11;
    if(!memcmp(map,"/Game/Levels/dw/dw_12/dw_12",26))number=12;
    rush162.reason=3;
    if(!read_local(NULL,current+0x3340,&again,4) || stage!=again ||
       !active_flag || fault || busy || flight!=1 || !Kh02ControlledStartAuthorized() ||
       cookie!=session.gate.current.cookie || epoch!=Kh02LifecycleEpoch(life) ||
       receipt!=Kh02LifecycleCurrent(life) || Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK)return;
    if(rush_complete162(&rush162,cookie,round,number,stage)) {rush162.reason=4;local_rush200(cookie,epoch,round);}
    Kh02OpeningTraceStart(game.base,game.base+0x438870,current,162,
                         (round<<16)|(stage<<8)|rush162.reason);
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02RushEventTestV162(uint64_t event) {
    InterlockedIncrement(&flight);rush_event162(NULL,event);InterlockedDecrement(&flight);
}
#endif
