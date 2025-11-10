#ifndef NUTMEG_ENGINE_H
#define NUTMEG_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file nutmeg_engine.h
 * \brief Public API for the Nutmeg game engine runtime.
 *
 * The Nutmeg engine provides an event/condition/action pipeline that mirrors
 * visual scripting workflows found in tools like GDevelop or Clickteam
 * Fusion. Games are described by creating scenes, spawning objects, and then
 * wiring events that react to conditions and execute actions without requiring
 * handwritten update loops for each entity.
 */

struct NutmegEngine;
struct NutmegScene;
struct NutmegObject;

/** Simple 2D vector used by built-in helpers. */
typedef struct NutmegVec2 {
    float x;
    float y;
} NutmegVec2;

/** Engine level pointer type aliases to make the API more readable. */
typedef struct NutmegEngine NutmegEngine;
typedef struct NutmegScene NutmegScene;
typedef struct NutmegObject NutmegObject;

/**
 * A condition callback returns true when its logic has been satisfied. The
 * callback receives the engine, the owning scene, an optional object (NULL for
 * global events) and a user provided payload pointer.
 */
typedef bool (*NutmegConditionFn)(NutmegEngine *, NutmegScene *, NutmegObject *, void *userdata);

/**
 * An action callback performs mutations when executed. The signature matches
 * condition callbacks but returns void.
 */
typedef void (*NutmegActionFn)(NutmegEngine *, NutmegScene *, NutmegObject *, void *userdata);

/** Wrapper storing a condition callback and its payload. */
typedef struct NutmegCondition {
    NutmegConditionFn fn;
    void *userdata;
} NutmegCondition;

/** Wrapper storing an action callback and its payload. */
typedef struct NutmegAction {
    NutmegActionFn fn;
    void *userdata;
} NutmegAction;

/** Determines which targets an event operates on. */
typedef enum NutmegEventScope {
    /** Execute once per tick regardless of objects. */
    NUTMEG_EVENT_SCOPE_GLOBAL,
    /** Execute for the scene once per tick (object pointer is NULL). */
    NUTMEG_EVENT_SCOPE_SCENE,
    /** Execute individually for each live object in the scene. */
    NUTMEG_EVENT_SCOPE_OBJECTS
} NutmegEventScope;

/** Defines a GDevelop/Clickteam style event. */
typedef struct NutmegEvent {
    const char *name;          /**< Optional debug name. */
    NutmegEventScope scope;    /**< Dispatch target. */
    bool once;                 /**< If true, run at most a single time. */
    bool triggered;            /**< Internal flag to honour once semantics. */
    NutmegCondition *conditions; /**< Dynamic array of conditions. */
    size_t condition_count;
    size_t condition_capacity;
    NutmegAction *actions;     /**< Dynamic array of actions. */
    size_t action_count;
    size_t action_capacity;
} NutmegEvent;

/**
 * Allocate a new engine instance. Use nutmeg_engine_destroy once finished.
 */
NutmegEngine *nutmeg_engine_create(void);

/** Destroy an engine created with nutmeg_engine_create. */
void nutmeg_engine_destroy(NutmegEngine *engine);

/** Attach user owned context data to the engine. */
void nutmeg_engine_set_userdata(NutmegEngine *engine, void *userdata);

/** Retrieve the previously stored user context pointer. */
void *nutmeg_engine_userdata(NutmegEngine *engine);

/** Return the accumulated runtime of the engine in seconds. */
float nutmeg_engine_time(const NutmegEngine *engine);

/** Retrieve the most recent delta time passed to nutmeg_engine_tick. */
float nutmeg_engine_last_delta(const NutmegEngine *engine);

/** Create and register a new scene with the engine. */
NutmegScene *nutmeg_engine_add_scene(NutmegEngine *engine, const char *name);

/** Lookup a scene by name. Returns NULL when missing. */
NutmegScene *nutmeg_engine_find_scene(NutmegEngine *engine, const char *name);

/** Activate the named scene. */
bool nutmeg_engine_set_active_scene(NutmegEngine *engine, const char *name);

/** Retrieve the currently active scene (may be NULL). */
NutmegScene *nutmeg_engine_get_active_scene(NutmegEngine *engine);

/** Spawn a new object inside a scene. */
NutmegObject *nutmeg_scene_spawn_object(NutmegScene *scene, const char *name);

/** Destroy an object created inside the given scene. */
void nutmeg_scene_destroy_object(NutmegScene *scene, NutmegObject *object);

/** Retrieve a stable identifier for an object. */
unsigned long nutmeg_object_id(const NutmegObject *object);

/** Query the name string associated with an object. */
const char *nutmeg_object_name(const NutmegObject *object);

/**
 * Access mutable vector data for an object.
 */
NutmegVec2 *nutmeg_object_position(NutmegObject *object);
NutmegVec2 *nutmeg_object_velocity(NutmegObject *object);

/** User storage per object. */
void nutmeg_object_set_userdata(NutmegObject *object, void *userdata);
void *nutmeg_object_userdata(NutmegObject *object);

/** Retrieve a pointer to the owning scene. */
NutmegScene *nutmeg_object_scene(NutmegObject *object);

/**
 * Append an event to the scene. Ownership of the event structure and its
 * dynamically allocated condition/action arrays transfers to the scene. The
 * caller should treat the provided event as moved and avoid mutating it
 * afterwards.
 */
void nutmeg_scene_add_event(NutmegScene *scene, NutmegEvent event);

/** Reset the runtime state of an event (clears once/triggered flags). */
void nutmeg_event_reset(NutmegEvent *event);

/** Utility constructors for events. */
NutmegEvent nutmeg_event_make(const char *name, NutmegEventScope scope, bool once);
void nutmeg_event_add_condition(NutmegEvent *event, NutmegConditionFn fn, void *userdata);
void nutmeg_event_add_action(NutmegEvent *event, NutmegActionFn fn, void *userdata);

/** Tick the engine forward by delta seconds. */
void nutmeg_engine_tick(NutmegEngine *engine, float delta_seconds);

/**
 * Enumerate the objects in a scene. Returns an array pointer owned by the
 * scene. The caller should not modify or free it. The array is valid until the
 * next mutation (spawn/destroy) on the scene.
 */
NutmegObject **nutmeg_scene_objects(NutmegScene *scene, size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* NUTMEG_ENGINE_H */
