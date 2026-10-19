/* Single-owner asynchronous Tide choice. No game UI pointer is dereferenced
 * after its close handler. All strings live for the pinned module lifetime. */
typedef struct TideTextV87 {const wchar_t *data;int32_t count,capacity;} TideTextV87;
#define TIDE_TEXT(s) {s,(int32_t)(sizeof(s)/sizeof(wchar_t)),(int32_t)(sizeof(s)/sizeof(wchar_t))}
static const TideTextV87 tide_message=TIDE_TEXT(L"The way to Demon Tide is open.<br>Continue to the final battle?");
static const TideTextV87 tide_continue=TIDE_TEXT(L"Continue");
static const TideTextV87 tide_return=TIDE_TEXT(L"Return");
static struct {
    uint64_t owner,epoch,cookie,object,serial;
    uint32_t state,fade,selected,explicit_continue,configured;
} tide_choice;
static uint64_t tide_choice_counts[10];
static void tide_choice_report(uint32_t reason) {
    if(reason<10)tide_choice_counts[reason]++;
    Kh02OpeningTraceStart(game.base,game.base+0x563c95,tide_choice.owner,87,reason);
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02TideChoiceDebugV87(uint64_t *out) {
    memcpy(out,tide_choice_counts,sizeof(tide_choice_counts));
    out[10]=tide_choice.state;out[11]=tide_choice.object;out[12]=tide_choice.serial;
}
#endif
static int tide_choice_current(void) {
    return GetCurrentThreadId()==owner_thread && active_flag && !fault && !busy &&
        tide_choice.owner && tide_choice.epoch && tide_choice.cookie &&
        session.context==tide_choice.owner && Kh02LifecycleEpoch(life)==tide_choice.epoch &&
        session.gate.current.cookie==tide_choice.cookie &&
        Kh02ControlledStartAuthorized() && goal_route_context(tide_choice.owner)==1;
}
static int tide_choice_customize(void *object,void *message,void **button0,void **button1,uintptr_t caller) {
    uint64_t table;
    if(tide_choice.state!=1 || message!=&tide_message || caller!=game.base+0x58cc62 ||
       !tide_choice_current() || tide_choice.object ||
       !zodiac_read((uintptr_t)object,&table,8) || table!=game.base+0x3788c60)return 0;
    tide_choice.object=(uintptr_t)object;
    *button0=(void*)&tide_continue;*button1=(void*)&tide_return;
    return 1;
}
static void tide_choice_configured(void *object,unsigned char result) {
    if(GetCurrentThreadId()==owner_thread && tide_choice.state==1 &&
       tide_choice.object==(uintptr_t)object)tide_choice.configured=result!=0;
}
static void tide_choice_event(void *object,uint32_t event,uint32_t value) {
    if(GetCurrentThreadId()!=owner_thread || tide_choice.state!=2 ||
       tide_choice.object!=(uintptr_t)object)return;
    if(event==7 || event==8 || event==5) {
        tide_choice.explicit_continue=event==7 && value==0;
        tide_choice_report(2);
    }
}
static void tide_choice_closed(void *object,uint32_t phase,uint32_t selected) {
    if(GetCurrentThreadId()!=owner_thread || tide_choice.state!=2 || phase!=1 ||
       tide_choice.object!=(uintptr_t)object)return;
    tide_choice.selected=selected;tide_choice.object=0;tide_choice.state=3;
    tide_choice_report(3);
}
static uint32_t tide_choice_begin(uint64_t *frame,uint64_t epoch) {
    typedef uint32_t (__fastcall *OpenFn)(void*,uint32_t,const TideTextV87*,unsigned char,uint32_t,const void*,unsigned char);
    uint64_t manager,table;unsigned char prefix[15];
    const uint64_t empty_delegate[7]={0};
    static const unsigned char expected[15]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20};
    if(tide_choice.state){tide_choice_counts[8]++;return 2;} /* Refuse quietly: trigger repeats each tick while UI is open. */
    tide_choice.owner=frame[0];tide_choice.epoch=epoch;tide_choice.cookie=session.gate.current.cookie;
    tide_choice.fade=(uint32_t)frame[3];tide_choice.object=0;tide_choice.selected=UINT32_MAX;
    tide_choice.explicit_continue=tide_choice.configured=0;
    if(!tide_choice_current() || !zodiac_read(frame[0]+0x7e30,&manager,8) || !manager ||
       !zodiac_read(manager,&table,8) || table<game.base || table>=game.base+0x4f5f000 ||
       !zodiac_read(game.base+0x58cb00,prefix,sizeof(prefix)) || memcmp(prefix,expected,sizeof(prefix))) {
        tide_choice_report(7);return 0; /* Caller keeps proven save-point fallback. */
    }
    tide_choice.serial++;tide_choice.state=1;
    /* The opener copies an empty delegate; it retains no pointers into this
     * stack. Our existing phase observer supplies the asynchronous response. */
    ((OpenFn)(game.base+0x58cb00))((void*)(uintptr_t)manager,2,&tide_message,1,1,empty_delegate,0);
    if(tide_choice.object && tide_choice.configured && tide_choice_current()) {
        tide_choice.state=2;tide_choice_report(1);
    } else {
        /* Uncertain UI creation must never fall through to the ending. */
        tide_choice.state=4;tide_choice_report(7);
    }
    return 1;
}
static void TideChoiceServiceV87(void) {
    typedef unsigned char (__fastcall *TravelFn)(void*,uint64_t,uint64_t,uint32_t);
    uint32_t mask=0,go,fade;uint64_t owner,map,spawn;
    if(GetCurrentThreadId()!=owner_thread || tide_choice.state!=3)return;
    if(!tide_choice_current()) {tide_choice.state=0;tide_choice_report(6);return;}
    go=tide_choice.explicit_continue && tide_choice.selected==0 &&
       Kh02AcceptedGoalAccessV80(&mask) && (mask&1);
    owner=tide_choice.owner;fade=tide_choice.fade;
    map=go?goal_names.tide_map:goal_names.depths_map;
    spawn=go?goal_names.tide_spawn:goal_names.save_spawn;
    if(!map || !spawn || !tide_choice_current()) {tide_choice.state=0;tide_choice_report(6);return;}
    tide_choice.state=0; /* Consume before potentially reentrant native travel. */
    tide_choice_report(go?5:4);
    ((TravelFn)(game.base+0x458f40))((void*)(uintptr_t)owner,map,spawn,fade);
}
