#ifndef KH02_PUBLICATION_GATE_CANDIDATE_H
#define KH02_PUBLICATION_GATE_CANDIDATE_H
#include "kh02_native_journal_candidate.h"
#include "kh02_frozen_save_candidate.h"
typedef struct Kh02PublicationGate Kh02PublicationGate;
typedef struct Kh02PublicationService Kh02PublicationService;
typedef unsigned char (*Kh02PublicationOriginal)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
enum { KH02_PUBLICATION_SAVED=1, KH02_PUBLICATION_SAVE_FAILED,
       KH02_PUBLICATION_JOURNAL_FAILED, KH02_PUBLICATION_CANCELLED,
       KH02_PUBLICATION_ALREADY_USED };
/* Offline candidate. One owner per attempt; creates its own connection to an
   already initialized isolated journal and copies validated bundle input.
   Original and its code/context must remain alive through Execute's return.
   The fifth original argument is replaced with the owned frozen descriptor.
   Creating a second owner is NOT deduplication across platform invocations. */
MEMBER_API Kh02PublicationGate *Kh02PublicationCreate(const char*,const unsigned char*,uint32_t,
    const unsigned char*,uint32_t,Kh02PublicationOriginal,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
/* Any joined worker, or synchronous owner-thread fallback. Consumed even on
   failure; no automatic retry. Never holds a lock across original. */
MEMBER_API uint32_t Kh02PublicationExecute(Kh02PublicationGate*);
/* Any thread with a held lifetime: blocks entry into original if it wins before
   save admission. Once admitted, original must finish; revocation cannot undo
   a game save. No timeout frees pending input/connection/code. */
MEMBER_API void Kh02PublicationRevoke(Kh02PublicationGate*);
/* Creating thread only. Revokes first; BUSY retains everything while Execute
   is pending. Final destruction REQUIRES external users joined/no new calls,
   including Revoke. Returns journal status; clears pointer only on success. */
MEMBER_API uint32_t Kh02PublicationClose(Kh02PublicationGate**);
/* Shared connection for the eventual hook adapter. Open before the live test.
   Up to 64 outstanding owners; admission/capacity failure refuses the save.
   CreateAttempt copies input on its calling thread; Close for those attempts
   belongs to the SERVICE creating thread, after worker users have joined.
   External flight accounting must hold the service through CreateAttempt. */
MEMBER_API Kh02PublicationService *Kh02PublicationServiceOpen(const char*);
MEMBER_API Kh02PublicationGate *Kh02PublicationCreateAttempt(Kh02PublicationService*,
    const unsigned char*,uint32_t,const unsigned char*,uint32_t,
    Kh02PublicationOriginal,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
/* Revokes all registered attempts, rejects new ones. BUSY retains connection
   until every attempt is explicitly closed; join all callers before final free.
   Creating thread only. Journal contention is refusal, never a host wait. */
MEMBER_API uint32_t Kh02PublicationServiceClose(Kh02PublicationService**);
#endif
