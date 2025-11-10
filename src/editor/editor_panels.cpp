#include "editor_panels.h"

#include <imgui_internal.h>

#include <cstdio>

static ImGuiWindowFlags nm_panel_flags()
{
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
           ImGuiWindowFlags_NoCollapse;
}

float nm_editor_draw_menu_bar()
{
    float menu_height = 0.0f;
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New...");
            ImGui::MenuItem("Load...");
            ImGui::MenuItem("Save...");
            ImGui::MenuItem("Save as...");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Toggle inspector", nullptr, true);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::MenuItem("Version History...");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Project")) {
            ImGui::MenuItem("Project settings");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Parts")) {
            ImGui::MenuItem("Scene bank");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Special")) {
            ImGui::MenuItem("Export wizard...");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::MenuItem("Open manual");
            ImGui::MenuItem("About Nutmeg Editor");
            ImGui::EndMenu();
        }

        menu_height = ImGui::GetWindowSize().y;
        ImGui::EndMainMenuBar();
    }

    return menu_height;
}

static void nm_draw_meter_bar(const char *label, float value, const NmEditorLayout &layout, int history_index, const NmEditorUIState &state)
{
    ImGui::PushID(label);

    ImGui::TextUnformatted(label);
    ImVec2 label_size = ImGui::CalcTextSize(label);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 available(layout.size.x - 32.0f, (layout.size.y - 70.0f) / 3.0f);
    available.x = available.x < 40.0f ? 40.0f : available.x;
    available.y = available.y < 80.0f ? 80.0f : available.y;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 bar_min = cursor;
    ImVec2 bar_max = ImVec2(cursor.x + available.x, cursor.y + available.y);

    const ImU32 bg_col = IM_COL32(210, 210, 210, 255);
    const ImU32 border_col = IM_COL32(80, 80, 80, 255);
    const ImU32 fill_col = IM_COL32(140, 140, 255, 255);

    draw_list->AddRectFilled(bar_min, bar_max, bg_col, 2.0f);
    draw_list->AddRect(bar_min, bar_max, border_col, 2.0f);

    float clamped = value;
    if (clamped < 0.0f) {
        clamped = 0.0f;
    }
    if (clamped > 100.0f) {
        clamped = 100.0f;
    }

    float fill_height = available.y * (clamped / 100.0f);
    ImVec2 fill_min = ImVec2(bar_min.x + 4.0f, bar_max.y - fill_height);
    ImVec2 fill_max = ImVec2(bar_max.x - 4.0f, bar_max.y - 4.0f);
    draw_list->AddRectFilled(fill_min, fill_max, fill_col, 2.0f);

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%3.0f%%", clamped);
    ImVec2 text_size = ImGui::CalcTextSize(buffer);
    ImVec2 text_pos = ImVec2(bar_min.x + (available.x - text_size.x) * 0.5f, bar_min.y + 6.0f);
    draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), buffer);

    // sparkline history along the bottom
    const int sample_count = 120;
    float last_value = state.meter_history[history_index][(state.meter_cursor + sample_count - 1) % sample_count];
    ImVec2 spark_origin = ImVec2(bar_min.x + 6.0f, bar_max.y - 18.0f);
    float spark_width = available.x - 12.0f;
    float spark_height = 12.0f;
    draw_list->AddRect(spark_origin, ImVec2(spark_origin.x + spark_width, spark_origin.y + spark_height), border_col, 2.0f);
    for (int i = 0; i < sample_count; ++i) {
        int index = (state.meter_cursor + i) % sample_count;
        float sample = state.meter_history[history_index][index];
        float next = state.meter_history[history_index][(index + 1) % sample_count];
        float x0 = spark_origin.x + spark_width * (i / (float)sample_count);
        float x1 = spark_origin.x + spark_width * ((i + 1) / (float)sample_count);
        float y0 = spark_origin.y + spark_height - (spark_height * (sample / 100.0f));
        float y1 = spark_origin.y + spark_height - (spark_height * (next / 100.0f));
        draw_list->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(60, 60, 60, 255), 1.5f);
    }

    ImGui::Dummy(ImVec2(available.x, available.y + 20.0f));
    ImGui::PopID();
    (void)label_size;
    (void)last_value;
}

void nm_editor_draw_sidebar(NmEditorUIState &state, const NmEditorMeters &meters, const NmEditorLayout &layout)
{
    (void)meters;
    ImGui::SetNextWindowPos(layout.position);
    ImGui::SetNextWindowSize(layout.size);
    ImGui::Begin("PerformanceMeters", nullptr, nm_panel_flags());

    nm_draw_meter_bar("CPU", state.meter_smoothed[0], layout, 0, state);
    nm_draw_meter_bar("RAM", state.meter_smoothed[1], layout, 1, state);
    nm_draw_meter_bar("GPU", state.meter_smoothed[2], layout, 2, state);

    ImGui::End();
}

void nm_editor_draw_canvas(NmEditorUIState &state, const NmEditorLayout &layout)
{
    ImGui::SetNextWindowPos(layout.position);
    ImGui::SetNextWindowSize(layout.size);
    ImGuiWindowFlags flags = nm_panel_flags() | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("GameCanvas", nullptr, flags);

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(255, 255, 255, 255));
    draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(60, 60, 60, 255));

    ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + 24.0f, canvas_pos.y + 24.0f));
    ImGui::TextUnformatted("Game Preview");

    ImGui::SetCursorScreenPos(ImVec2(canvas_pos.x + 40.0f, canvas_pos.y + 80.0f));
    float previous_scale = ImGui::GetFont()->Scale;
    ImGui::SetWindowFontScale(2.7f);
    ImGui::TextUnformatted(state.text_content);
    ImGui::SetWindowFontScale(previous_scale);

    ImGui::End();
}

void nm_editor_draw_inspector(NmEditorUIState &state, const NmEditorLayout &layout)
{
    ImGui::SetNextWindowPos(layout.position);
    ImGui::SetNextWindowSize(layout.size);
    ImGui::Begin("ObjectsInspector", nullptr, nm_panel_flags());

    ImGui::TextUnformatted("Objects");
    ImGui::Separator();

    ImGui::TextUnformatted("NewText1 input");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##object_name", state.object_name, IM_ARRAYSIZE(state.object_name));

    ImGui::Spacing();
    if (ImGui::Button("Properties", ImVec2(-1.0f, 0.0f))) {
        state.show_properties = !state.show_properties;
    }
    ImGui::Button("Delete object...", ImVec2(-1.0f, 0.0f));
    ImGui::Button("Open FloppedGameCoding... (COMING SOON)", ImVec2(-1.0f, 0.0f));

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::TextUnformatted("359x305 - \"Game\"");
    ImGui::Text("Font: %s", state.font_name);
    ImGui::TextUnformatted("Size: 69");
    ImGui::Text("Text: %s", state.text_content);
    ImGui::Text("Created: %s", state.created_label);

    ImGui::End();
}

void nm_editor_draw_footer(const NmEditorUIState &state, const NmEditorLayout &layout)
{
    (void)state;
    ImGui::SetNextWindowPos(layout.position);
    ImGui::SetNextWindowSize(layout.size);
    ImGuiWindowFlags flags = nm_panel_flags() | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("FooterBar", nullptr, flags);

    if (ImGui::Button("New object...", ImVec2(160.0f, 0.0f))) {
        /* placeholder interaction */
    }
    ImGui::SameLine();
    ImGui::Button("Event editor...", ImVec2(160.0f, 0.0f));
    ImGui::SameLine();
    ImGui::Button("Open FloppedGameScripts... (C#, Ansi-C, Flang)", ImVec2(320.0f, 0.0f));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextUnformatted("Copr 2025. Floppy");

    ImGui::End();
}
