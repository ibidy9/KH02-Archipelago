#ifndef KH02_CANONICAL_ASSOCIATION_CANDIDATE_H
#define KH02_CANONICAL_ASSOCIATION_CANDIDATE_H
#include "kh02_receipt_bindings_candidate.h"
typedef struct Kh02CanonicalPart {uint32_t offset,size;} Kh02CanonicalPart;
typedef struct Kh02CanonicalAssociation {
    uint32_t size,set_count;
    Kh02CanonicalPart token,container,scope,association;
    Kh02CanonicalPart sets[64];
    /* UINT32_MAX means unknown/unassociated. Other values index sets[]. */
    uint32_t records[100];
    unsigned char binding[32];
} Kh02CanonicalAssociation;
/* Offline native encoder only. Input must be an owned copy of a validated
   native bundle. Rechecks framing/scope, not server history or live provenance.
   Output is one owned read-only allocation, independent of all input buffers. */
MEMBER_API const Kh02CanonicalAssociation *Kh02CanonicalCreate(const unsigned char*,uint32_t,const unsigned char*,uint32_t);
/* Join consumers before destruction; no native persistence or save permission. */
MEMBER_API uint32_t Kh02CanonicalDestroy(const Kh02CanonicalAssociation**);
#endif
