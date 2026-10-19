#ifndef KH02_INVENTORY_CANDIDATE_H
#define KH02_INVENTORY_CANDIDATE_H
#include "kh02_owner_reader.h"
typedef struct Kh02InventorySnapshot {
    uint64_t owner,primary,reference;
    uint32_t counts[7],reference_counts[7];
} Kh02InventorySnapshot;
KH02_GATE_API uint32_t Kh02ReadInventory(Kh02BoundedRead,void*,uint64_t,Kh02InventorySnapshot*);
#endif
