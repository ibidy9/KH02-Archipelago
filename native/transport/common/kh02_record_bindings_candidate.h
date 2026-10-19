#ifndef KH02_RECORD_BINDINGS_CANDIDATE_H
#define KH02_RECORD_BINDINGS_CANDIDATE_H
#include "kh02_membership_admission_candidate.h"
#define KH02_BIND_SIZE 0x1725b0u
#define KH02_BIND_RECORD 0x3b40u
typedef struct Kh02BindingStore Kh02BindingStore;
typedef struct Kh02BindingBundle {
    uint64_t cookie,serial;
    /* Zero means opaque/unassociated, never empty AP history. */
    uint64_t revisions[100];
    unsigned char container[KH02_BIND_SIZE];
} Kh02BindingBundle;
typedef struct Kh02BindingLease {const Kh02BindingBundle *data;uint64_t cookie,serial;} Kh02BindingLease;
enum { KH02_BIND_OK=1,KH02_BIND_REFUSED,KH02_BIND_BUSY,KH02_BIND_INVALID,KH02_BIND_CAPACITY };
/* Offline component. Inputs must already be owned exact bytes. No process reads.
   Create/Put/Refresh/Destroy belong to the creating thread. Capture/Release may
   run on workers. Destroy requires joined callers and no outstanding bundles. */
MEMBER_API Kh02BindingStore *Kh02BindingsCreate(uint64_t,const unsigned char*,uint32_t);
MEMBER_API uint32_t Kh02BindingsPut(Kh02BindingStore*,uint32_t,uint64_t,const unsigned char*,uint32_t);
MEMBER_API uint32_t Kh02BindingsRefresh(Kh02BindingStore*,uint32_t,const unsigned char*,const unsigned char*,uint32_t);
MEMBER_API uint32_t Kh02BindingsCapture(Kh02BindingStore*,const unsigned char*,uint32_t,Kh02BindingLease*);
MEMBER_API uint32_t Kh02BindingsRelease(Kh02BindingStore*,Kh02BindingLease*);
MEMBER_API uint32_t Kh02BindingsInvalidate(Kh02BindingStore*);
MEMBER_API uint32_t Kh02BindingsDestroy(Kh02BindingStore**);
#endif
