#include "kh02_wardrobe_catalog_candidate.h"
typedef struct CatalogAward {
    Kh02ReceiptOwner *owner;
    Kh02GameGrantContext original,guarded;
    Kh02CosmeticAttempt *attempt;
    uint64_t sequence;
} CatalogAward;
static uint32_t current(void *context) {
    CatalogAward *a=context;
    return Kh02ReceiptOwnerCheck(a->owner)==KH02_MEMBER_OK &&
        a->original.active(a->original.active_context)==1 &&
        Kh02ReceiptOwnerCheck(a->owner)==KH02_MEMBER_OK;
}
static uint32_t admitted(const Kh02ReceiptIdentity *receipt,uint64_t cookie,void *context) {
    CatalogAward *a=context;Kh02WardrobeDescriptor descriptor;
    if(!Kh02WardrobeResolve(receipt->item,&descriptor) ||
       cookie!=a->original.session->gate.current.cookie || !current(a))return 0;
    /* Identity comes from the receipt owner's private validated proposal.
       Copy by value before invoking external guard/reader callbacks. */
    return Kh02WardrobeAwardOnce(&a->guarded,a->sequence,descriptor.item,a->attempt)==KH02_GATE_OK && current(a);
}
uint32_t Kh02WardrobeCatalogReceiptOnce(Kh02ReceiptOwner *owner,Kh02GameGrantContext *game,
    uint64_t sequence,const unsigned char *proposal,uint32_t size,Kh02CosmeticAttempt *attempt) {
    CatalogAward a;
    if(!owner || !game || !game->session || !game->active || !attempt || attempt->consumed || !sequence)return KH02_MEMBER_REFUSED;
    a.owner=owner;a.original=a.guarded=*game;a.attempt=attempt;a.sequence=sequence;
    a.guarded.active=current;a.guarded.active_context=&a;
    if(!current(&a))return KH02_MEMBER_REFUSED;
    return Kh02ReceiptOwnerExecuteIdentity(owner,proposal,size,admitted,&a);
}
