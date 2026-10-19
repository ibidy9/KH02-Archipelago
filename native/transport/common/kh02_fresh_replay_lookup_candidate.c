/* Expose existing validated empty construction only in this fresh candidate. */
#include "kh02_history_lookup_candidate.c"
KH02_RECEIPT_API Kh02DiskLookup *Kh02DiskLookupCreateEmpty(const Kh02ReceiptScope *scope,
    const Kh02ReceiptIdentity *history,uint32_t count,const uint64_t *ids,uint32_t id_count) {
    return create_empty(scope,history,count,ids,id_count);
}
