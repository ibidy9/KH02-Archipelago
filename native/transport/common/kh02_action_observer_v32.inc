/* Included only by the additive v32 owner. No game writes or game calls. */
static void diagnostic_inc(uint64_t *value) {
    volatile LONG64 *counter=(volatile LONG64*)value;LONG64 before;
    do {
        before=InterlockedCompareExchange64(counter,0,0);
        if(before==INT64_MAX) {
            InterlockedOr64((volatile LONG64*)&default_abilities.overflow,1);
            return;
        }
    } while(InterlockedCompareExchange64(counter,before+1,before)!=before);
}
#include "kh02_action_gate_v33.inc"
static int diagnostic_pointer(uint64_t pointer) {
    return pointer>=0x10000 && pointer<=0x7fffffffffffULL-0x5000 && !(pointer&7);
}
static void diagnostic_epoch(void) {
    default_abilities.epoch=seen_epoch;
    memset(default_abilities.epoch_counts,0,sizeof(default_abilities.epoch_counts));
    memset(default_abilities.samples,0,sizeof(default_abilities.samples));
    memset(default_abilities.callers,0,sizeof(default_abilities.callers));
}
/* Recovery is an action-predicate filter; native kind 1 is NEVER blocked.
   Counter Blast retains the independent native kind-4 filter. */
uint32_t kh02_default_ability_capture(void *raw,uint32_t kind,void *return_address,uint32_t hook) {
    uint32_t blocked=0,reason=KH02_OBS_ACCEPTED,index=hook==0?0:2,record=0,substate=0;
    uint64_t receiver=(uintptr_t)raw,actor=receiver,vtable=0,primary=0,provider=0;
    uint64_t predicate=0,attributes=0,again=0,manager=0,state=0,state_vtable=0;
    uint64_t caller=(uintptr_t)return_address;unsigned char state_kind=0;
    Kh02AbilityDiagnosticCounters *counter;
    if(hook>1)return 0;
    InterlockedIncrement(&flight);
    counter=&default_abilities.hooks[hook];diagnostic_inc(&counter->raw);
    if((hook==0 && kind==1) || (hook==1 && kind==4))diagnostic_inc(&counter->target_attempts[index]);
    if(GetCurrentThreadId()!=owner_thread){reason=KH02_OBS_THREAD;goto done;}
    if(!InterlockedCompareExchange(&active_flag,0,0)){reason=KH02_OBS_INACTIVE;goto done;}
    if(InterlockedCompareExchange(&fault,0,0)){reason=KH02_OBS_FAULT;goto done;}
    if(!life || !Kh02LifecycleCurrent(life)){reason=KH02_OBS_OWNER;goto done;}
    if(Kh02LifecycleEpoch(life)!=seen_epoch){reason=KH02_OBS_EPOCH;goto done;}
    if(!session.context || !session.world || !session.gate.current.observation){reason=KH02_OBS_SESSION;goto done;}
    if(kind!=(hook==0?1u:4u)){reason=KH02_OBS_KIND;goto done;}
    record=1;provider=session.gate.current.provider;
    if(hook==0)actor=receiver>=0x450?receiver-0x450:0;
    if(!diagnostic_pointer(receiver) || !diagnostic_pointer(actor) ||
       !read_local(NULL,receiver,&vtable,8) || !read_local(NULL,actor,&primary,8)) {
        reason=KH02_OBS_RECEIVER;goto done;
    }
    if((primary!=game.base+0x3735bc8 && primary!=game.base+0x373d490) ||
       (hook==0 && vtable!=game.base+(primary==game.base+0x3735bc8?0x3737f60:0x373f828)) ||
       !read_local(NULL,vtable+(hook==0?0x118:0x1230),&predicate,8) ||
       predicate!=game.base+(hook==0?0x512300:0x517320)) {reason=KH02_OBS_TABLE;goto done;}
    if(actor!=provider || !read_local(NULL,actor+0x4c8,&attributes,8) ||
       !diagnostic_pointer(attributes) || attributes!=session.gate.current.attributes) {
        reason=KH02_OBS_CHAIN;goto done;
    }
    if(hook==0) {
        if(!read_local(NULL,receiver+0x130,&manager,8) || !diagnostic_pointer(manager) ||
           !read_local(NULL,manager+0x270,&state,8) || !diagnostic_pointer(state) ||
           !read_local(NULL,state+0x58,&state_kind,1)) {reason=KH02_OBS_CHAIN;goto done;}
        if(state_kind!=0x11){record=0;reason=KH02_OBS_KIND;goto done;}
        if(!read_local(NULL,state,&state_vtable,8) || !read_local(NULL,state+0x88,&substate,4)) {
            reason=KH02_OBS_CHAIN;goto done;
        }
        if(state_vtable!=game.base+0x3725778){reason=KH02_OBS_TABLE;goto done;}
        if(substate!=2 && substate!=7 && substate!=8 && substate!=10) {
            record=0;reason=KH02_OBS_KIND;goto done;
        }
        if(!read_local(NULL,receiver+0x130,&again,8) || again!=manager ||
           !read_local(NULL,manager+0x270,&again,8) || again!=state) {
            reason=KH02_OBS_UNSTABLE;goto done;
        }
    }
    if(!read_local(NULL,receiver,&again,8) || again!=vtable ||
       !read_local(NULL,actor+0x4c8,&again,8) || again!=attributes) {
        reason=KH02_OBS_UNSTABLE;goto done;
    }
    diagnostic_inc(&default_abilities.epoch_counts[hook][index]);
    /* Recovery action request caller only; all other Jump uses delegate. */
    if(hook==1 || caller==game.base+0x51a23f)blocked=action_should_block(hook);
done:
    if(record) {
        if(caller<game.base+0x1000 || caller>=game.base+0x3598000)caller=0;else caller-=game.base;
        default_abilities.samples[hook][index]=(Kh02AbilityDiagnosticSample){
            receiver,vtable,actor,provider,caller,kind,(int32_t)reason,
            manager,state,state_vtable,state_kind,substate};
        if(reason==KH02_OBS_ACCEPTED && caller) {
            uint32_t i;
            for(i=0;i<4;i++) {
                uint64_t *slot=&default_abilities.callers[hook][index][i];
                if(*slot==caller)break;if(!*slot){*slot=caller;break;}
            }
            if(i==4)InterlockedOr64((volatile LONG64*)&default_abilities.overflow,2);
        }
    }
    diagnostic_inc(&counter->reasons[reason]);InterlockedDecrement(&flight);return blocked;
}

uint32_t kh02_guard_dodge_capture(void *raw,uint32_t kind,void *return_address,uint32_t hook) {
    uint32_t blocked=0,reason=KH02_OBS_ACCEPTED,index=kind==2?1:kind==28?3:4,record=0;
    uint64_t receiver=(uintptr_t)raw,vtable=0,actor=0,provider=0,primary=0;
    uint64_t predicate=0,attributes=0,again=0,caller=(uintptr_t)return_address;
    Kh02AbilityDiagnosticCounters *counter;
    if(hook<2 || hook>3)return 0;
    InterlockedIncrement(&flight);
    counter=&default_abilities.hooks[hook];diagnostic_inc(&counter->raw);
    if(index<4)diagnostic_inc(&counter->target_attempts[index]);
    if(GetCurrentThreadId()!=owner_thread){reason=KH02_OBS_THREAD;goto done;}
    if(!InterlockedCompareExchange(&active_flag,0,0)){reason=KH02_OBS_INACTIVE;goto done;}
    if(InterlockedCompareExchange(&fault,0,0)){reason=KH02_OBS_FAULT;goto done;}
    if(!life || !Kh02LifecycleCurrent(life)){reason=KH02_OBS_OWNER;goto done;}
    if(Kh02LifecycleEpoch(life)!=seen_epoch){reason=KH02_OBS_EPOCH;goto done;}
    if(!session.context || !session.world || !session.gate.current.observation) {
        reason=KH02_OBS_SESSION;goto done;
    }
    if(kind!=(hook==2?2u:28u)){reason=KH02_OBS_KIND;goto done;}
    record=1;provider=session.gate.current.provider;
    if(receiver>=0x450)actor=receiver-0x450;
    if(!diagnostic_pointer(receiver) || !diagnostic_pointer(actor) ||
       !read_local(NULL,receiver,&vtable,8)) {reason=KH02_OBS_RECEIVER;goto done;}
    if((vtable!=game.base+0x3737f60 && vtable!=game.base+0x373f828) ||
       !read_local(NULL,actor,&primary,8) ||
       primary!=game.base+(vtable==game.base+0x3737f60?0x3735bc8:0x373d490) ||
       !read_local(NULL,vtable+(hook==2?0x168:0x180),&predicate,8) ||
       predicate!=game.base+(hook==2?0x511dc0:0x511060)) {
        reason=KH02_OBS_TABLE;goto done;
    }
    if(actor!=provider || !read_local(NULL,actor+0x4c8,&attributes,8) ||
       !diagnostic_pointer(attributes) || attributes!=session.gate.current.attributes) {
        reason=KH02_OBS_CHAIN;goto done;
    }
    if(!read_local(NULL,receiver,&again,8) || again!=vtable ||
       !read_local(NULL,actor,&again,8) || again!=primary ||
       !read_local(NULL,actor+0x4c8,&again,8) || again!=attributes) {
        reason=KH02_OBS_UNSTABLE;goto done;
    }
    diagnostic_inc(&default_abilities.epoch_counts[hook][index]);
    blocked=action_should_block(hook);
done:
    if(record) {
        if(caller<game.base+0x1000 || caller>=game.base+0x3598000)caller=0;
        else caller-=game.base;
        default_abilities.samples[hook][index]=(Kh02AbilityDiagnosticSample){
            receiver,vtable,actor,provider,caller,kind,(int32_t)reason,0,0,0,0,0};
        if(reason==KH02_OBS_ACCEPTED && caller) {
            uint32_t i;
            for(i=0;i<4;i++) {
                uint64_t *slot=&default_abilities.callers[hook][index][i];
                if(*slot==caller)break;
                if(!*slot){*slot=caller;break;}
            }
            if(i==4)InterlockedOr64((volatile LONG64*)&default_abilities.overflow,2);
        }
    }
    diagnostic_inc(&counter->reasons[reason]);
    InterlockedDecrement(&flight);
    return blocked;
}


uint32_t Kh02GrantOwnerDefaultAbilitySnapshot(Kh02DefaultAbilitySnapshot *out) {
    uint64_t before[4];uint32_t h,i;LONG busy_before;
    if(!out)return KH02_GATE_REFUSED;
    memset(out,0,sizeof(*out));
    if(GetCurrentThreadId()!=owner_thread)return KH02_GATE_REFUSED;
    busy_before=InterlockedCompareExchange(&flight,0,0);
    for(h=0;h<4;h++)before[h]=(uint64_t)InterlockedCompareExchange64(
        (volatile LONG64*)&default_abilities.hooks[h].raw,0,0);
    /* Sample arrays are owner-thread-only; foreign threads touch only atomics. */
    out->epoch=default_abilities.epoch;
    memcpy(out->epoch_counts,default_abilities.epoch_counts,sizeof(out->epoch_counts));
    memcpy(out->samples,default_abilities.samples,sizeof(out->samples));
    memcpy(out->callers,default_abilities.callers,sizeof(out->callers));
    out->overflow=(uint64_t)InterlockedCompareExchange64((volatile LONG64*)&default_abilities.overflow,0,0);
    for(h=0;h<4;h++) {
        out->hooks[h].raw=(uint64_t)InterlockedCompareExchange64((volatile LONG64*)&default_abilities.hooks[h].raw,0,0);
        for(i=0;i<KH02_OBS_REASONS;i++)out->hooks[h].reasons[i]=(uint64_t)InterlockedCompareExchange64(
            (volatile LONG64*)&default_abilities.hooks[h].reasons[i],0,0);
        for(i=0;i<4;i++)out->hooks[h].target_attempts[i]=(uint64_t)InterlockedCompareExchange64(
            (volatile LONG64*)&default_abilities.hooks[h].target_attempts[i],0,0);
    }
    out->coherent=!busy_before && !InterlockedCompareExchange(&flight,0,0);
    for(h=0;h<4;h++)if(before[h]!=(uint64_t)InterlockedCompareExchange64(
        (volatile LONG64*)&default_abilities.hooks[h].raw,0,0))out->coherent=0;
    return KH02_GATE_OK;
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02AbilityDiagnosticTestCapture(void *receiver,uint32_t kind,void *caller,uint32_t hook) {
    if(hook<2)kh02_default_ability_capture(receiver,kind,caller,hook);
    else kh02_guard_dodge_capture(receiver,kind,caller,hook);
}
KH02_RECEIPT_API void Kh02AbilityDiagnosticTestSaturate(void) {
    memset(default_abilities.hooks,0,sizeof(default_abilities.hooks));
    default_abilities.hooks[1].raw=INT64_MAX;
    default_abilities.hooks[1].reasons[KH02_OBS_ACCEPTED]=INT64_MAX;
}
#endif
