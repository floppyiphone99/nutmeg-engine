#include "editor_ui.h"

#include "editor_panels.h"

#include <imgui.h>

#include <cstring>

NmEditorUIState::NmEditorUIState()
{
    nm_editor_ui_initialize(*this);
}

void nm_editor_ui_initialize(NmEditorUIState &state)
{
    std::strncpy(state.object_name, "NewText1", sizeof(state.object_name));
    state.object_name[sizeof(state.object_name) - 1] = '\0';
    std::strncpy(state.text_content, "Game", sizeof(state.text_content));
    state.text_content[sizeof(state.text_content) - 1] = '\0';
    std::strncpy(state.font_name, "Chicago FLF", sizeof(state.font_name));
    state.font_name[sizeof(state.font_name) - 1] = '\0';
    std::strncpy(state.created_label, "Now", sizeof(state.created_label));
    state.created_label[sizeof(state.created_label) - 1] = '\0';

    for (int i = 0; i < 3; ++i) {
        state.meter_smoothed[i] = 0.0f;
        for (int j = 0; j < 120; ++j) {
            state.meter_history[i][j] = 0.0f;
        }
    }
    state.meter_cursor = 0;
    state.show_properties = false;
    state.last_delta = 0.0f;
}

void nm_editor_apply_theme()
{
    ImGui::StyleColorsClassic();

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.GrabRounding = 2.0f;
    style.WindowPadding = ImVec2(8.0f, 6.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.83f, 0.83f, 0.83f, 1.0f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.73f, 0.73f, 0.73f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.78f, 0.78f, 0.78f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.43f, 0.43f, 0.43f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.43f, 0.43f, 0.43f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.43f, 0.43f, 0.43f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
}

static void nm_editor_update_metrics(NmEditorUIState &state, const NmEditorMeters &meters)
{
    const float smoothing = 0.18f;
    const float history_smoothing = 0.12f;

    const float values[3] = {meters.cpu, meters.ram, meters.gpu};
    for (int i = 0; i < 3; ++i) {
        state.meter_smoothed[i] = state.meter_smoothed[i] * (1.0f - smoothing) + values[i] * smoothing;
        state.meter_history[i][state.meter_cursor] = state.meter_history[i][state.meter_cursor] * (1.0f - history_smoothing) + values[i] * history_smoothing;
    }

    state.meter_cursor = (state.meter_cursor + 1) % 120;
}

void nm_editor_render_ui(NmEditorUIState &state, const NmEditorMeters &meters, float delta_seconds)
{
    state.last_delta = delta_seconds;
    nm_editor_update_metrics(state, meters);

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 viewport_pos = viewport->Pos;
    const ImVec2 viewport_size = viewport->Size;

    float menu_height = nm_editor_draw_menu_bar();

    const float footer_height = 96.0f;
    const float sidebar_width = 170.0f;
    const float inspector_width = 280.0f;

    NmEditorLayout sidebar_layout{
        ImVec2(viewport_pos.x, viewport_pos.y + menu_height),
        ImVec2(sidebar_width, viewport_size.y - menu_height - footer_height)
    };

    NmEditorLayout inspector_layout{
        ImVec2(viewport_pos.x + viewport_size.x - inspector_width, viewport_pos.y + menu_height),
        ImVec2(inspector_width, viewport_size.y - menu_height - footer_height)
    };

    NmEditorLayout canvas_layout{
        ImVec2(sidebar_layout.position.x + sidebar_layout.size.x, viewport_pos.y + menu_height),
        ImVec2(viewport_size.x - sidebar_layout.size.x - inspector_layout.size.x, viewport_size.y - menu_height - footer_height)
    };

    NmEditorLayout footer_layout{
        ImVec2(viewport_pos.x, viewport_pos.y + viewport_size.y - footer_height),
        ImVec2(viewport_size.x, footer_height)
    };

    nm_editor_draw_sidebar(state, meters, sidebar_layout);
    nm_editor_draw_canvas(state, canvas_layout);
    nm_editor_draw_inspector(state, inspector_layout);
    nm_editor_draw_footer(state, footer_layout);
}
