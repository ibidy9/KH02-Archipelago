/* Development journey lease. Existing production modes are untouched.
 * Only the shared owner services this; no timer thread or external writer. */
typedef struct JourneyIdentity229 {
    uint64_t cookie,epoch,owner,player,manager;
    char map[32];
} JourneyIdentity229;
static JourneyIdentity229 journey_candidate229,journey_bound229;
static uint64_t journey_since229,journey_last229;
static void journey_reset229(void) {
    memset(&journey_candidate229,0,sizeof(journey_candidate229));
    memset(&journey_bound229,0,sizeof(journey_bound229));
    journey_since229=journey_last229=0;
}
static int journey_sample229(JourneyIdentity229 *v) {
    Kh02SessionSample s;uint32_t hp,flags,owned=0;
    memset(v,0,sizeof(*v));
    if(GetCurrentThreadId()!=owner_thread || !is_active(NULL) || !Kh02ControlledStartAuthorized() ||
       !session.context || busy || (mickey_mode168!=1 && mickey_mode168!=2) || mickey_cookie168!=session.gate.current.cookie ||
       !Kh02MickeyOwnership171(&owned) || !owned ||
       Kh02OwnerReadSample(read_local,NULL,game.base,game.cache,session.gate.current.generation,&s)!=KH02_GATE_OK ||
       s.context!=session.context || s.world!=session.world ||
       s.provider!=session.gate.current.provider || s.attributes!=session.gate.current.attributes ||
       s.phase!=3 || s.title_flags || s.pending_map || (s.travel_flags&1) || s.world_flag ||
       !read_local(NULL,s.context+0x384c,v->map,32) || !lab_map225(v->map) ||
       !read_local(NULL,s.context+0x348,&flags,4) || (flags&2) ||
       !read_local(NULL,s.provider+0x5dc,&hp,4) || !hp ||
       !read_local(NULL,s.provider+0x9e8,&v->manager,8) || !mickey_ptr167(v->manager) ||
       !auto_bytes224(game.base+0x1eb3590,pause224,sizeof(pause224)))return 0;
#ifdef KH02_GRANT_OWNER_SELFTEST
    if(test_pause224>=0){if(test_pause224)return 0;}else
#endif
    if(((unsigned char (__fastcall *)(void*))(game.base+0x1eb3590))((void*)(uintptr_t)s.world))return 0;
    v->cookie=session.gate.current.cookie;v->epoch=Kh02LifecycleEpoch(life);
    v->owner=s.context;v->player=s.provider;return 1;
}
static void journey229(void) {
    JourneyIdentity229 v,again;MickeyControl168 b={0};uint64_t now;
    if(!journey_enabled229)return;
    now=now224();
    if(mickey_failed171 || (mickey_mode168!=2 && (now>=journey_deadline229 || journey_attempts229>=24))) {
        journey_enabled229=0;auto_enabled224=0;return;
    }
    if(!journey_sample229(&v)) {journey_since229=journey_last229=0;return;}
    if(!memcmp(&v,&journey_bound229,sizeof(v)))return;
    if(!journey_since229 || now<journey_last229 || now-journey_last229>2000 ||
       memcmp(&v,&journey_candidate229,sizeof(v))) {
        journey_candidate229=v;journey_since229=journey_last229=now;return;
    }
    journey_last229=now;
    if(now-journey_since229<1000)return;
    if(!journey_sample229(&again) || memcmp(&v,&again,sizeof(v)))return;
    /* Consume before mutation. Any rejected/uncertain binding stops this lease.
     * Never automatically retry creation, adopt another pawn, or recreate a
     * missing pawn under the same bound identity. */
    journey_bound229=v;if(journey_attempts229<UINT32_MAX)journey_attempts229++;
    b.magic=0x38364b4d;b.version=1;b.size=sizeof(b);b.request=1;b.command=3;
    b.cookie=v.cookie;b.epoch=v.epoch;mickey_process168(&b);
    if(b.response!=1 || b.status!=1 || b.after!=1 || (b.result!=1 && b.result!=32)) {
        journey_enabled229=0;auto_enabled224=0;return;
    }
    b.request=2;b.command=5;mickey_process168(&b);
    if(b.response!=2 || b.status!=1){journey_enabled229=0;auto_enabled224=0;}
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API uint32_t Kh02JourneyState229(void) {
    return journey_attempts229 | (journey_enabled229<<16);
}
#endif
