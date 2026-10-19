#include "kh02_receipt_grant_candidate.h"
#include <string.h>
static int valid(Kh02ReceiptGrant *p,Kh02InventorySnapshot *inventory) {
    Kh02SessionSample sample={0};
    if(Kh02ReceiptOwnerCheck(p->owner)!=KH02_MEMBER_OK || p->check(p->context)!=1 ||
       p->session->stopped || p->session->uncertain || !p->session->gate.ready ||
       memcmp(&p->session->gate.current,&p->lease,sizeof(p->lease)))return 0;
    if(p->read(p->context,&sample,inventory)!=1 || sample.guards_verified!=1 || sample.reserved ||
       sample.generation!=p->lease.generation || sample.phase!=3 || sample.title_flags ||
       sample.pending_map || (sample.travel_flags&1) || sample.world_flag ||
       sample.context!=p->session->context || sample.world!=p->session->world ||
       sample.state!=p->lease.state || sample.provider!=p->lease.provider || sample.attributes!=p->lease.attributes ||
       inventory->owner!=p->before.owner || inventory->primary!=p->before.primary)return 0;
    /* A read/check adapter is also an external callback: revalidate afterward. */
    return p->check(p->context)==1 && Kh02ReceiptOwnerCheck(p->owner)==KH02_MEMBER_OK &&
       !p->session->stopped && !p->session->uncertain && p->session->gate.ready &&
       !memcmp(&p->session->gate.current,&p->lease,sizeof(p->lease));
}
static uint32_t guarded_execute(uint32_t battle,void *context) {
    Kh02ReceiptGrant local,*output=context;Kh02ReceiptItemName name={0};Kh02InventorySnapshot observed;
    uint32_t i;
    if(!output || battle>6)return 0;
    local=*output;output->entered=0;output->observed_before=output->observed_after=UINT32_MAX;
    if(!local.owner || !local.session || !local.check || !local.read || !local.convert || !local.grant || local.show>1 ||
       local.before.counts[battle]>=99)return 0;
    output->observed_before=local.before.counts[battle];
    if(!valid(&local,&observed) || memcmp(observed.counts,local.before.counts,sizeof(observed.counts)))return 0;
    if(local.convert(local.context,battle,&name)!=1 || !name.index)return 0;
    /* Membership and existing session validity must both survive conversion.
       The proven native grant remains the sole writer; no raw count patch. */
    if(!valid(&local,&observed) || memcmp(observed.counts,local.before.counts,sizeof(observed.counts)))return 0;
    output->entered=1;
    local.grant(local.context,&name,local.show);
    if(!valid(&local,&observed))return 0;
    for(i=0;i<7;i++)if(i!=battle && observed.counts[i]!=local.before.counts[i])return 0;
    output->observed_after=observed.counts[battle];
    return observed.counts[battle]==local.before.counts[battle]+1;
}

uint32_t Kh02ReceiptGrantExecute(uint32_t battle,const Kh02ReceiptIdentity *receipt,void *context) {
    if(!receipt)return 0;
    return guarded_execute(battle,context);
}
/* Local consumables do not create or mutate AP receipt identities. */
uint32_t Kh02LocalConsumableGrant193(uint32_t battle,Kh02ReceiptGrant *packet) {
    uint32_t verified,result;Kh02Lease lease;Kh02SessionCandidate *s;
    if((battle!=0 && battle!=3) || !packet || !(s=packet->session) ||
       s->stopped || s->uncertain || s->gate.busy || packet->before.counts[battle]>=99)return 0;
    lease=packet->lease;
    if(Kh02GateConfirm(&s->gate,&lease)!=KH02_GATE_OK ||
       Kh02GateBegin(&s->gate,&lease)!=KH02_GATE_OK)return 0;
    verified=guarded_execute(battle,packet);
    result=Kh02GateFinish(&s->gate,&lease,verified);
    s->gate.ready=0;
    if(result!=KH02_GATE_OK)s->uncertain=1;
    return result==KH02_GATE_OK;
}
