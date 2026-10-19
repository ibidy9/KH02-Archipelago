/* Additive development gate. No spell table, save, MP or inventory writes. */
#include "kh02_magic_predicate_v41.h"
static Kh02MagicPredicate magic_predicate;
extern uint32_t Kh02MagicOwnership(uint32_t*);
extern uint32_t Kh02ActionGatePermit(uint64_t);
static void magic_inc(uint64_t *p) {
    LONG64 old;
    do { old=InterlockedCompareExchange64((volatile LONG64*)p,0,0);
         if(old==INT64_MAX)return;
    } while(InterlockedCompareExchange64((volatile LONG64*)p,old+1,old)!=old);
}
static void magic_epoch(void) {
    if(magic_predicate.epoch!=seen_epoch) {
        magic_predicate.epoch=seen_epoch;magic_predicate.observed=0;
        magic_predicate.enabled=0;magic_predicate.mask=127;
    }
}
uint32_t Kh02MagicGateSet(uint64_t epoch,uint64_t authority,uint32_t mask,uint64_t sequence) {
    (void)epoch;(void)authority;(void)mask;(void)sequence;return 2;
}
uint32_t Kh02MagicReceiptReady(uint64_t item) {
    uint32_t owned=0,bit=item==0x02021d?1:item==0x02021e?4:item==0x02021f?16:item==0x020204?64:0;
    return bit && is_active(NULL) && session.context && session.world &&
        session.gate.current.observation && Kh02MagicOwnership(&owned) && (owned&bit);
}
uint32_t kh02_magic_predicate_capture(void *receiver,uint32_t grand,void *ret) {
    uint64_t actor=(uintptr_t)receiver>=0x450?(uintptr_t)receiver-0x450:0;
    uint64_t vt=0,ivt=0,fn=0,again=0,caller=(uintptr_t)ret;
    uint32_t command=0,reason=0,blocked=0,index=7;
    static const uint32_t commands[7]={0x33,0x34,0x37,0x38,0x3b,0x3c,0x47};
    Kh02MagicObservation table;
    InterlockedIncrement(&flight);magic_inc(&magic_predicate.raw);
    if(GetCurrentThreadId()!=owner_thread){reason=1;goto done;}
    if(!is_active(NULL)){reason=2;goto done;}
    if(!session.context || !session.world || !session.gate.current.observation){reason=3;goto done;}
    magic_epoch();
    if(caller!=game.base+0x519852){reason=4;goto done;}
    if(actor!=session.gate.current.provider || !diagnostic_pointer(actor) ||
       !read_local(NULL,actor,&vt,8) || !read_local(NULL,(uintptr_t)receiver,&ivt,8)) {reason=5;goto done;}
    if((vt!=game.base+0x3735bc8 && vt!=game.base+0x373d490) ||
       ivt!=game.base+(vt==game.base+0x3735bc8?0x3737f60:0x373f828) ||
       !read_local(NULL,ivt+0x158,&fn,8) || fn!=game.base+0x5115d0 ||
       !read_local(NULL,vt+0x1878,&fn,8) || fn!=game.base+0x4f6e40) {reason=6;goto done;}
    if(!read_local(NULL,actor+0xbb0,&command,4)){reason=5;goto done;}
    for(uint32_t i=0;i<7;i++)if(commands[i]==command){index=i;break;}
    if(index==7 || (grand&255)!=(index==1 || index==3 || index==5)){reason=7;goto done;}
    if(Kh02GrantOwnerMagicObservation(&table)!=1 || table.provider!=actor || table.replacement){reason=8;goto done;}
    if(!read_local(NULL,actor,&again,8) || again!=vt ||
       !read_local(NULL,actor+0xbb0,&again,4) || (uint32_t)again!=command){reason=9;goto done;}
    magic_predicate.last_receiver=(uintptr_t)receiver;magic_predicate.last_actor=actor;
    magic_predicate.last_caller=caller-game.base;magic_predicate.last_command=command;magic_predicate.last_grand=grand&255;
    magic_inc(&magic_predicate.accepted[index]);magic_predicate.observed|=1ULL<<index;
    uint32_t owned=0;
    if(Kh02MagicOwnership(&owned) && !(owned&(1u<<index))) {
        magic_inc(&magic_predicate.blocked[index]);blocked=1;
    }
done:
    magic_inc(&magic_predicate.reasons[reason]);InterlockedDecrement(&flight);return blocked;
}
void Kh02MagicPredicateSnapshot(Kh02MagicPredicate *out) {
    if(!out)return;
    if(GetCurrentThreadId()==owner_thread) {
        uint32_t owned=0;magic_epoch();magic_predicate.enabled=Kh02MagicOwnership(&owned);magic_predicate.mask=owned;
    }
    memcpy(out,&magic_predicate,sizeof(*out));
}
