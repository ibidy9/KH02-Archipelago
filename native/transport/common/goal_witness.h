#ifndef KH02_GOAL_WITNESS_H
#define KH02_GOAL_WITNESS_H
#include <stdint.h>
#include <string.h>

/* Internal native input only, never a host-supplied authorization command.
 * Producer must validate the current shared owner before setting admitted.
 * No game writes, game calls, allocations, hooks or network effects here. */
typedef struct Kh02GoalFrame {
    uint64_t cookie, epoch, load_id, owner;
    uint32_t admitted, objective42, tide_bonus, in_tide, selected_goal;
} Kh02GoalFrame;

typedef struct Kh02GoalWitness {
    uint64_t cookie, epoch, load_id, owner, sample_number;
    uint64_t phantom_sample, tide_sample;
    uint32_t armed, completed, selected_goal;
} Kh02GoalWitness;

/* A read/authority gap invalidates incomplete transitions. Already-witnessed
 * completion is retained under its ORIGINAL identity for host journaling.
 * The enclosing owner must not relabel that proof after load/scope changes. */
static void Kh02GoalWitnessGap(Kh02GoalWitness *state) {
    state->armed = 0;
}

static uint32_t Kh02GoalWitnessStep(Kh02GoalWitness *state, const Kh02GoalFrame *frame) {
    uint32_t phantom, tide;
    if (!state || !frame) return 0;
    if (frame->admitted != 1 || !frame->cookie || !frame->epoch ||
        !frame->load_id || !frame->owner || frame->objective42 > 2 ||
        frame->tide_bonus > 255 || frame->in_tide > 1 ||
        (frame->selected_goal != 1 && frame->selected_goal != 2)) {
        Kh02GoalWitnessGap(state);
        return 0;
    }
    if (state->cookie != frame->cookie || state->epoch != frame->epoch ||
        state->load_id != frame->load_id || state->owner != frame->owner ||
        state->selected_goal != frame->selected_goal) {
        /* A pending proof is never overwritten/relabelled by a new load. Return
         * 2 means journal the ORIGINAL proof; it is not admission of this frame. */
        if (state->completed) { Kh02GoalWitnessGap(state); return 2; }
        memset(state, 0, sizeof(*state));
        state->cookie = frame->cookie;
        state->epoch = frame->epoch;
        state->load_id = frame->load_id;
        state->owner = frame->owner;
        state->selected_goal = frame->selected_goal;
    }
    if (state->sample_number == UINT64_MAX) {
        Kh02GoalWitnessGap(state);
        return 0;
    }
    ++state->sample_number;
    phantom = frame->objective42;
    tide = (frame->tide_bonus >> 7) & 1;
    if (frame->selected_goal == 1 && !(state->completed & 1)) {
        if (phantom == 1) state->armed |= 1;
        else {
            if (phantom == 2 && (state->armed & 1)) {
                state->completed |= 1;
                state->phantom_sample = state->sample_number;
            }
            state->armed &= ~1u;
        }
    }
    if (frame->selected_goal == 2 && !(state->completed & 2)) {
        if (!frame->in_tide) state->armed &= ~2u;
        else if (!tide) state->armed |= 2;
        else {
            if (state->armed & 2) {
                state->completed |= 2;
                state->tide_sample = state->sample_number;
            }
            state->armed &= ~2u;
        }
    }
    return 1;
}
#endif
