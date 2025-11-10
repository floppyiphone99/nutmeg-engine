#include "nutmeg_editor.h"

#include "editor_panels.h"
#include "editor_ui.h"

#include "nutmeg_engine.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl2.h>

#include <cstdio>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

struct NmEditorApp {
    GLFWwindow *window = nullptr;
    NutmegEngine *engine = nullptr;
    NmEditorUIState ui_state;
    bool running = false;
    double last_frame_time = 0.0;
};

static void nm_editor_glfw_error(int error, const char *description)
{
    std::fprintf(stderr, "[GLFW] Error %d: %s\n", error, description ? description : "");
}

NmEditorApp *nm_editor_app_create(void)
{
    glfwSetErrorCallback(nm_editor_glfw_error);

    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialise GLFW for the Nutmeg editor.\n");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Nutmeg Editor", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create GLFW window for the Nutmeg editor.\n");
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    nm_editor_apply_theme();

    NmEditorApp *app = new NmEditorApp();
    app->window = window;
    app->running = true;
    app->last_frame_time = glfwGetTime();
    nm_editor_ui_initialize(app->ui_state);

    return app;
}

void nm_editor_app_destroy(NmEditorApp *app)
{
    if (!app) {
        return;
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (app->window) {
        glfwDestroyWindow(app->window);
    }
    glfwTerminate();

    delete app;
}

void nm_editor_app_set_engine(NmEditorApp *app, NutmegEngine *engine)
{
    if (!app) {
        return;
    }
    app->engine = engine;
}

void nm_editor_app_request_close(NmEditorApp *app)
{
    if (!app) {
        return;
    }
    app->running = false;
}

void nm_editor_app_main_loop(NmEditorApp *app)
{
    if (!app || !app->window) {
        return;
    }

    while (app->running && !glfwWindowShouldClose(app->window)) {
        glfwPollEvents();

        double current_time = glfwGetTime();
        float delta = static_cast<float>(current_time - app->last_frame_time);
        app->last_frame_time = current_time;

        if (app->engine) {
            nutmeg_engine_tick(app->engine, delta);
        }

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        NmEditorMeters meters{};
        if (app->engine) {
            const NutmegEngineMetrics *snapshot = nutmeg_engine_metrics(app->engine);
            if (snapshot) {
                meters.cpu = snapshot->cpu_usage;
                meters.ram = snapshot->ram_usage;
                meters.gpu = snapshot->gpu_usage;
            }
        }

        nm_editor_render_ui(app->ui_state, meters, delta);

        ImGui::Render();
        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(app->window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.58f, 0.58f, 0.58f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(app->window);
    }
}
