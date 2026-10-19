#ifndef KH02_MENU_GATE_V45_H
#define KH02_MENU_GATE_V45_H
#include <stdint.h>
typedef struct Kh02MenuObservation {
    uint64_t raw,reasons[9],accepted[7][7],native_true[7][7],blocked[7][7];
    uint64_t epoch,last_hud,last_vtable,last_actor,last_caller,last_command;
    uint64_t last_row,last_result,last_row_bytes,last_row_tail;
} Kh02MenuObservation;
void Kh02MenuObservationSnapshot(Kh02MenuObservation*);
#endif
