/* Included after the existing production composition. Private pending state;
   shared request/output fields are never used as native authority. */
static Kh02DifficultyPending difficulty_pending;
extern uint32_t Kh02DifficultyPrimaryChange(uintptr_t,uint32_t,uint32_t);
static uint32_t difficulty_bind(uint64_t cookie,uint64_t challenge,const unsigned char *scope) {
    return Kh02DifficultyBind(&difficulty_pending,cookie,challenge,scope);
}
static uint32_t difficulty_command(void *raw) {
    Kh02FreshTransportBlock *b=raw;Kh02DifficultyRequest copy;uint32_t result=2;
    if(!b || !b->policy_ready || b->policy_reserved || GetCurrentThreadId()!=harness_thread ||
       !harness_life || !Kh02LifecycleCurrent(harness_life) ||
       !b->requested_authority || b->requested_authority!=accepted_authority)return 2;
    active_authority=b->requested_authority;memcpy(&copy,&b->difficulty_request,sizeof(copy));
    result=Kh02DifficultyQueue(&difficulty_pending,&copy,active_authority,
        Kh02LifecycleEpoch(harness_life),controlled_transport_permit(NULL));
    active_authority=0;return result;
}
static void difficulty_after_load(unsigned char success) {
    uint64_t root,link,context,observed;uint32_t result;
    if(!success || difficulty_pending.phase!=1 || !harness_life ||
       GetCurrentThreadId()!=harness_thread || !Kh02LifecycleCurrent(harness_life))return;
    active_authority=difficulty_pending.authority;
    if(!controlled_transport_permit(NULL)){active_authority=0;return;}
    uint64_t base=harness_router->config.base;
    observed=Kh02DifficultyObserveRoot(harness_read,NULL,base);
    if((observed&255)!=1 || !harness_read(NULL,base+0x480b5d0,&root,8) ||
       !harness_read(NULL,root+0x230,&link,8) || !harness_read(NULL,link+0x50,&context,8) ||
       Kh02DifficultyObserve(harness_read,NULL,base,context)!=observed) {
        difficulty_pending.phase=4;difficulty_pending.result=2;active_authority=0;
        harness_end(harness_begin(19),2);return;
    }
    result=Kh02DifficultyApply(&difficulty_pending,accepted_authority,
        Kh02LifecycleEpoch(harness_life),controlled_transport_permit(NULL),context,
        (uint32_t)((observed>>8)&255),Kh02DifficultyPrimaryChange);
    active_authority=0;harness_end(harness_begin(19),result);
}
static void difficulty_snapshot(void *raw) {
    Kh02FreshTransportBlock *b=raw;
    if(!b)return;
    if(b->transport.command==3 && harness_life && Kh02LifecycleCurrent(harness_life) &&
       GetCurrentThreadId()==harness_thread && b->requested_authority==accepted_authority) {
        active_authority=b->requested_authority;
        if(controlled_transport_permit(NULL)) {
            b->difficulty_observed=Kh02DifficultyObserveRoot(harness_read,NULL,harness_router->config.base);
            Kh02DifficultyConfirm(&difficulty_pending,active_authority,
                Kh02LifecycleEpoch(harness_life),1,b->difficulty_observed);
        }
        active_authority=0;
    }
    memcpy(&b->difficulty_state,&difficulty_pending,sizeof(difficulty_pending));
}
