#include "goal_config_v80.h"
static uint32_t goal_config_command(void*);
static void goal_config_snapshot(void*);
#include "kh02_magic_observation_v38.h"
#include "kh02_magic_predicate_v41.h"
#include "kh02_menu_gate_v45.h"
#include "kh02_situation_observer_v46.h"
#include "kh02_style_gate_v47.h"
#include "kh02_meter_access_v48.h"
#include "kh02_difficulty_pending_candidate.h"
#include "kh02_grant_owner_candidate.h"
static uint32_t difficulty_bind(uint64_t,uint64_t,const unsigned char*);
static uint32_t difficulty_command(void*);
static void difficulty_after_load(unsigned char);
static void difficulty_snapshot(void*);
static void difficulty_observation_snapshot(void*);
static void default_ability_snapshot(void*);
static uint32_t action_gate_command(void*);
static void action_gate_snapshot(void*);
#define KH02_CONTROLLED_SNAPSHOT(block) do {difficulty_observation_snapshot(block);difficulty_snapshot(block);default_ability_snapshot(block);action_gate_snapshot(block);goal_config_snapshot(block);} while(0)
#include "kh02_policy_command_candidate.h"
static uint32_t production_after_setup(uint32_t);
static uint32_t production_command_service(void*,uint32_t);
static uint32_t custom_popup_command_v115(void*);
static uint32_t objective_command_v116(void*);
static uint32_t mickey_config_command172(void*);
static uint32_t xp_config_command185(void*);
static uint32_t trap_command242(void*);
static uint32_t death_command249(void*);
extern uint32_t Kh02ObjectiveBeforeTransitionV116(void);
#define KH02_CONTROLLED_BEFORE_STOP() Kh02ObjectiveBeforeTransitionV116()
#define KH02_CONTROLLED_BLOCK_EXTRA Kh02PolicyCommand policy_command; uint64_t policy_challenge; unsigned char policy_scope[32]; uint32_t policy_ready,policy_reserved; Kh02DifficultyRequest difficulty_request; Kh02DifficultyPending difficulty_state; uint64_t difficulty_observed; Kh02DefaultAbilitySnapshot default_abilities; Kh02DifficultyRequest action_request; uint64_t action_sequence,action_epoch,action_mask,action_recovery_blocked,action_counter_blocked,action_guard_blocked,action_dodge_blocked; Kh02MagicObservation magic_observation; Kh02MagicPredicate magic_predicate; Kh02MenuObservation menu_observation; Kh02SituationObservation situation_observation; Kh02StyleGate style_gate; Kh02MeterAccess meter_access; Kh02DifficultyRequest goal_request; Kh02GoalConfigV80 goal_state;
#define KH02_CONTROLLED_FRESH_REPLAY_SETUP 1
#define KH02_CONTROLLED_AFTER_SETUP(value) production_after_setup(value)
#define KH02_CONTROLLED_EXTRA_COMMAND(block,command,status,handled) do { \
    if((command)==15){(handled)=1;(status)=death_command249(block);} \
    if((command)==14){(handled)=1;(status)=trap_command242(block);} \
    if((command)==13){(handled)=1;(status)=xp_config_command185(block);} \
    if((command)==12){(handled)=1;(status)=mickey_config_command172(block);} \
    if((command)==11){(handled)=1;(status)=objective_command_v116(block);} \
    if((command)==10){(handled)=1;(status)=custom_popup_command_v115(block);} \
    if((command)==9){(handled)=1;(status)=goal_config_command(block);} \
    if((command)==8){(handled)=1;(status)=action_gate_command(block);} \
    if((command)==7){(handled)=1;(status)=difficulty_command(block);} \
    if((command)==KH02_POLICY_COMMAND){(handled)=1;(status)=production_command_service((block),(command));} \
} while(0)
#include "kh02_controlled_session_transport_candidate.c"

extern uint32_t Kh02CustomPopupRequestV115(const void*,uint32_t);
extern uint32_t Kh02ObjectiveRequestV116(void*,uint32_t,uint32_t,uint32_t*);
uint32_t Kh02ControlledStartAuthorized(void);
uint32_t Kh02ObjectiveAuthorityV116(void) {
    return GetCurrentThreadId()==harness_thread && fresh_map && accepted_authority &&
        (uint64_t)InterlockedCompareExchange64(&fresh_map->authority,0,0)==accepted_authority &&
        Kh02ControlledStartAuthorized();
}
static uint32_t objective_command_v116(void *raw) {
    Kh02FreshTransportBlock *block=raw;uint32_t status=2,written=0;
    if(GetCurrentThreadId()!=harness_thread || !block->requested_authority ||
       block->requested_authority!=accepted_authority)return status;
    active_authority=block->requested_authority;
    if(controlled_transport_permit(NULL))status=Kh02ObjectiveRequestV116(block->proposal,
        block->proposal_size,sizeof(block->proposal),&written);
    block->proposal_size=written;active_authority=0;return status;
}
static uint32_t custom_popup_command_v115(void *raw) {
    Kh02FreshTransportBlock *block=raw;uint32_t status=2;
    if(GetCurrentThreadId()!=harness_thread || !block->requested_authority ||
       block->requested_authority!=accepted_authority)return status;
    active_authority=block->requested_authority;
    if(controlled_transport_permit(NULL))status=Kh02CustomPopupRequestV115(block->proposal,block->proposal_size);
    active_authority=0;return status;
}

static void default_ability_snapshot(void *raw) {
    Kh02FreshTransportBlock *block=raw;
    if(!block || block->transport.command!=3)return;
    memset(&block->default_abilities,0,sizeof(block->default_abilities));
    if(block->requested_authority && block->requested_authority==accepted_authority)
        Kh02GrantOwnerDefaultAbilitySnapshot(&block->default_abilities);
    Kh02GrantOwnerMagicObservation(&block->magic_observation);
    Kh02MagicPredicateSnapshot(&block->magic_predicate);
    Kh02MenuObservationSnapshot(&block->menu_observation);
    Kh02SituationObservationSnapshot(&block->situation_observation);
    Kh02StyleGateSnapshot(&block->style_gate);
    Kh02MeterAccessSnapshot(&block->meter_access);
}

static Kh02PolicyCommandOwner production_owner;
static Kh02PolicyProjection accepted_projection;

/* Opening travel is a native callback BETWEEN client commands. Use the same
 * authenticated startup/storage binding as FreshCopy and the clear/menu
 * initializer. No history-command acknowledgement is required before New Game:
 * its receipt owner does not exist yet. Grant permissions remain unchanged. */
uint32_t Kh02ControlledStartAuthorized(void) {
    return harness_router && harness_life && GetCurrentThreadId()==harness_thread &&
        storage_permit(NULL) && !harness_router->closed && !harness_router->domain.closed &&
        !harness_life->closed && harness_life->branch && !harness_life->branch->closed &&
        Kh02LifecycleCurrent(harness_life)!=NULL;
}
#ifdef KH02_GRANT_OWNER_SELFTEST
KH02_RECEIPT_API void Kh02StartAuthorityDebugV56(uint64_t *out) {
    uint64_t v[]={(uintptr_t)fresh_map,accepted_authority,fresh_map?(uint64_t)fresh_map->authority:0,
        (uintptr_t)harness_router,(uintptr_t)harness_life,GetCurrentThreadId(),harness_thread,
        storage_permit(NULL),harness_router?harness_router->closed:99,
        harness_router?harness_router->domain.closed:99,harness_life?harness_life->closed:99,
        harness_life?(uintptr_t)harness_life->branch:0,
        harness_life&&harness_life->branch?harness_life->branch->closed:99,
        (uintptr_t)Kh02LifecycleCurrent(harness_life),active_authority};memcpy(out,v,sizeof(v));
}
#endif

/* The established branch history is scoped, authenticated and append-only.
   Reading accepted ownership is independent of permission for NEW updates.
   Callbacks still validate the current local pawn and load before calling here. */
uint32_t Kh02MickeyOwnership171(uint32_t *mask) {
    if(!mask || !harness_router || !harness_life ||
       GetCurrentThreadId()!=harness_thread || !storage_permit(NULL) ||
       harness_router->closed || harness_router->domain.closed || harness_life->closed ||
       !harness_life->branch || harness_life->branch->closed ||
       !Kh02LifecycleCurrent(harness_life))return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    if(branch->history_count>4096)return 0;
    uint32_t result=0;
    for(uint32_t i=0;i<branch->history_count;i++) {
        uint64_t id=branch->history[i].item;
        result|=id==0x020233?1:0;
    }
    *mask=result;return 1;
}
uint32_t Kh02MovementOwnership209(uint32_t *mask) {
    if(!mask || !harness_router || !harness_life ||
       GetCurrentThreadId()!=harness_thread || !storage_permit(NULL) ||
       harness_router->closed || harness_router->domain.closed || harness_life->closed ||
       !harness_life->branch || harness_life->branch->closed ||
       !Kh02LifecycleCurrent(harness_life))return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    if(branch->history_count>4096)return 0;
    uint32_t result=0;
    for(uint32_t i=0;i<branch->history_count;i++) {
        uint64_t id=branch->history[i].item;
        result|=id==0x020205?1:id==0x020206?2:id==0x020207?16:0;
    }
    *mask=result;return 1;
}
uint32_t Kh02StyleOwnership(uint32_t *mask) {
    if(!mask || !harness_router || !harness_life ||
       GetCurrentThreadId()!=harness_thread || !storage_permit(NULL) ||
       harness_router->closed || harness_router->domain.closed || harness_life->closed ||
       !harness_life->branch || harness_life->branch->closed ||
       !Kh02LifecycleCurrent(harness_life))return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    if(branch->history_count>4096)return 0;
    uint32_t result=0;
    for(uint32_t i=0;i<branch->history_count;i++) {
        uint64_t id=branch->history[i].item;
        result|=id==0x020208?1:0;
    }
    *mask=result;return 1;
}
uint32_t Kh02ActionOwnership(uint32_t *mask) {
    if(!mask || !harness_router || !harness_life ||
       GetCurrentThreadId()!=harness_thread || !storage_permit(NULL) ||
       harness_router->closed || harness_router->domain.closed || harness_life->closed ||
       !harness_life->branch || harness_life->branch->closed ||
       !Kh02LifecycleCurrent(harness_life))return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    if(branch->history_count>4096)return 0;
    uint32_t result=0;
    for(uint32_t i=0;i<branch->history_count;i++) {
        uint64_t id=branch->history[i].item;
        result|=id==0x020216?8:id==0x020217?4:id==0x020218?2:id==0x020219?1:0;
    }
    *mask=result;return 1;
}
uint32_t Kh02MagicOwnership(uint32_t *mask) {
    if(!mask || !harness_router || !harness_life ||
       GetCurrentThreadId()!=harness_thread || !storage_permit(NULL) ||
       harness_router->closed || harness_router->domain.closed || harness_life->closed ||
       !harness_life->branch || harness_life->branch->closed ||
       !Kh02LifecycleCurrent(harness_life))return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    if(branch->history_count>4096)return 0;
    uint32_t result=0,fire=0,blizzard=0,thunder=0;
    for(uint32_t i=0;i<branch->history_count;i++) {
        uint64_t id=branch->history[i].item;
        if(id==0x02021d)result|=++fire>=2?3:1;
        else if(id==0x02021e)result|=++blizzard>=2?12:4;
        else if(id==0x02021f)result|=++thunder>=2?48:16;
        else if(id==0x020204)result|=64;
    }
    *mask=result;return 1;
}
uint32_t Kh02StatOwnership(uint32_t *hp,uint32_t *mp) {
    if(!hp || !mp || !harness_router || !harness_life ||
       GetCurrentThreadId()!=harness_thread || !storage_permit(NULL) ||
       harness_router->closed || harness_router->domain.closed || harness_life->closed ||
       !harness_life->branch || harness_life->branch->closed || !Kh02LifecycleCurrent(harness_life))return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    if(branch->history_count>4096)return 0;
    *hp=*mp=0;
    for(uint32_t i=0;i<branch->history_count;i++) {
        if(branch->history[i].item==0x02020b)(*hp)++;
        else if(branch->history[i].item==0x02020c)(*mp)++;
    }
    return 1;
}
extern uint32_t Kh02StatReconcile(void);
extern uint32_t Kh02StatReceiptReady(uint64_t);
static uint32_t production_item_schema;
extern uint32_t Kh02MagicReceiptReady(uint64_t);
extern uint32_t Kh02ActionReceiptReady(uint64_t item);
extern uint32_t Kh02MovementReceiptReady209(uint64_t);
#include "kh02_stat_popup_transport_v111.inc"

static uint32_t policy_identity(const Kh02ReceiptIdentity *item,uint64_t revision,void *unused) {
    (void)revision;(void)unused;
    uint32_t status=item && ((item->item>=0x020212 && item->item<=0x020215) || Kh02StatReceiptReady(item->item) || Kh02ActionReceiptReady(item->item) || Kh02MovementReceiptReady209(item->item) || (production_item_schema==4 && (Kh02MagicReceiptReady(item->item) || Kh02StyleReceiptReady(item->item)))) ? KH02_MEMBER_OK:KH02_MEMBER_REFUSED;
    if(status==KH02_MEMBER_OK)Kh02StatPopupStageV111(&popup_queue_v111,item->item,Kh02LifecycleEpoch(harness_life),popup_cookie_v111());
    return status;
}
static uint32_t policy_commit(void *context,const unsigned char *proposal,uint32_t size) {
    return Kh02HistoryRouterExecute((Kh02SaveRouter*)context,proposal,size,policy_identity,NULL);
}
static uint32_t policy_apply(void *context,const Kh02PolicyProjection *projection) {
    (void)context;
    uint32_t hp,mp;
    if(!projection || !Kh02StatOwnership(&hp,&mp) || projection->hp_items!=hp || projection->mp_items!=mp ||
       projection->ability_mask || projection->spell_mask ||
       (projection->world_mask&~KH02_WORLD_MASK) || !controlled_transport_permit(NULL))
        return KH02_POLICY_REFUSED;
    if(!Kh02StatReconcile())return KH02_POLICY_REFUSED;
    accepted_projection=*projection;return controlled_transport_permit(NULL)?KH02_POLICY_OK:KH02_POLICY_REFUSED;
}
static uint32_t production_after_setup(uint32_t status) {
    Kh02ReceiptScope *scope;unsigned char binding[32];uint64_t challenge;
    if(status!=1)return status;
    if(!fresh_map || !harness_router || !harness_router->life || !harness_router->life->branch)return 2;
    scope=&harness_router->life->branch->scope;
    if(Kh02PolicyScopeBinding(scope->team,scope->slot,scope->seed,scope->seed_size,binding)!=1 ||
       Kh02PolicyProcessChallenge(&challenge)!=1 ||
       Kh02PolicyCommandBind(&production_owner,scope->cookie,challenge,binding,
           policy_apply,policy_commit,harness_router)!=1)return 2;
    if(difficulty_bind(scope->cookie,challenge,binding)!=1)return 2;
    fresh_map->policy_challenge=challenge;memcpy(fresh_map->policy_scope,binding,32);
    MemoryBarrier();fresh_map->policy_ready=1;return status;
}
static uint32_t production_command_service(void *raw,uint32_t command) {
    Kh02FreshTransportBlock *block=raw;Kh02PolicyCommand *copy;uint32_t status=2;
    (void)command;
    if(GetCurrentThreadId()!=harness_thread)return status;
    Kh02StatPopupBeginV111(&popup_queue_v111);
    if(!block->policy_ready || block->policy_reserved || !block->requested_authority ||
       block->requested_authority!=accepted_authority)return status;
    active_authority=block->requested_authority;
    if(!controlled_transport_permit(NULL)){active_authority=0;return status;}
    copy=VirtualAlloc(NULL,sizeof(*copy),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!copy){active_authority=0;return status;}memcpy(copy,&block->policy_command,sizeof(*copy));
    if(copy->live_authority==active_authority && controlled_transport_permit(NULL)) {
        production_item_schema=copy->policy.item_schema_version;
        status=Kh02PolicyCommandService(&production_owner,copy);
        Kh02StatPopupFinishV111(&popup_queue_v111,status==KH02_POLICY_OK,Kh02LifecycleEpoch(harness_life),popup_cookie_v111());
        popup_publish_v111();
        if(status==KH02_POLICY_OK)Kh02MeterReconcile();
        production_item_schema=0;
    }
    VirtualFree(copy,0,MEM_RELEASE);active_authority=0;return status;
}

static void difficulty_observation_snapshot(void *raw) {
    Kh02FreshTransportBlock *b=raw;static uint64_t observed_epoch;
    if(!b || b->transport.command!=3 || !harness_life || !Kh02LifecycleCurrent(harness_life) ||
       !b->requested_authority || b->requested_authority!=accepted_authority)return;
    active_authority=b->requested_authority;
    uint64_t epoch=Kh02LifecycleEpoch(harness_life);
    if(epoch && epoch!=observed_epoch && controlled_transport_permit(NULL)) {
        observed_epoch=epoch;
        harness_end(harness_begin(18),Kh02DifficultyObserveRoot(harness_read,NULL,harness_router->config.base));
    }
    active_authority=0;
}

#include "kh02_difficulty_transport_v21.inc"
#include "kh02_action_gate_transport_v33.inc"

#include "goal_config_v80.inc"

/* Versioned fixed-size startup policy on the existing authenticated channel.
 * It does not require a gameplay pawn and therefore can precede a Depths load. */
extern uint32_t Kh02MickeyConfigure172(uint64_t,uint32_t);
static uint32_t mickey_config_command172(void *raw) {
    Kh02FreshTransportBlock *b=raw;Kh02DifficultyRequest r;uint32_t status=2;
    if(!b || GetCurrentThreadId()!=harness_thread || !b->policy_ready || b->policy_reserved ||
       b->proposal_size!=sizeof(r) || !b->requested_authority ||
       b->requested_authority!=accepted_authority ||
       (uint64_t)InterlockedCompareExchange64(&b->authority,0,0)!=accepted_authority)return 2;
    memcpy(&r,b->proposal,sizeof(r));
    if(r.magic!=0x4b484d43 || r.version!=1 || r.size!=sizeof(r) || r.desired>2 ||
       r.sequence!=1 || r.epoch || r.cookie!=difficulty_pending.cookie ||
       r.challenge!=difficulty_pending.challenge || memcmp(r.scope,difficulty_pending.scope,32) ||
       r.authority!=accepted_authority)return 2;
    active_authority=b->requested_authority;
    /* Configuration only: title has no active gameplay record yet.
       Preserve authenticated storage/history scope, without permitting grants. */
    if(controlled_transport_permit(NULL) && harness_router && harness_life &&
       !harness_router->closed && !harness_router->domain.closed && !harness_life->closed &&
       harness_life->branch && !harness_life->branch->closed)
        status=Kh02MickeyConfigure172(r.cookie,r.desired);
    active_authority=0;return status;
}

extern uint32_t Kh02XpConfigure185(uint64_t,uint32_t);
static uint32_t xp_config_command185(void *raw) {
    Kh02FreshTransportBlock *b=raw;Kh02DifficultyRequest r;uint32_t status=2;
    if(!b || GetCurrentThreadId()!=harness_thread || !b->policy_ready || b->policy_reserved ||
       b->proposal_size!=sizeof(r) || !b->requested_authority ||
       b->requested_authority!=accepted_authority ||
       (uint64_t)InterlockedCompareExchange64(&b->authority,0,0)!=accepted_authority)return 2;
    memcpy(&r,b->proposal,sizeof(r));
    if(r.magic!=0x4b485850 || r.version!=1 || r.size!=sizeof(r) || (r.desired!=2 && r.desired!=3 && r.desired!=4 && r.desired!=6 && r.desired!=8) ||
       r.sequence!=1 || r.epoch || r.cookie!=difficulty_pending.cookie ||
       r.challenge!=difficulty_pending.challenge || memcmp(r.scope,difficulty_pending.scope,32) ||
       r.authority!=accepted_authority)return 2;
    active_authority=b->requested_authority;
    /* Configuration only: title has no active gameplay record yet.
       Preserve authenticated storage/history scope, without permitting grants. */
    if(controlled_transport_permit(NULL) && harness_router && harness_life &&
       !harness_router->closed && !harness_router->domain.closed && !harness_life->closed &&
       harness_life->branch && !harness_life->branch->closed)
        status=Kh02XpConfigure185(r.cookie,r.desired);
    active_authority=0;return status;
}

extern uint32_t Kh02TrapRequest242(void*,uint32_t,uint32_t,uint32_t*);
static uint32_t trap_command242(void *raw) {
    Kh02FreshTransportBlock *block=raw;uint32_t status=2,written=0;
    if(GetCurrentThreadId()!=harness_thread || !block->requested_authority || block->requested_authority!=accepted_authority)return status;
    active_authority=block->requested_authority;
    if(controlled_transport_permit(NULL))status=Kh02TrapRequest242(block->proposal,block->proposal_size,sizeof(block->proposal),&written);
    block->proposal_size=written;active_authority=0;return status;
}

extern uint32_t Kh02DeathRequest249(void*,uint32_t,uint32_t,uint32_t*);
static uint32_t death_command249(void *raw) {
    Kh02FreshTransportBlock *block=raw;uint32_t status=2,written=0;
    if(GetCurrentThreadId()!=harness_thread || !block->requested_authority || block->requested_authority!=accepted_authority)return status;
    active_authority=block->requested_authority;
    if(controlled_transport_permit(NULL))status=Kh02DeathRequest249(block->proposal,block->proposal_size,sizeof(block->proposal),&written);
    block->proposal_size=written;active_authority=0;return status;
}

/* Index-bound transient effects use authenticated history, never save membership. */
uint32_t Kh02TrapReceipt244(uint64_t id,uint32_t kind) {
    if(!id || id>4096 || (kind!=1 && kind!=2) || !Kh02ObjectiveAuthorityV116() ||
       !harness_life || !harness_life->branch || harness_life->branch->closed)return 0;
    Kh02DiskBranch *branch=harness_life->branch;
    return id<=branch->history_count && branch->history[id-1].item==(kind==1?0x020234ULL:0x020235ULL);
}
