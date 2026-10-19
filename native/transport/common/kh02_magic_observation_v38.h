#ifndef KH02_MAGIC_V38_H
#define KH02_MAGIC_V38_H
#include <stdint.h>
/* Read-only, fixed local-pawn chain. No process search, writes, or native calls. */
typedef struct Kh02MagicRow {
    uint32_t command,flags,effect,voice;
    uint64_t projectile,trailing;
} Kh02MagicRow;
typedef struct Kh02MagicObservation {
    uint32_t status,count,capacity,reserved;
    uint64_t epoch,provider,equipment,equipment_vtable,magic,magic_vtable,records,replacement;
    Kh02MagicRow rows[7];
} Kh02MagicObservation;
_Static_assert(sizeof(Kh02MagicObservation)==304,"magic observation ABI");

uint32_t Kh02GrantOwnerMagicObservation(Kh02MagicObservation*);
#endif
