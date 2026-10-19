#ifndef KH02_SAVE_HEADER_BINDINGS_CANDIDATE_H
#define KH02_SAVE_HEADER_BINDINGS_CANDIDATE_H
#include "kh02_receipt_bindings_candidate.h"
/* Successful native construction only. Owner-thread atomic record/header pair.
   Header bytes have no receipt revision; every opaque record stays untouched. */
MEMBER_API uint32_t Kh02BindingsPutHeader(Kh02BindingStore*,uint32_t,uint64_t,
    const unsigned char*,uint32_t,const unsigned char*,uint32_t);
MEMBER_API uint32_t Kh02ReceiptBindingsPutHeader(Kh02ReceiptBindings*,uint32_t,
    Kh02ReceiptOwner*,const Kh02ReceiptLease*,const unsigned char*,uint32_t,const unsigned char*,uint32_t);
#endif
