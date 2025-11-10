#pragma once

#include "nutmeg_engine.h"

#include <cstddef>

struct NmEditorMeters {
    float cpu = 0.0f;
    float ram = 0.0f;
    float gpu = 0.0f;
};

struct NmEditorUIState {
    char object_name[64];
    char text_content[128];
    char font_name[64];
    char created_label[64];
    float meter_smoothed[3];
    float meter_history[3][120];
    std::size_t meter_cursor;
    bool show_properties;
    float last_delta;

    NmEditorUIState();
};

void nm_editor_ui_initialize(NmEditorUIState &state);
void nm_editor_apply_theme();
void nm_editor_render_ui(NmEditorUIState &state, const NmEditorMeters &meters, float delta_seconds);
