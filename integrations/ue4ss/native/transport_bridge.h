#ifndef KH02_UE4SS_TRANSPORT_BRIDGE_H
#define KH02_UE4SS_TRANSPORT_BRIDGE_H
#include "bridge_api.h"
#include "reflected_grant.h"
/* Internal trusted owner-thread entry. Packet, adapter and request must all
   outlive the synchronous receipt operation; none may be shared/mapped input. */
uint32_t Kh02UE4SSTransportAttach(Kh02ReceiptGrant *, uint64_t, Kh02ReflectedGrantAdapter *,
                                  Kh02UE4SSRequest *);
#endif
