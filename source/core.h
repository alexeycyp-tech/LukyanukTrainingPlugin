#ifndef BOOKMARK_CORE_H
#define BOOKMARK_CORE_H
#include <math.h>
#include <stdint.h>

#define POSITION_TOLERANCE_METERS 0.50f
#define ORIENTATION_TOLERANCE_DEGREES 45.0f
#define POSE_Z_FEEDBACK_TOLERANCE_METERS 0.04f
#define TIMER_BACKWARD_DISTANCE_JUMP_METERS 5.0f
#define TIMER_BACKWARD_TIME_JUMP_SECONDS 0.1f
#define TIMER_FORWARD_DISTANCE_JUMP_METERS 30.0f

typedef struct {
    uintptr_t car, movement, line;
    unsigned generation, count;
    int mode, active, state, drivable, foreground;
    int gear, gear_valid;
    float distance, progress, speed, vector_speed, time, countdown;
    float x, y, z;
    float velocity[3];
    float q[4];
} Sample;
typedef struct { int valid; Sample sample; float target_index; } Mark;
typedef struct { int enabled, versions, log_ok, thread_ok, runtime_ok; } Gates;
typedef struct { float distance, elapsed; } TimerPoint;

static int valid_gear(int gear) { return gear>=0 && gear<=7; }

/* Raw RBR gear values include neutral below first. A sequential upshift can
 * reach saved gear N only if we stage the preceding forward gear N-1. */
static int sequential_stage_gear(int saved,int *stage) {
    if(!stage || saved<3 || saved>7) return 0;
    *stage=saved-1;
    return valid_gear(*stage);
}

static int finite_sample(const Sample *s) {
    return isfinite(s->distance) && isfinite(s->progress) && isfinite(s->speed)
        && isfinite(s->vector_speed) && isfinite(s->time) && isfinite(s->countdown)
        && isfinite(s->x) && isfinite(s->y) && isfinite(s->z)
        && isfinite(s->velocity[0]) && isfinite(s->velocity[1]) && isfinite(s->velocity[2]);
}
static int quaternion_valid(const float q[4]) {
    float n=0;
    for(unsigned i=0;i<4;i++) { if(!isfinite(q[i])) return 0; n+=q[i]*q[i]; }
    return isfinite(n) && n>=0.9f && n<=1.1f;
}
static int normalized_quaternion(const float in[4],float out[4]) {
    if(!quaternion_valid(in)) return 0;
    float n=sqrtf(in[0]*in[0]+in[1]*in[1]+in[2]*in[2]+in[3]*in[3]);
    for(unsigned i=0;i<4;i++) out[i]=in[i]/n;
    return 1;
}
static float rotation_error_degrees(const float a[4],const float b[4]) {
    if(!quaternion_valid(a) || !quaternion_valid(b)) return NAN;
    double aa=0,bb=0,ab=0;
    for(unsigned i=0;i<4;i++) { aa+=(double)a[i]*a[i]; bb+=(double)b[i]*b[i]; ab+=(double)a[i]*b[i]; }
    double dot=fabs(ab/sqrt(aa*bb));
    if(dot>1) dot=1;
    return (float)(acos(dot)*114.591559026);
}
static int orientation_within_tolerance(float error_degrees) {
    return isfinite(error_degrees) && error_degrees<=ORIENTATION_TOLERANCE_DEGREES;
}
static int timer_sample_discontinuous(float distance_delta,float time_delta) {
    return distance_delta < -TIMER_BACKWARD_DISTANCE_JUMP_METERS
        || time_delta < -TIMER_BACKWARD_TIME_JUMP_SECONDS
        || distance_delta > TIMER_FORWARD_DISTANCE_JUMP_METERS;
}
static int same_context(const Sample *a, const Sample *b) {
    return a->generation==b->generation && a->car==b->car && a->movement==b->movement
        && a->line==b->line && a->count==b->count;
}
static int same_context_objects(const Sample *a, const Sample *b) {
    return a->car==b->car && a->movement==b->movement && a->line==b->line
        && a->count==b->count;
}
/* Exact points and duplicate distances are handled without extrapolation. */
static int distance_index(const float *d, unsigned n, float m, float *idx) {
    if (!d || n<2 || !isfinite(m)) return 0;
    for (unsigned i=0;i<n;i++) if (!isfinite(d[i]) || (i && d[i]<d[i-1])) return 0;
    if (m<d[0] || m>d[n-1]) return 0;
    for (unsigned i=0;i<n;i++) {
        if (m==d[i]) { *idx=(float)i; return 1; }
        if (i+1<n && d[i]<m && m<d[i+1]) {
            *idx=(float)i+(m-d[i])/(d[i+1]-d[i]); return 1;
        }
    }
    return 0;
}
static int key_edge(int down, int *previous) {
    int edge=down && !*previous; *previous=down; return edge;
}
/* Return the same rejection reason used by timer_profile_append so runtime
 * diagnostics can distinguish a full profile from non-monotonic samples. */
static const char *timer_profile_rejection_reason(const TimerPoint *points,unsigned *count,unsigned capacity,float distance,float elapsed) {
    if(!points || !count || !capacity) return "invalid_profile_arguments";
    if(!isfinite(distance)) return "non_finite_distance";
    if(!isfinite(elapsed)) return "non_finite_elapsed";
    if(elapsed<0) return "negative_elapsed";
    if(*count>capacity) return "count_exceeds_capacity";
    if(*count) {
        TimerPoint last=points[*count-1];
        if(distance<last.distance-TIMER_BACKWARD_DISTANCE_JUMP_METERS) return "distance_regression_over_5m";
        if(elapsed<last.elapsed-TIMER_BACKWARD_TIME_JUMP_SECONDS) return "elapsed_regression_over_0_1s";
        if(distance<=last.distance+0.001f) return 0;
    }
    if(*count>=capacity) return "capacity_reached";
    return 0;
}
/* Timer profiles are sampled in race-time order. Repeated or nearly repeated
 * distances are ignored so stopped frames cannot create a vertical segment. */
static int timer_profile_append_detailed(TimerPoint *points,unsigned *count,unsigned capacity,float distance,float elapsed,const char **rejection_reason) {
    const char *reason=timer_profile_rejection_reason(points,count,capacity,distance,elapsed);
    if(rejection_reason) *rejection_reason=reason;
    if(reason) return 0;
    if(*count) {
        TimerPoint last=points[*count-1];
        if(distance<=last.distance+0.001f) {
            if(distance>=last.distance-TIMER_BACKWARD_DISTANCE_JUMP_METERS && elapsed>last.elapsed) points[*count-1].elapsed=elapsed;
            return 1;
        }
        if(elapsed<last.elapsed) elapsed=last.elapsed;
    }
    points[*count].distance=distance; points[*count].elapsed=elapsed; (*count)++;
    return 1;
}
static int timer_profile_append(TimerPoint *points,unsigned *count,unsigned capacity,float distance,float elapsed) {
    return timer_profile_append_detailed(points,count,capacity,distance,elapsed,0);
}
static int timer_profile_time_at(const TimerPoint *points,unsigned count,float distance,float *elapsed) {
    if(!points || count<2 || !elapsed || !isfinite(distance)) return 0;
    for(unsigned i=0;i<count;i++) {
        if(!isfinite(points[i].distance) || !isfinite(points[i].elapsed) || (i && (points[i].distance<=points[i-1].distance || points[i].elapsed<points[i-1].elapsed))) return 0;
    }
    if(distance<points[0].distance || distance>points[count-1].distance) return 0;
    if(distance==points[0].distance) { *elapsed=points[0].elapsed; return 1; }
    for(unsigned i=1;i<count;i++) {
        if(distance==points[i].distance) { *elapsed=points[i].elapsed; return 1; }
        if(points[i-1].distance<distance && distance<points[i].distance) {
            float fraction=(distance-points[i-1].distance)/(points[i].distance-points[i-1].distance);
            *elapsed=points[i-1].elapsed+(points[i].elapsed-points[i-1].elapsed)*fraction;
            return isfinite(*elapsed);
        }
    }
    return 0;
}
static int timer_crossing_time(float before_distance,float before_time,float after_distance,float after_time,float line,float *crossing_time) {
    if(!crossing_time || !isfinite(before_distance) || !isfinite(before_time) || !isfinite(after_distance) || !isfinite(after_time) || !isfinite(line)) return 0;
    if(after_distance<=before_distance || after_time<before_time || line<before_distance || line>after_distance) return 0;
    float fraction=(line-before_distance)/(after_distance-before_distance);
    *crossing_time=before_time+(after_time-before_time)*fraction;
    return isfinite(*crossing_time);
}
static const char *load_guard(const Sample *s,const Mark *m,const Gates *g) {
    if (!g->versions) return "unsupported_binary_hash";
    if (!g->log_ok) return "logging_unavailable";
    if (!finite_sample(s)) return "nonfinite_sample";
    if (!s->foreground) return "game_not_foreground";
    if (!s->active || s->mode!=1) return "not_driving";
    if (!m->valid) return "no_saved_mark";
    if (!same_context(s,&m->sample)) return "different_run_or_objects";
    if (s->state!=0 && s->state!=4) return "stock_jump_state_rejected";
    if (s->distance-m->sample.distance<5.0f) return "only_backward_jump_at_least_5m";
    if (!g->enabled) return "dry_run";
    if (!g->thread_ok) return "unexpected_render_thread";
    if (!g->runtime_ok) return "runtime_code_changed";
    return 0;
}
#endif
