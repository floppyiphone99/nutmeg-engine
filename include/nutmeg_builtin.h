#ifndef NUTMEG_BUILTIN_H
#define NUTMEG_BUILTIN_H

#include "nutmeg_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Helper payload for timer based conditions. Keeps state between ticks for a
 * single event instance.
 */
typedef struct NutmegTimer {
    float interval;
    float accumulator;
    bool repeat;
} NutmegTimer;

/** Initialise a timer helper. */
static inline NutmegTimer nutmeg_timer_make(float interval, bool repeat)
{
    NutmegTimer timer;
    timer.interval = interval;
    timer.accumulator = 0.0f;
    timer.repeat = repeat;
    return timer;
}

/** Condition returning true when the timer interval has elapsed. */
bool nutmeg_condition_timer(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata);

/** Action that integrates velocity onto an object's position. */
void nutmeg_action_integrate(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata);

/** Action that applies a constant acceleration to an object's velocity. */
void nutmeg_action_accelerate(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata);

/** Payload describing a velocity change. */
typedef struct NutmegVelocityChange {
    NutmegVec2 delta;
} NutmegVelocityChange;

/** Action that modifies velocity by the delta stored in the payload. */
void nutmeg_action_add_velocity(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata);

/** Payload describing a translation. */
typedef struct NutmegTranslation {
    NutmegVec2 delta;
} NutmegTranslation;

/** Action that directly moves an object by the delta vector. */
void nutmeg_action_translate(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata);

/** Action that prints a textual message for debugging purposes. */
void nutmeg_action_debug_print(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* NUTMEG_BUILTIN_H */
