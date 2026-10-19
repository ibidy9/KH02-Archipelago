/* Uses the existing authenticated owner-thread channel. No save/item writes. */
extern uint32_t Kh02ActionGateSet(uint64_t,uint64_t,uint32_t);
extern void Kh02ActionGateCounts(uint64_t*,uint64_t*,uint64_t*,uint64_t*);
static uint64_t action_sequence,action_command_epoch;

uint32_t Kh02ActionGatePermit(uint64_t authority) {
    return fresh_map && authority && authority==accepted_authority &&
        GetCurrentThreadId()==harness_thread &&
        (uint64_t)InterlockedCompareExchange64(&fresh_map->authority,0,0)==authority &&
        storage_permit(NULL);
}
static uint32_t action_gate_command(void *raw) {
    Kh02FreshTransportBlock *b=raw;Kh02DifficultyRequest r;uint32_t result;
    if(!b || !harness_life || !Kh02LifecycleCurrent(harness_life) ||
       !b->policy_ready || b->policy_reserved ||
       !Kh02ActionGatePermit(b->requested_authority))return 2;
    memcpy(&r,&b->action_request,sizeof(r));
    if(r.magic!=0x4b484d47 || r.version!=1 || r.size!=sizeof(r) || r.desired>127 ||
       !r.sequence || r.sequence<=action_sequence ||
       r.cookie!=difficulty_pending.cookie || r.challenge!=difficulty_pending.challenge ||
       memcmp(r.scope,difficulty_pending.scope,32) ||
       r.authority!=b->requested_authority || r.epoch!=Kh02LifecycleEpoch(harness_life))return 2;
    result=Kh02MagicGateSet(r.epoch,r.authority,r.desired,r.sequence);
    if(result==1) {
        action_sequence=r.sequence;action_command_epoch=r.epoch;
    }
    Kh02MagicPredicateSnapshot(&b->magic_predicate);return result;
}
static void action_gate_snapshot(void *raw) {
    Kh02FreshTransportBlock *b=raw;
    if(!b || !Kh02ActionGatePermit(b->requested_authority))return;
    b->action_sequence=action_sequence;b->action_epoch=action_command_epoch;
    uint32_t owned=0;
    if(!Kh02ActionOwnership(&owned))return;
    b->action_mask=owned;b->action_epoch=Kh02LifecycleEpoch(harness_life);
    Kh02ActionGateCounts(&b->action_recovery_blocked,&b->action_counter_blocked,&b->action_guard_blocked,&b->action_dodge_blocked);
}
