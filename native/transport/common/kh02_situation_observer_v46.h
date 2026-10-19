#ifndef KH02_SITUATION_OBSERVER_V46_H
#define KH02_SITUATION_OBSERVER_V46_H
#include <stdint.h>
typedef struct Kh02SituationObservation {
    uint64_t raw,reasons[8],accepted[7][4],native_true[7][4];
    uint64_t epoch,hud,controller,controller_vtable,actor,character_kind;
    uint64_t command,caller,row,result,ability_primary,ability_reference;
    uint64_t context,world,inventory_owner;
} Kh02SituationObservation;
void Kh02SituationObservationSnapshot(Kh02SituationObservation*);
#endif
