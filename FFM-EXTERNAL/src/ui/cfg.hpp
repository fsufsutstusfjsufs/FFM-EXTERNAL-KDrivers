#pragma once
#include "imgui.h"

namespace cfg {
namespace esp {
    inline bool  line         = false;
    inline bool  box          = true;
    inline int   box_type     = 3;
    inline bool  info_bar     = true;
    inline bool  skeleton     = false;
    inline bool  health       = true;
    inline bool  distance     = true;
    inline bool  weapon       = true;
    inline bool  bot_tag      = true;
    inline bool  tracer       = false;
    inline bool  crosshair    = true;
    inline bool  fov_circle   = false;
    inline bool  name         = true;

    inline float max_range     = 250.f;
    inline float box_rounding  = 3.f;
    inline float box_thickness = 2.0f;
    inline float bracket_frac  = 0.28f;
    inline float fov_radius    = 250.f;

    inline ImVec4 box_col      = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 box_visible  = ImVec4(0.30f, 1.0f, 0.55f, 1.0f);
    inline ImVec4 accent       = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 name_col     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col = ImVec4(0.95f, 0.95f, 0.30f, 1.0f);
    inline ImVec4 weapon_col   = ImVec4(1.0f, 0.85f, 0.30f, 1.0f);
    inline ImVec4 bot_col      = ImVec4(1.0f, 0.55f, 0.10f, 1.0f);
    inline ImVec4 tracer_col   = ImVec4(0.0f, 0.90f, 1.0f, 0.55f);
    inline ImVec4 crosshair_col= ImVec4(1.0f, 1.0f, 1.0f, 0.9f);
    inline ImVec4 fov_col      = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
}

namespace aim {
    inline bool  visible_enabled  = false;
    inline float visible_fov      = 200.f;
    inline float visible_max      = 250.f;
    inline bool  fov_circle       = false;
    inline float fov_thickness    = 1.5f;
    inline ImVec4 fov_col         = ImVec4(1.f, 1.f, 1.f, 0.6f);
    inline bool  wipe_aim_counter = false;
    inline float head_bias        = 0.0f;
    inline int   target_position  = 0; // 0 = Head, 1 = Neck, 2 = Chest
    inline bool  target_knocked   = false; // Target knocked players in Silent Aim
}

namespace settings {
    inline int fps_choice = 1; // 0 = 60 FPS, 1 = 90 FPS, 2 = 120 FPS, 3 = Max (Uncapped)

    inline int get_target_fps() {
        switch (fps_choice) {
            case 0: return 60;
            case 1: return 90;
            case 2: return 120;
            default: return 0; // Uncapped
        }
    }
}
}
