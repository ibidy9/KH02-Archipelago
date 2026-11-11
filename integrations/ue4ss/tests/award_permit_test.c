#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../../native/transport/common/kh02_wardrobe_catalog_candidate.h"
#include "../native/registration_api.h"
typedef struct Award {
 Kh02GameGrantContext *game;Kh02CosmeticAttempt *attempt;
 Kh02Lease lease;Kh02WardrobeDescriptor descriptor;
} Award;
static Kh02CosmeticSnapshot observed;
static unsigned ready=1,entered_calls,fail_after;
static uint32_t valid(Award *a,Kh02CosmeticSnapshot *snapshot) {
 assert(a && a->game && a->attempt);*snapshot=observed;return ready;
}
#include "../native/registration_award.inc"
uint32_t Kh02UE4SSRegistrationCall(Kh02RegistrationRequest *p) {
 assert(p->size==sizeof(*p) && p->version==1 && p->pawn==0x10000 && p->context==0x20000);
 assert(p->code==0x600 && p->show==1 && !p->entered);
 if(!p->permit(p->owner_context))return 0;
 ++entered_calls;p->entered=1;observed.primary.items[0][0]=1;
 assert(p->permit(p->owner_context)); // Original snapshot may change after entry.
 if(fail_after){p->fault=1;return 0;}
 return 1;
}
int main(void) {
 Kh02SessionCandidate session={0};Kh02GameGrantContext game={0};Kh02CosmeticAttempt attempt={0};
 Award a={0};game.session=&session;session.context=0x20000;
 a.game=&game;a.attempt=&attempt;a.lease.provider=0x10000;a.descriptor.code=0x600;
 assert(registration_award(&a)==1 && attempt.entered==1 && entered_calls==1);
 memset(&observed,0,sizeof(observed));memset(&attempt,0,sizeof(attempt));
 observed.primary.items[1][0]=1;
 assert(!registration_award(&a) && !attempt.entered && entered_calls==1); // Changed before entry.
 memset(&observed,0,sizeof(observed));ready=0;
 assert(!registration_award(&a) && !attempt.entered && entered_calls==1);
 ready=1;fail_after=1;
 assert(!registration_award(&a) && attempt.entered==1 && entered_calls==2);
 puts("PASS original-snapshot permit, post-entry mutation, revocation and uncertain entry propagation");
 return 0;
}
