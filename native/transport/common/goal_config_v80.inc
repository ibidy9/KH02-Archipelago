/* Existing authenticated command channel; no game writes or route changes. */
static Kh02GoalConfigV80 goal_config;
extern uint32_t Kh02GoalOwnershipV79(const uint64_t*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t*);
uint32_t Kh02AcceptedGoalAccessV80(uint32_t *out) {
    uint64_t ids[4096];uint32_t n,result;
    if(!out)return 0;*out=0;
    if(!goal_config.configured || !Kh02ControlledStartAuthorized())return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    n=branch->history_count;if(n>4096)return 0;
    for(uint32_t i=0;i<n;i++)ids[i]=branch->history[i].item;
    if(!Kh02GoalOwnershipV79(ids,n,goal_config.schema,goal_config.goal,
        goal_config.requirement,goal_config.start,&result) ||
       !Kh02ControlledStartAuthorized() || harness_life->branch!=branch ||
       branch->history_count!=n)return 0;
    *out=result;return 1;
}
static void goal_config_snapshot(void *raw) {
    Kh02FreshTransportBlock *b=raw;uint32_t mask=0;
    if(!b)return;
    goal_config.available=Kh02AcceptedGoalAccessV80(&mask);
    goal_config.access=goal_config.available?mask:0;
    goal_config.epoch=goal_config.available?Kh02LifecycleEpoch(harness_life):0;
    memcpy(&b->goal_state,&goal_config,sizeof(goal_config));
}
static uint32_t goal_config_command(void *raw) {
    Kh02FreshTransportBlock *b=raw;Kh02DifficultyRequest r;uint32_t goal,requirement,start,schema;
    if(!b || !b->policy_ready || b->policy_reserved || !harness_life ||
       !Kh02LifecycleCurrent(harness_life) || !Kh02ActionGatePermit(b->requested_authority))return 2;
    memcpy(&r,&b->goal_request,sizeof(r));
    goal=r.desired&255;requirement=(r.desired>>8)&255;start=(r.desired>>16)&255;schema=r.desired>>24;
    if(r.magic!=0x4b484743 || r.version!=1 || r.size!=sizeof(r) ||
       !r.sequence || r.sequence<=goal_config.sequence ||
       r.cookie!=difficulty_pending.cookie || r.challenge!=difficulty_pending.challenge ||
       memcmp(r.scope,difficulty_pending.scope,32) || r.authority!=b->requested_authority ||
       r.epoch!=Kh02LifecycleEpoch(harness_life) || schema!=5 ||
       (goal!=1 && goal!=2) || requirement<1 || requirement>3 || start>3)return 2;
    if(goal_config.configured && (goal_config.goal!=goal || goal_config.requirement!=requirement ||
       goal_config.start!=start || goal_config.schema!=schema))return 2;
    if(!Kh02ActionGatePermit(r.authority))return 2;
    goal_config.sequence=r.sequence;goal_config.configured=1;goal_config.goal=goal;
    goal_config.requirement=requirement;goal_config.start=start;goal_config.schema=schema;
    goal_config_snapshot(raw);return 1;
}

/* Menu callback uses the same authenticated, append-only logical ownership.
   Starting Depths also authorizes entry without requiring an access item. */
uint32_t Kh02MenuDepthsOwnedV146(void) {
    Kh02DiskBranch *branch;uint32_t n,owned;
    if(!goal_config.configured || !Kh02ControlledStartAuthorized())return 0;
    branch=harness_life->branch;n=branch->history_count;
    if(n>4096)return 0;
    owned=goal_config.start==3;
    for(uint32_t i=0;i<n;i++)if(branch->history[i].item==0x020215)owned=1;
    return owned && Kh02ControlledStartAuthorized() && harness_life->branch==branch && branch->history_count==n;
}

/* Forest uses the same authenticated ownership contract. */
uint32_t Kh02MenuForestOwnedV149(void) {
    Kh02DiskBranch *branch;uint32_t n,owned;
    if(!goal_config.configured || !Kh02ControlledStartAuthorized())return 0;
    branch=harness_life->branch;n=branch->history_count;
    if(n>4096)return 0;
    owned=goal_config.start==2;
    for(uint32_t i=0;i<n;i++)if(branch->history[i].item==0x020214)owned=1;
    return owned && Kh02ControlledStartAuthorized() && harness_life->branch==branch && branch->history_count==n;
}

uint32_t Kh02MenuWorldOwnedV150(uint32_t world) {
    Kh02DiskBranch *branch;uint32_t n,owned;
    if(world>3 || !goal_config.configured || !Kh02ControlledStartAuthorized())return 0;
    branch=harness_life->branch;n=branch->history_count;if(n>4096)return 0;
    owned=goal_config.start==world;
    for(uint32_t i=0;i<n;i++)if(branch->history[i].item==0x020212+world)owned=1;
    return owned && Kh02ControlledStartAuthorized() && harness_life->branch==branch && branch->history_count==n;
}
