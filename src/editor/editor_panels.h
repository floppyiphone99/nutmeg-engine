#pragma once

#include "editor_ui.h"

#include <imgui.h>

struct NmEditorLayout {
    ImVec2 position;
    ImVec2 size;
};

float nm_editor_draw_menu_bar();
void nm_editor_draw_sidebar(NmEditorUIState &state, const NmEditorMeters &meters, const NmEditorLayout &layout);
void nm_editor_draw_canvas(NmEditorUIState &state, const NmEditorLayout &layout);
void nm_editor_draw_inspector(NmEditorUIState &state, const NmEditorLayout &layout);
void nm_editor_draw_footer(const NmEditorUIState &state, const NmEditorLayout &layout);
