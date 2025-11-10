#include <stdio.h>
#include <string.h>

#include "nutmeg_engine.h"
#include "nutmeg_builtin.h"

typedef struct NameFilter {
    const char *name;
} NameFilter;

static bool condition_name_equals(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)engine;
    (void)scene;

    if (!object || !userdata) {
        return false;
    }

    const NameFilter *filter = (const NameFilter *)userdata;
    const char *name = nutmeg_object_name(object);
    return name && strcmp(name, filter->name) == 0;
}

static void action_log_position(NutmegEngine *engine, NutmegScene *scene, NutmegObject *object, void *userdata)
{
    (void)engine;
    (void)scene;
    (void)userdata;

    if (!object) {
        return;
    }

    NutmegVec2 *position = nutmeg_object_position(object);
    printf("%s -> position: (%.2f, %.2f)\n", nutmeg_object_name(object), position->x, position->y);
}

int main(void)
{
    NutmegEngine *engine = nutmeg_engine_create();
    if (!engine) {
        fprintf(stderr, "Failed to create engine\n");
        return 1;
    }

    NutmegScene *scene = nutmeg_engine_add_scene(engine, "demo");
    nutmeg_engine_set_active_scene(engine, "demo");

    NutmegObject *player = nutmeg_scene_spawn_object(scene, "Player");
    NutmegObject *satellite = nutmeg_scene_spawn_object(scene, "Satellite");

    NutmegVec2 *player_velocity = nutmeg_object_velocity(player);
    player_velocity->x = 1.0f;

    NutmegVec2 *satellite_velocity = nutmeg_object_velocity(satellite);
    satellite_velocity->x = 0.0f;
    satellite_velocity->y = 0.5f;

    /* Event: integrate velocity for all objects each tick */
    NutmegEvent integrate = nutmeg_event_make("Integrate", NUTMEG_EVENT_SCOPE_OBJECTS, false);
    nutmeg_event_add_action(&integrate, nutmeg_action_integrate, NULL);
    nutmeg_scene_add_event(scene, integrate);

    /* Event: accelerate the player every second */
    static NameFilter player_filter = {"Player"};
    static NutmegTimer player_timer = {1.0f, 0.0f, true};
    static NutmegVelocityChange speed_boost = {{0.25f, 0.0f}};

    NutmegEvent accelerate = nutmeg_event_make("Boost", NUTMEG_EVENT_SCOPE_OBJECTS, false);
    nutmeg_event_add_condition(&accelerate, condition_name_equals, &player_filter);
    nutmeg_event_add_condition(&accelerate, nutmeg_condition_timer, &player_timer);
    nutmeg_event_add_action(&accelerate, nutmeg_action_add_velocity, &speed_boost);
    nutmeg_event_add_action(&accelerate, nutmeg_action_debug_print, "Speed boost!");
    nutmeg_scene_add_event(scene, accelerate);

    /* Event: log satellite position every half second */
    static NameFilter satellite_filter = {"Satellite"};
    static NutmegTimer satellite_timer = {0.5f, 0.0f, true};

    NutmegEvent log_satellite = nutmeg_event_make("LogSatellite", NUTMEG_EVENT_SCOPE_OBJECTS, false);
    nutmeg_event_add_condition(&log_satellite, condition_name_equals, &satellite_filter);
    nutmeg_event_add_condition(&log_satellite, nutmeg_condition_timer, &satellite_timer);
    nutmeg_event_add_action(&log_satellite, action_log_position, NULL);
    nutmeg_scene_add_event(scene, log_satellite);

    printf("Running Nutmeg orbit demo...\n");
    for (int i = 0; i < 600; ++i) {
        nutmeg_engine_tick(engine, 1.0f / 60.0f);
    }

    NutmegVec2 *player_position = nutmeg_object_position(player);
    NutmegVec2 *satellite_position = nutmeg_object_position(satellite);
    printf("Final player position: (%.2f, %.2f)\n", player_position->x, player_position->y);
    printf("Final satellite position: (%.2f, %.2f)\n", satellite_position->x, satellite_position->y);

    nutmeg_engine_destroy(engine);
    return 0;
}
