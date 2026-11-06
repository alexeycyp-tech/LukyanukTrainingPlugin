#ifndef LUKYANUK_JUMP_BACK_H
#define LUKYANUK_JUMP_BACK_H

#include "core.h"

#define JUMP_BACK_SPACING_METERS 250.0f
#define JUMP_BACK_MAX_POINTS 1024u

typedef struct {
    float distance;
    float index;
} JumpBackPoint;

typedef struct {
    uintptr_t car, movement, line;
    unsigned generation, node_count;
    JumpBackPoint points[JUMP_BACK_MAX_POINTS];
    unsigned point_count;
    unsigned last_jump_index;
    float last_jump_distance;
    int initialized, last_jump_valid;
} JumpBackState;

enum {
    JUMP_BACK_BUILD_NONE=0,
    JUMP_BACK_BUILD_CONTEXT_RESET=1,
    JUMP_BACK_BUILD_POINTS_READY=2
};

void jump_back_clear(JumpBackState *state);
int jump_back_build(JumpBackState *state,const Sample *sample,const float *distances,unsigned count);
int jump_back_choose(const JumpBackState *state,float current_distance,unsigned *point_index);
const JumpBackPoint *jump_back_get(const JumpBackState *state,unsigned point_index);
void jump_back_commit(JumpBackState *state,unsigned point_index);

#endif
