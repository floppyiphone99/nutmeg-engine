#include "nutmeg_engine.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

struct NutmegObject {
    unsigned long id;
    bool alive;
    char name[64];
    NutmegScene *scene;
    NutmegVec2 position;
    NutmegVec2 velocity;
    void *userdata;
};

struct NutmegScene {
    char name[64];
    NutmegEngine *engine;
    NutmegObject **objects;
    size_t object_count;
    size_t object_capacity;
    NutmegEvent *events;
    size_t event_count;
    size_t event_capacity;
    unsigned long next_object_id;
};

struct NutmegEngine {
    NutmegScene **scenes;
    size_t scene_count;
    size_t scene_capacity;
    NutmegScene *active_scene;
    float time;
    float last_delta;
    void *userdata;
    NutmegEngineMetrics metrics;
    float gpu_meter_phase;
};

static void *nutmeg_realloc_array(void *ptr, size_t elem_size, size_t *capacity, size_t min_capacity)
{
    if (*capacity >= min_capacity) {
        return ptr;
    }

    size_t new_capacity = (*capacity == 0) ? 4 : *capacity;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }

    void *resized = realloc(ptr, new_capacity * elem_size);
    if (!resized) {
        /* allocation failure is fatal */
        abort();
    }

    *capacity = new_capacity;
    return resized;
}

static NutmegScene *nutmeg_scene_create(NutmegEngine *engine, const char *name)
{
    NutmegScene *scene = (NutmegScene *)calloc(1, sizeof(*scene));
    if (!scene) {
        return NULL;
    }

    scene->engine = engine;
    scene->objects = NULL;
    scene->object_count = 0;
    scene->object_capacity = 0;
    scene->events = NULL;
    scene->event_count = 0;
    scene->event_capacity = 0;
    scene->next_object_id = 1;

    if (name) {
        strncpy(scene->name, name, sizeof(scene->name) - 1);
        scene->name[sizeof(scene->name) - 1] = '\0';
    } else {
        scene->name[0] = '\0';
    }

    return scene;
}

static void nutmeg_event_free(NutmegEvent *event)
{
    if (!event) {
        return;
    }

    free(event->conditions);
    free(event->actions);
    event->conditions = NULL;
    event->actions = NULL;
    event->condition_count = 0;
    event->condition_capacity = 0;
    event->action_count = 0;
    event->action_capacity = 0;
}

static void nutmeg_scene_free(NutmegScene *scene)
{
    if (!scene) {
        return;
    }

    for (size_t i = 0; i < scene->object_count; ++i) {
        free(scene->objects[i]);
    }
    free(scene->objects);
    scene->objects = NULL;

    for (size_t i = 0; i < scene->event_count; ++i) {
        nutmeg_event_free(&scene->events[i]);
    }
    free(scene->events);
    scene->events = NULL;

    free(scene);
}

static size_t nutmeg_scene_estimated_memory(const NutmegScene *scene)
{
    if (!scene) {
        return 0;
    }

    size_t total = sizeof(*scene);
    total += scene->object_capacity * sizeof(NutmegObject *);
    for (size_t i = 0; i < scene->object_count; ++i) {
        total += sizeof(*scene->objects[i]);
    }

    total += scene->event_capacity * sizeof(NutmegEvent);
    for (size_t i = 0; i < scene->event_count; ++i) {
        const NutmegEvent *event = &scene->events[i];
        total += event->condition_capacity * sizeof(NutmegCondition);
        total += event->action_capacity * sizeof(NutmegAction);
    }

    return total;
}

static float nutmeg_clamp_percentage(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 100.0f) {
        return 100.0f;
    }
    return value;
}

static void nutmeg_engine_update_metrics(NutmegEngine *engine, double tick_cpu_time, float delta_seconds)
{
    if (!engine) {
        return;
    }

    float cpu_usage = 0.0f;
    if (delta_seconds > 0.0f && tick_cpu_time >= 0.0) {
        cpu_usage = (float)((tick_cpu_time / (double)delta_seconds) * 100.0);
    }

    size_t total_memory = sizeof(*engine) + engine->scene_capacity * sizeof(NutmegScene *);
    for (size_t i = 0; i < engine->scene_count; ++i) {
        total_memory += nutmeg_scene_estimated_memory(engine->scenes[i]);
    }

    const float budget_mb = 256.0f;
    float ram_usage = (float)total_memory / (1024.0f * 1024.0f);
    ram_usage = nutmeg_clamp_percentage((ram_usage / budget_mb) * 100.0f);

    engine->gpu_meter_phase += delta_seconds * 0.8f;
    float oscillation = (sinf(engine->gpu_meter_phase) + 1.0f) * 15.0f;
    float gpu_usage = nutmeg_clamp_percentage(0.6f * cpu_usage + oscillation);

    engine->metrics.cpu_usage = nutmeg_clamp_percentage(cpu_usage);
    engine->metrics.ram_usage = ram_usage;
    engine->metrics.gpu_usage = nutmeg_clamp_percentage(0.5f * engine->metrics.gpu_usage + 0.5f * gpu_usage);
}

NutmegEngine *nutmeg_engine_create(void)
{
    NutmegEngine *engine = (NutmegEngine *)calloc(1, sizeof(*engine));
    if (!engine) {
        return NULL;
    }

    engine->scenes = NULL;
    engine->scene_count = 0;
    engine->scene_capacity = 0;
    engine->active_scene = NULL;
    engine->time = 0.0f;
    engine->last_delta = 0.0f;
    engine->userdata = NULL;
    engine->metrics.cpu_usage = 0.0f;
    engine->metrics.ram_usage = 0.0f;
    engine->metrics.gpu_usage = 0.0f;
    engine->gpu_meter_phase = 0.0f;
    return engine;
}

void nutmeg_engine_destroy(NutmegEngine *engine)
{
    if (!engine) {
        return;
    }

    for (size_t i = 0; i < engine->scene_count; ++i) {
        nutmeg_scene_free(engine->scenes[i]);
    }

    free(engine->scenes);
    free(engine);
}

void nutmeg_engine_set_userdata(NutmegEngine *engine, void *userdata)
{
    if (!engine) {
        return;
    }
    engine->userdata = userdata;
}

void *nutmeg_engine_userdata(NutmegEngine *engine)
{
    if (!engine) {
        return NULL;
    }
    return engine->userdata;
}

float nutmeg_engine_time(const NutmegEngine *engine)
{
    return engine ? engine->time : 0.0f;
}

float nutmeg_engine_last_delta(const NutmegEngine *engine)
{
    return engine ? engine->last_delta : 0.0f;
}

const NutmegEngineMetrics *nutmeg_engine_metrics(const NutmegEngine *engine)
{
    return engine ? &engine->metrics : NULL;
}

NutmegScene *nutmeg_engine_add_scene(NutmegEngine *engine, const char *name)
{
    if (!engine) {
        return NULL;
    }

    NutmegScene *scene = nutmeg_scene_create(engine, name);
    if (!scene) {
        return NULL;
    }

    engine->scenes = (NutmegScene **)nutmeg_realloc_array(engine->scenes, sizeof(NutmegScene *), &engine->scene_capacity, engine->scene_count + 1);
    engine->scenes[engine->scene_count++] = scene;

    if (!engine->active_scene) {
        engine->active_scene = scene;
    }

    return scene;
}

NutmegScene *nutmeg_engine_find_scene(NutmegEngine *engine, const char *name)
{
    if (!engine || !name) {
        return NULL;
    }

    for (size_t i = 0; i < engine->scene_count; ++i) {
        NutmegScene *scene = engine->scenes[i];
        if (strncmp(scene->name, name, sizeof(scene->name)) == 0) {
            return scene;
        }
    }

    return NULL;
}

bool nutmeg_engine_set_active_scene(NutmegEngine *engine, const char *name)
{
    if (!engine) {
        return false;
    }

    if (name == NULL) {
        engine->active_scene = NULL;
        return true;
    }

    NutmegScene *scene = nutmeg_engine_find_scene(engine, name);
    if (!scene) {
        return false;
    }

    engine->active_scene = scene;
    return true;
}

NutmegScene *nutmeg_engine_get_active_scene(NutmegEngine *engine)
{
    if (!engine) {
        return NULL;
    }
    return engine->active_scene;
}

NutmegObject *nutmeg_scene_spawn_object(NutmegScene *scene, const char *name)
{
    if (!scene) {
        return NULL;
    }

    NutmegObject *object = (NutmegObject *)calloc(1, sizeof(*object));
    if (!object) {
        return NULL;
    }

    object->scene = scene;
    object->alive = true;
    object->id = scene->next_object_id++;
    object->position.x = 0.0f;
    object->position.y = 0.0f;
    object->velocity.x = 0.0f;
    object->velocity.y = 0.0f;
    object->userdata = NULL;

    if (name) {
        strncpy(object->name, name, sizeof(object->name) - 1);
        object->name[sizeof(object->name) - 1] = '\0';
    } else {
        object->name[0] = '\0';
    }

    scene->objects = (NutmegObject **)nutmeg_realloc_array(scene->objects, sizeof(NutmegObject *), &scene->object_capacity, scene->object_count + 1);
    scene->objects[scene->object_count++] = object;

    return object;
}

static void nutmeg_scene_remove_object(NutmegScene *scene, size_t index)
{
    if (index >= scene->object_count) {
        return;
    }

    free(scene->objects[index]);
    scene->objects[index] = NULL;

    size_t last = scene->object_count - 1;
    if (index != last) {
        scene->objects[index] = scene->objects[last];
    }
    scene->objects[last] = NULL;
    scene->object_count--;
}

void nutmeg_scene_destroy_object(NutmegScene *scene, NutmegObject *object)
{
    if (!scene || !object) {
        return;
    }

    for (size_t i = 0; i < scene->object_count; ++i) {
        if (scene->objects[i] == object) {
            nutmeg_scene_remove_object(scene, i);
            break;
        }
    }
}

unsigned long nutmeg_object_id(const NutmegObject *object)
{
    return object ? object->id : 0UL;
}

const char *nutmeg_object_name(const NutmegObject *object)
{
    return object ? object->name : NULL;
}

NutmegVec2 *nutmeg_object_position(NutmegObject *object)
{
    return object ? &object->position : NULL;
}

NutmegVec2 *nutmeg_object_velocity(NutmegObject *object)
{
    return object ? &object->velocity : NULL;
}

void nutmeg_object_set_userdata(NutmegObject *object, void *userdata)
{
    if (!object) {
        return;
    }
    object->userdata = userdata;
}

void *nutmeg_object_userdata(NutmegObject *object)
{
    return object ? object->userdata : NULL;
}

NutmegScene *nutmeg_object_scene(NutmegObject *object)
{
    return object ? object->scene : NULL;
}

void nutmeg_scene_add_event(NutmegScene *scene, NutmegEvent event)
{
    if (!scene) {
        return;
    }

    scene->events = (NutmegEvent *)nutmeg_realloc_array(scene->events, sizeof(NutmegEvent), &scene->event_capacity, scene->event_count + 1);
    scene->events[scene->event_count++] = event;
}

void nutmeg_event_reset(NutmegEvent *event)
{
    if (!event) {
        return;
    }

    event->triggered = false;
    for (size_t i = 0; i < event->condition_count; ++i) {
        /* no per-condition state to reset currently */
        (void)i;
    }
}

NutmegEvent nutmeg_event_make(const char *name, NutmegEventScope scope, bool once)
{
    NutmegEvent event;
    event.name = name;
    event.scope = scope;
    event.once = once;
    event.triggered = false;
    event.conditions = NULL;
    event.condition_count = 0;
    event.condition_capacity = 0;
    event.actions = NULL;
    event.action_count = 0;
    event.action_capacity = 0;
    return event;
}

void nutmeg_event_add_condition(NutmegEvent *event, NutmegConditionFn fn, void *userdata)
{
    if (!event || !fn) {
        return;
    }

    event->conditions = (NutmegCondition *)nutmeg_realloc_array(event->conditions, sizeof(NutmegCondition), &event->condition_capacity, event->condition_count + 1);
    event->conditions[event->condition_count].fn = fn;
    event->conditions[event->condition_count].userdata = userdata;
    event->condition_count += 1;
}

void nutmeg_event_add_action(NutmegEvent *event, NutmegActionFn fn, void *userdata)
{
    if (!event || !fn) {
        return;
    }

    event->actions = (NutmegAction *)nutmeg_realloc_array(event->actions, sizeof(NutmegAction), &event->action_capacity, event->action_count + 1);
    event->actions[event->action_count].fn = fn;
    event->actions[event->action_count].userdata = userdata;
    event->action_count += 1;
}

static bool nutmeg_conditions_pass(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, const NutmegEvent *event)
{
    for (size_t i = 0; i < event->condition_count; ++i) {
        NutmegCondition condition = event->conditions[i];
        if (!condition.fn(engine, scene, object, condition.userdata)) {
            return false;
        }
    }
    return true;
}

static void nutmeg_execute_actions(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, const NutmegEvent *event)
{
    for (size_t i = 0; i < event->action_count; ++i) {
        NutmegAction action = event->actions[i];
        action.fn(engine, scene, object, action.userdata);
    }
}

static void nutmeg_tick_scene(NutmegScene *scene, float delta)
{
    (void)delta;

    for (size_t e = 0; e < scene->event_count; ++e) {
        NutmegEvent *event = &scene->events[e];
        if (event->once && event->triggered) {
            continue;
        }

        bool triggered_this_tick = false;

        switch (event->scope) {
            case NUTMEG_EVENT_SCOPE_GLOBAL:
            case NUTMEG_EVENT_SCOPE_SCENE:
                if (nutmeg_conditions_pass(scene->engine, scene, NULL, event)) {
                    nutmeg_execute_actions(scene->engine, scene, NULL, event);
                    triggered_this_tick = true;
                }
                break;
            case NUTMEG_EVENT_SCOPE_OBJECTS:
                for (size_t i = 0; i < scene->object_count; ++i) {
                    NutmegObject *object = scene->objects[i];
                    if (!object || !object->alive) {
                        continue;
                    }

                    if (nutmeg_conditions_pass(scene->engine, scene, object, event)) {
                        nutmeg_execute_actions(scene->engine, scene, object, event);
                        triggered_this_tick = true;
                    }
                }
                break;
        }

        if (event->once && triggered_this_tick) {
            event->triggered = true;
        }
    }
}

void nutmeg_engine_tick(NutmegEngine *engine, float delta_seconds)
{
    if (!engine || delta_seconds < 0.0f) {
        return;
    }

    clock_t tick_start = clock();

    engine->last_delta = delta_seconds;
    engine->time += delta_seconds;

    NutmegScene *scene = engine->active_scene;
    if (scene) {
        nutmeg_tick_scene(scene, delta_seconds);
    }

    clock_t tick_end = clock();
    double cpu_time = 0.0;
    if (tick_end != (clock_t)-1 && tick_start != (clock_t)-1 && tick_end >= tick_start) {
        cpu_time = (double)(tick_end - tick_start) / (double)CLOCKS_PER_SEC;
    }

    nutmeg_engine_update_metrics(engine, cpu_time, delta_seconds);
    if (!scene) {
        return;
    }

    nutmeg_tick_scene(scene, delta_seconds);
}

NutmegObject **nutmeg_scene_objects(NutmegScene *scene, size_t *out_count)
{
    if (!scene) {
        if (out_count) {
            *out_count = 0;
        }
        return NULL;
    }

    if (out_count) {
        *out_count = scene->object_count;
    }
    return scene->objects;
}
