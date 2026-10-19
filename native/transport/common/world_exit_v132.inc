/* Retained Castle completion, observed before existing routing. No game writes.
 * Scope is the immutable prepared-session cookie. Read through authenticated
 * action 6 only; raw hits and rejects are diagnostics, never completions. */
static uint64_t exit_cookie_v132,exit_hits_v132;
static uint32_t exit_mask_v132,exit_reason_v132;
static int exit_source_v132(uint64_t context) {
    uint64_t root,instance,current,table;uint32_t story;char map[32];
    if(!context || (context&7) ||
        !read_local(NULL,game.base+0x480b5d0,&root,8) || !root ||
        !read_local(NULL,root+0x230,&instance,8) || !instance ||
        !read_local(NULL,instance+0x50,&current,8) || current!=context ||
        !read_local(NULL,context,&table,8) || table!=game.base+0x36df228 ||
        !read_local(NULL,context+0x3338,&story,4) ||
        !read_local(NULL,context+0x384c,map,sizeof(map)))return 0;
    if(story==200 && !memcmp(map,"/Game/Levels/dw/dw_59/dw_59",26))return 1;
    if(story==280 && !memcmp(map,"/Game/Levels/dw/dw_03/dw_03",26))return 2;
    if(story==360 && !memcmp(map,"/Game/Levels/dw/dw_59/dw_59",26))return 4;
    if(!memcmp(map,"/Game/Levels/dw/dw_08/dw_08",26))return 16;
    return 0;
}
/* Only the known, non-mutating forwarding scope may surround observation.
 * All authority checks still run and no delivery is performed in this scope. */
static int exit_nesting_v134(void) {
    return flight==0 || (flight==1 && ww_forward_depth_v134==1);
}
static void exit_observe_v132(uint64_t *frame,uintptr_t caller) {
    Kh02ReceiptOwner *receipt;uint64_t epoch,cookie,map,spawn,target_map=0,target_spawn=0;int world;
    /* Wrong-thread callbacks cannot touch even this diagnostic state. */
    if(GetCurrentThreadId()!=owner_thread || !frame || (caller!=game.base+0x43eca3 && caller!=game.base+0x313644))return;
    exit_hits_v132++;exit_reason_v132=1;
    cookie=session.gate.current.cookie;epoch=Kh02LifecycleEpoch(life);
    receipt=Kh02LifecycleCurrent(life);
    /* Diagnostic-only independent failure bits; never relax admission.
       Fixed existing trace retains each callback instead of only the last. */
    {
        uint32_t rejected=(!active_flag?1u:0u)|(fault?2u:0u)|(busy?4u:0u)|
            (flight?8u:0u)|(!cookie?16u:0u)|(!epoch?32u:0u)|(!receipt?64u:0u);
        uint32_t authorized=Kh02ControlledStartAuthorized();
        uint32_t membership=receipt?Kh02ReceiptOwnerCheck(receipt):UINT32_MAX;
        if(!authorized)rejected|=128u;
        if(membership!=KH02_MEMBER_OK)rejected|=256u;
        Kh02OpeningTraceStart(game.base,caller,frame[0],133,rejected);
        Kh02OpeningTraceStart(game.base,caller,frame[0],134,(uint32_t)flight);
        Kh02OpeningTraceStart(game.base,caller,frame[0],135,membership);
    }
    if(!active_flag || fault || busy || !exit_nesting_v134() || !cookie || !epoch || !receipt ||
       !Kh02ControlledStartAuthorized() || Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK)return;
    exit_reason_v132=2;
    world=exit_source_v132(frame[0]);if(!world)return;
    /* Retain the old postgame endpoint within the existing shared hook.
     * Normal Forest story350 -> cinematic dw59 remains untouched. */
    if((caller==game.base+0x313644)!=(world==16))return;
    /* Resolve immutable process-local identifiers through the existing helper. */
    map=start_name(world==1?L"/Game/Levels/dw/dw_02/dw_02":world==2?L"/Game/Levels/dw/dw_03/dw_03":L"/Game/Levels/dw/dw_09/dw_09",1);
    spawn=start_name(world==1?L"dw_02_Lv_Save_01":world==2?L"dw_03_Lv_Save_01":L"dw_09_Lv_Start_01",1);
    exit_reason_v132=3;
    /* The proven postgame guard did not constrain the incoming spawn/fade.
       Its exact Forest caller and Depths target map distinguish it from the
       normal dw59 cinematic entry; return names are resolved in this process. */
    if(!map || frame[1]!=map || (world!=16 && (!spawn || frame[2]!=spawn)))return;
    if(world==4 || world==16) {
        target_map=start_name(L"/Game/Levels/dw/dw_08/dw_08",1);
        target_spawn=start_name(L"dw_08_Lv_Save_03",1);
        if(!target_map || !target_spawn)return;
    }
    exit_reason_v132=4;
    if(!active_flag || fault || busy || !exit_nesting_v134() || !Kh02ControlledStartAuthorized() ||
       cookie!=session.gate.current.cookie || epoch!=Kh02LifecycleEpoch(life) ||
       receipt!=Kh02LifecycleCurrent(life) || Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK ||
       exit_source_v132(frame[0])!=world)return;
    if(exit_cookie_v132!=cookie){exit_cookie_v132=cookie;exit_mask_v132=0;}
    if(world==4 || world==16){frame[1]=target_map;frame[2]=target_spawn;}
    exit_mask_v132|=world==16?4:world;exit_reason_v132=5;
}

#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02ExitObserveTestV132(uint64_t *frame,uintptr_t caller) {exit_observe_v132(frame,caller);}
KH02_RECEIPT_API uint32_t Kh02ExitForwardTestV134(void *callback,void *context) {
    void *saved=kh02_ww_remote_trampoline;uint8_t result;
    if(!callback || flight || ww_forward_depth_v134 || ww_pending)return 0;
    kh02_ww_remote_trampoline=callback;
    result=kh02_ww_remote(context,0);
    kh02_ww_remote_trampoline=saved;
    return result==7 && !flight && !ww_forward_depth_v134;
}
#endif

/* Retain actual authenticated Depths final-trigger activation, not story410 alone.
 * Called only after the proven exact caller/target and access decision checks.
 * No route or prompt mutations, no extra hook, no new game writes. */
static void depths_complete_v145(uint64_t context,uint64_t epoch,uint64_t cookie) {
    Kh02ReceiptOwner *receipt;
    if(GetCurrentThreadId()!=owner_thread)return;
    receipt=Kh02LifecycleCurrent(life);
    if(!active_flag || fault || busy || !cookie || !epoch || !receipt ||
       session.context!=context || session.gate.current.cookie!=cookie ||
       Kh02LifecycleEpoch(life)!=epoch || seen_epoch!=epoch ||
       !Kh02ControlledStartAuthorized() || Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK ||
       goal_route_context(context)!=1)return;
    if(!active_flag || fault || busy || session.context!=context ||
       session.gate.current.cookie!=cookie || Kh02LifecycleEpoch(life)!=epoch ||
       Kh02LifecycleCurrent(life)!=receipt || !Kh02ControlledStartAuthorized() ||
       Kh02ReceiptOwnerCheck(receipt)!=KH02_MEMBER_OK || goal_route_context(context)!=1)return;
    if(exit_cookie_v132!=cookie){exit_cookie_v132=cookie;exit_mask_v132=0;}
    exit_mask_v132|=8; /* Existing journal makes repeated trigger ticks idempotent. */
}
