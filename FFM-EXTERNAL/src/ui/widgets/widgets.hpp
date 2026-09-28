#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "styles/styles.h"
#include <functional>

namespace ui {
namespace widgets {
    void separator(float a);
    bool button(const char* name, float a, std::function<void()> callback = nullptr);
    bool toggle(const char* label, bool* v, float a);
    void begin_card(const char* title, float a);
    void end_card(float a);
    void slider_styled(const char* label, float* val, float min, float max, float a, const char* fmt = "%.1f");
}
}