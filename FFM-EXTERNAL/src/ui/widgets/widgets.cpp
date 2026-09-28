#define IMGUI_DEFINE_MATH_OPERATORS
#include "widgets.hpp"
#include "Android_draw/draw.h"
#include <functional>

namespace ui {

namespace widgets {
    using namespace style;

    void separator(float a) {
        if (a < 0.01f) return;

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImVec2 pos = w->DC.CursorPos;
        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;

        ImDrawList* dl = w->DrawList;
        float y = pos.y + 4.f * S;

        dl->AddRectFilledMultiColor(
            ImVec2(pos.x, y), ImVec2(pos.x + ww * 0.4f, y + 1),
            col(UI::border, a * 0.6f), col(UI::border, 0.f),
            col(UI::border, 0.f), col(UI::border, a * 0.6f)
        );
        dl->AddRectFilledMultiColor(
            ImVec2(pos.x + ww * 0.6f, y), ImVec2(pos.x + ww, y + 1),
            col(UI::border, 0.f), col(UI::border, a * 0.6f),
            col(UI::border, a * 0.6f), col(UI::border, 0.f)
        );

        ImGui::Dummy(ImVec2(0, 8.f * S));
    }

    bool button(const char* name, float a, std::function<void()> callback) {
        if (a < 0.01f) return false;

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("btn_") + name;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h = 30.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max) && !popup();
        bool press = hov && ImGui::IsMouseClicked(0);

        if (press) {
            if (callback) callback();
        }

        ImDrawList* dl = w->DrawList;
        ImVec4 bg = hov ? UI::widget : UI::panel;
        dl->AddRectFilled(r.Min, r.Max, col(bg, a));
        dl->AddRect(r.Min, r.Max, col(UI::border, a));

        ImGui::PushFont(fontMedium);
        ImVec2 ts = ImGui::CalcTextSize(name);
        dl->AddText(ImVec2(r.GetCenter().x - ts.x * 0.5f, r.GetCenter().y - ts.y * 0.5f), 
                    col(UI::text, a), name);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, h + 4.f * S));
        
        return press;
    }

    bool toggle(const char* label, bool* v, float a) {
        if (a < 0.01f || !v) return false;

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        if (w->SkipItems) return false;

        std::string ids = std::string("tog_") + label;
        ImGuiID id = w->GetID(ids.c_str());

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h  = 22.f * S;

        ImVec2 pos = w->DC.CursorPos;
        ImRect r(pos, ImVec2(pos.x + ww, pos.y + h));

        ImGui::ItemSize(r);
        if (!ImGui::ItemAdd(r, id)) return false;

        bool hov = ImGui::IsMouseHoveringRect(r.Min, r.Max) && !popup();
        bool changed = false;
        if (hov && ImGui::IsMouseClicked(0)) {
            *v = !(*v);
            changed = true;
        }

        ImDrawList* dl = w->DrawList;

        // Animate knob translation 0.0 -> 1.0
        float t = anim(ids + "_anim", *v ? 1.f : 0.f, 15.f);

        // Switch track dimensions
        float sw = 36.f * S;
        float sh = 16.f * S;
        float sx = pos.x + ww - sw - 8.f * S;
        float sy = pos.y + (h - sh) * 0.5f;

        // Label on left
        ImGui::PushFont(fontMedium);
        ImVec4 text_col = *v ? UI::text : (hov ? UI::text_light : UI::text_dim);
        dl->AddText(ImVec2(pos.x + 8.f * S, pos.y + (h - fontMedium->FontSize) * 0.5f), col(text_col, a), label);
        ImGui::PopFont();

        // Track colors: dark obsidian when off, vibrant blood crimson when on
        ImU32 off_track = IM_COL32(20, 20, 25, (int)(220 * a));
        ImU32 on_track  = col(UI::accent, a);
        ImU32 track_col = ImLerp(off_track, on_track, t);

        ImU32 off_border = col(UI::border, 0.4f * a);
        ImU32 on_border  = col(UI::accent_light, a);
        ImU32 border_col = ImLerp(off_border, on_border, t);

        dl->AddRectFilled(ImVec2(sx, sy), ImVec2(sx + sw, sy + sh), track_col, sh * 0.5f);
        dl->AddRect(ImVec2(sx, sy), ImVec2(sx + sw, sy + sh), border_col, sh * 0.5f);

        // Circular knob with subtle drop shadow
        float kr = (sh - 4.f * S) * 0.5f;
        float kx = sx + 2.f * S + kr + (sw - sh) * t;
        float ky = sy + sh * 0.5f;

        dl->AddCircleFilled(ImVec2(kx + 1.f, ky + 1.f), kr, IM_COL32(0, 0, 0, (int)(90 * a)));
        dl->AddCircleFilled(ImVec2(kx, ky), kr, IM_COL32(255, 255, 255, (int)(250 * a)));

        ImGui::Dummy(ImVec2(0, 2.f * S));
        return changed;
    }

    static std::vector<ImVec2> s_card_stack;

    void begin_card(const char* title, float a) {
        ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImVec2 pos = w->DC.CursorPos;
        s_card_stack.push_back(pos);

        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float h  = 20.f * S;

        ImDrawList* dl = w->DrawList;

        // Card header badge
        dl->AddRectFilled(pos, ImVec2(pos.x + 3.f * S, pos.y + h), col(UI::accent, a), 2.f);

        ImGui::PushFont(fontBold);
        dl->AddText(ImVec2(pos.x + 10.f * S, pos.y + (h - fontBold->FontSize) * 0.5f), col(UI::text, a), title);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, h + 4.f * S));
    }

    void end_card(float a) {
        if (s_card_stack.empty()) return;
        ImVec2 start = s_card_stack.back();
        s_card_stack.pop_back();

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImDrawList* dl = w->DrawList;
        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        ImVec2 end_pos = ImVec2(start.x + ww, w->DC.CursorPos.y + 4.f * S);

        // Draw card background behind items using window background channel
        dl->AddRectFilled(start, end_pos, IM_COL32(14, 14, 18, (int)(140 * a)), 8.f);
        dl->AddRect(start, end_pos, col(UI::border, 0.45f * a), 8.f);

        ImGui::Dummy(ImVec2(0, 8.f * S));
    }

    void slider_styled(const char* label, float* val, float min, float max, float a, const char* fmt) {
        if (a < 0.01f || !val) return;

        ImGuiWindow* w = ImGui::GetCurrentWindow();
        ImDrawList* dl = w->DrawList;
        ImVec2 pos = w->DC.CursorPos;
        float ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;

        // Header line: Label on left, formatted value badge on right
        char val_buf[32];
        snprintf(val_buf, sizeof(val_buf), fmt, *val);

        ImGui::PushFont(fontMedium);
        dl->AddText(ImVec2(pos.x + 8.f * S, pos.y), col(UI::text_dim, a), label);

        ImVec2 ts = ImGui::CalcTextSize(val_buf);
        float bw = ts.x + 10.f * S;
        float bh = ts.y + 4.f * S;
        ImVec2 bmin(pos.x + ww - bw - 8.f * S, pos.y);
        ImVec2 bmax(bmin.x + bw, bmin.y + bh);

        dl->AddRectFilled(bmin, bmax, IM_COL32(28, 12, 16, (int)(200 * a)), 4.f);
        dl->AddRect(bmin, bmax, col(UI::accent, 0.4f * a), 4.f);
        dl->AddText(ImVec2(bmin.x + 5.f * S, bmin.y + 2.f * S), col(UI::accent_light, a), val_buf);
        ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, bh + 2.f * S));

        // Slider control
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f * S);
        ImGui::PushItemWidth(ww - 16.f * S);
        ImGui::SliderFloat((std::string("##sld_") + label).c_str(), val, min, max, "");
        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0, 4.f * S));
    }
}
}