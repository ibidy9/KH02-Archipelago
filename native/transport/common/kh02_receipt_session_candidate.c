#include "kh02_receipt_session_candidate.h"
typedef struct Attempt {
    Kh02ReceiptOwner *owner;
    Kh02SessionCandidate *session;
    Kh02Lease lease;
    Kh02SessionSample sample;
    uint64_t sequence;
    Kh02ReceiptDispatch dispatch;
    void *context;
    Kh02ReceiptIdentity receipt;
    uint32_t battle_id;
} Attempt;
static int battle_id(uint64_t item,uint32_t *out) {
    /* Matches CONSUMABLES/AP IDs and the proven seven-entry native inventory. */
    switch(item) {
        case 0x020209:*out=0;return 1; /* Potion */
        case 0x02020D:*out=1;return 1; /* Hi-Potion */
        case 0x02020E:*out=2;return 1; /* Mega-Potion */
        case 0x02020A:*out=3;return 1; /* Ether */
        case 0x02020F:*out=4;return 1; /* Mega-Ether */
        case 0x020210:*out=5;return 1; /* Elixir */
        case 0x020211:*out=6;return 1; /* Megalixir */
        default:return 0;
    }
}
static uint32_t dispatch(void *context) {
    Attempt *a=context;uint32_t result;
    if(Kh02ReceiptOwnerCheck(a->owner)!=KH02_MEMBER_OK)return 0;
    result=a->dispatch(a->battle_id,&a->receipt,a->context);
    /* Receipt revocation must also make the existing session uncertain, even
       if the game-facing callback incorrectly reports success after a stage. */
    return result==1 && Kh02ReceiptOwnerCheck(a->owner)==KH02_MEMBER_OK;
}
static uint32_t admitted(const Kh02ReceiptIdentity *receipt,uint64_t cookie,void *context) {
    Attempt *a=context;
    if(cookie!=a->session->gate.current.cookie || cookie!=a->lease.cookie || !battle_id(receipt->item,&a->battle_id))return 0;
    a->receipt=*receipt;
    return Kh02SessionExecute(a->session,&a->lease,a->sequence,&a->sample,dispatch,a)==KH02_GATE_OK;
}
uint32_t Kh02ReceiptSessionExecute(Kh02ReceiptOwner *owner,Kh02SessionCandidate *session,const Kh02Lease *lease,uint64_t sequence,const Kh02SessionSample *sample,const unsigned char *proposal,uint32_t size,Kh02ReceiptDispatch effect,void *context) {
    Attempt a={0};
    if(!owner || !session || !lease || !sample || !effect)return KH02_MEMBER_REFUSED;
    /* Copy request fields before a reentrant call can mutate caller storage.
       Session fields still belong exclusively to the verified owner thread. */
    a.owner=owner;a.session=session;a.lease=*lease;a.sample=*sample;
    a.sequence=sequence;a.dispatch=effect;a.context=context;
    return Kh02ReceiptOwnerExecuteIdentity(owner,proposal,size,admitted,&a);
}
