#ifndef KH02_NATIVE_JOURNAL_CANDIDATE_H
#define KH02_NATIVE_JOURNAL_CANDIDATE_H
#include "kh02_canonical_association_candidate.h"
typedef struct Kh02NativeJournal Kh02NativeJournal;
enum { KH02_JOURNAL_DURABLE=1,KH02_JOURNAL_REFUSED,KH02_JOURNAL_BUSY,KH02_JOURNAL_FAILED,KH02_JOURNAL_CLOSED };
/* Open an already initialized isolated reference-schema database, while the
   game is closed. UTF-8 path must be NUL-terminated. Never creates or migrates.
   Current prototype requires the audited system SQLite 3.43.2 library. */
MEMBER_API Kh02NativeJournal *Kh02NativeJournalOpen(const char*);
/* Prepared input must come from CanonicalCreate and remain held until return.
   No Python callbacks in Normal build. FULL disk synchronization can stall.
   Digest output is a caller-owned 65-byte buffer; only DURABLE fills it. */
MEMBER_API uint32_t Kh02NativeJournalAppend(Kh02NativeJournal*,const Kh02CanonicalAssociation*,char*);
/* Creating thread only; join users before final free. Close revokes new writes;
   BUSY retains the connection while a transaction finishes. */
MEMBER_API uint32_t Kh02NativeJournalClose(Kh02NativeJournal**);
#endif
