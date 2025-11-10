# Nutmeg Engine

Nutmeg is a lightweight C game runtime that embraces the condition/action
workflow popularised by click-based engines such as GDevelop and Clickteam
Fusion. Rather than writing bespoke update logic for each entity, you build
scenes composed of objects and wire up events that react to conditions and fire
actions.

The engine is intentionally minimal and focuses on being embeddable inside your
own tools or editors. It exposes a pure C API so you can bind it to scripting
languages or wrap it with a graphical authoring layer.

## Features

- **Event-driven gameplay** – register arbitrary condition and action callbacks
  to drive your game logic without per-object update loops.
- **Scene and object management** – spawn/destroy objects, attach user data, and
  iterate over the scene contents.
- **Reusable helpers** – common conditions and actions such as timers,
  acceleration, integration and debug printing are bundled in
  `nutmeg_builtin.h`.
- **Runtime telemetry** – pull CPU, RAM, and GPU style gauges from the engine
  to feed dashboards or editor overlays.
- **Pure C99 implementation** – the core is a small, dependency-free static
  library that can be embedded into existing pipelines.

## Building

Nutmeg uses CMake for its build configuration. A standard build will create the
static `nutmeg` library alongside the `orbit_demo` example executable.

```bash
cmake -S . -B build
cmake --build build
```

Run the included demo to see the event system in action:

```bash
./build/orbit_demo
```

The example simulates a player ship receiving periodic speed boosts and a
satellite orbiting with constant acceleration. Output is printed directly to the
terminal to highlight how events, timers, and actions compose together.

### Nutmeg ImGui Editor (optional)

An ImGui powered editor that mirrors retro Clickteam-style layouts ships with
the repository. It is disabled by default so the core library can build without
any third-party dependencies. Enable it through CMake when you have the
requirements installed:

```bash
cmake -S . -B build -DNUTMEG_BUILD_EDITOR=ON -DNUTMEG_IMGUI_DIR=/path/to/imgui
cmake --build build --target nutmeg_editor
```

If `NUTMEG_IMGUI_DIR` is omitted the build will fetch Dear ImGui from GitHub
using CMake's `FetchContent`. Providing the variable lets you reuse a local
checkout or vendored copy.

The editor relies on the following native libraries:

- [GLFW](https://www.glfw.org/) for window and input management.
- OpenGL 2.1 (or later) for rendering the ImGui draw lists.

When launched, `nutmeg_editor` displays live engine metrics (CPU/RAM/GPU
gauges), a game viewport, object inspector and command bar laid out to mimic a
classic Clickteam IDE.

## Getting Started

1. Create an engine and at least one scene:
   ```c
   NutmegEngine *engine = nutmeg_engine_create();
   NutmegScene *scene = nutmeg_engine_add_scene(engine, "level_1");
   nutmeg_engine_set_active_scene(engine, "level_1");
   ```
2. Spawn objects and configure their initial state:
   ```c
   NutmegObject *player = nutmeg_scene_spawn_object(scene, "Player");
   nutmeg_object_velocity(player)->x = 32.0f;
   ```
3. Build events that respond to conditions and execute actions:
   ```c
   NutmegEvent movement = nutmeg_event_make("Movement", NUTMEG_EVENT_SCOPE_OBJECTS, false);
   nutmeg_event_add_action(&movement, nutmeg_action_integrate, NULL);
   nutmeg_scene_add_event(scene, movement);
   ```
4. Step the simulation:
   ```c
   while (running) {
       nutmeg_engine_tick(engine, delta_time);
   }
   ```

Extend the runtime by defining your own condition and action callbacks or by
serialising events from an editor/front-end tailored to your workflow.

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for
full details.
