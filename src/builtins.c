#include "nutmeg_builtin.h"

#include <stdio.h>

bool nutmeg_condition_timer(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)scene;
    (void)object;

    if (!engine || !userdata) {
        return false;
    }

    NutmegTimer *timer = (NutmegTimer *)userdata;
    if (timer->interval <= 0.0f) {
        return true;
    }

    timer->accumulator += nutmeg_engine_last_delta(engine);
    if (timer->accumulator >= timer->interval) {
        if (timer->repeat) {
            timer->accumulator -= timer->interval;
        } else {
            timer->accumulator = timer->interval;
        }
        return true;
    }

    return false;
}

void nutmeg_action_integrate(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)scene;
    (void)userdata;

    if (!engine || !object) {
        return;
    }

    float dt = nutmeg_engine_last_delta(engine);
    NutmegVec2 *position = nutmeg_object_position(object);
    NutmegVec2 *velocity = nutmeg_object_velocity(object);
    position->x += velocity->x * dt;
    position->y += velocity->y * dt;
}

void nutmeg_action_accelerate(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)scene;

    if (!engine || !object || !userdata) {
        return;
    }

    float dt = nutmeg_engine_last_delta(engine);
    NutmegVec2 *velocity = nutmeg_object_velocity(object);
    NutmegVec2 *acceleration = (NutmegVec2 *)userdata;
    velocity->x += acceleration->x * dt;
    velocity->y += acceleration->y * dt;
}

void nutmeg_action_add_velocity(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)engine;
    (void)scene;

    if (!object || !userdata) {
        return;
    }

    NutmegVelocityChange *change = (NutmegVelocityChange *)userdata;
    NutmegVec2 *velocity = nutmeg_object_velocity(object);
    velocity->x += change->delta.x;
    velocity->y += change->delta.y;
}

void nutmeg_action_translate(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)engine;
    (void)scene;

    if (!object || !userdata) {
        return;
    }

    NutmegTranslation *translation = (NutmegTranslation *)userdata;
    NutmegVec2 *position = nutmeg_object_position(object);
    position->x += translation->delta.x;
    position->y += translation->delta.y;
}

void nutmeg_action_debug_print(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)engine;
    (void)scene;

    const char *message = userdata ? (const char *)userdata : "";
    if (object) {
        printf("[%s] %s\n", nutmeg_object_name(object), message);
    } else {
        printf("%s\n", message);
    }
}
