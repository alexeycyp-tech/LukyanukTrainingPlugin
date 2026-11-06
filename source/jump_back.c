#include <math.h>
#include <string.h>
#include "jump_back.h"

void jump_back_clear(JumpBackState *state) {
    if(state) memset(state,0,sizeof(*state));
}

static int checkpoint_distance_index(const float *distances,unsigned count,float distance,float *index) {
    unsigned lo=0,hi=count;
    if(!distances || !index || count<2 || !isfinite(distance) || distance<distances[0] || distance>distances[count-1]) return 0;
    while(lo<hi) {
        unsigned mid=lo+(hi-lo)/2;
        if(distances[mid]<distance) lo=mid+1;
        else hi=mid;
    }
    if(lo<count && distances[lo]==distance) { *index=(float)lo; return 1; }
    if(lo==0 || lo>=count || !(distances[lo-1]<distance && distance<distances[lo])) return 0;
    *index=(float)(lo-1)+(distance-distances[lo-1])/(distances[lo]-distances[lo-1]);
    return isfinite(*index);
}

static int append_point(JumpBackState *state,const float *distances,unsigned count,float distance) {
    float index=0;
    if(!state || state->point_count>=JUMP_BACK_MAX_POINTS || count<2 ||
       !isfinite(distance) || distance<distances[0]+5.0f || distance>distances[count-1]-5.0f ||
       !checkpoint_distance_index(distances,count,distance,&index)) return 0;
    if(state->point_count && distance-state->points[state->point_count-1].distance<0.1f) return 0;
    state->points[state->point_count].distance=distance;
    state->points[state->point_count].index=index;
    state->point_count++;
    return 1;
}

int jump_back_build(JumpBackState *state,const Sample *sample,const float *distances,unsigned count) {
    if(!state || !sample || !distances || !sample->active || !sample->car || !sample->movement || !sample->line ||
       count<2 || sample->count!=count || !isfinite(sample->distance)) return JUMP_BACK_BUILD_NONE;
    for(unsigned i=0;i<count;i++) if(!isfinite(distances[i]) || (i && distances[i]<distances[i-1])) return JUMP_BACK_BUILD_NONE;
    if(state->initialized && state->car==sample->car && state->movement==sample->movement &&
       state->line==sample->line && state->node_count==count && state->generation==sample->generation)
        return JUMP_BACK_BUILD_NONE;

    jump_back_clear(state);
    state->initialized=1;
    state->car=sample->car;
    state->movement=sample->movement;
    state->line=sample->line;
    state->node_count=count;
    state->generation=sample->generation;

    unsigned added=0;
    float start=distances[0],end=distances[count-1];
    if(append_point(state,distances,count,start+5.0f)) added++;
    for(float target=start+JUMP_BACK_SPACING_METERS;
        target<=end-5.0f && state->point_count<JUMP_BACK_MAX_POINTS;
        target+=JUMP_BACK_SPACING_METERS) {
        if(append_point(state,distances,count,target)) added++;
        if(target+JUMP_BACK_SPACING_METERS<=target) break;
    }
    return added?JUMP_BACK_BUILD_CONTEXT_RESET|JUMP_BACK_BUILD_POINTS_READY:JUMP_BACK_BUILD_CONTEXT_RESET;
}

int jump_back_choose(const JumpBackState *state,float current_distance,unsigned *point_index) {
    if(!state || !point_index || !state->initialized || !state->point_count || !isfinite(current_distance)) return 0;
    if(state->last_jump_valid) {
        float next_grid_point=state->last_jump_distance+JUMP_BACK_SPACING_METERS;
        if(current_distance<=next_grid_point+0.25f) {
            if(state->last_jump_index>0) {
                unsigned previous=state->last_jump_index-1;
                if(state->points[previous].distance<current_distance-0.25f) {
                    *point_index=previous;
                    return 1;
                }
            }
        }
    }
    unsigned found=JUMP_BACK_MAX_POINTS;
    for(unsigned i=0;i<state->point_count;i++) {
        if(state->points[i].distance<current_distance-0.25f) found=i;
        else break;
    }
    if(found==JUMP_BACK_MAX_POINTS) return 0;
    *point_index=found;
    return 1;
}

const JumpBackPoint *jump_back_get(const JumpBackState *state,unsigned point_index) {
    if(!state || point_index>=state->point_count) return 0;
    return &state->points[point_index];
}

void jump_back_commit(JumpBackState *state,unsigned point_index) {
    if(!state || point_index>=state->point_count) return;
    state->last_jump_index=point_index;
    state->last_jump_distance=state->points[point_index].distance;
    state->last_jump_valid=1;
}
