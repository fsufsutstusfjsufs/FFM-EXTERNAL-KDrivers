#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "../game/data.hpp"
#include "../ui/cfg.hpp"
#include "../ui/styles/styles.h"
#include <cstdio>
#include <mutex>

namespace func::esp {

    inline ImU32 hp_gradient(float t) {
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        float r, g, b;
        if (t < 0.5f) {
            float k = t * 2.f;
            r = 1.f;
            g = 0.35f + 0.55f * k;
            b = 0.15f;
        } else {
            float k = (t - 0.5f) * 2.f;
            r = 1.f - 0.9f * k;
            g = 0.9f;
            b = 0.15f + 0.30f * k;
        }
        return IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
    }

    inline void draw_shadowed_text(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* txt) {
        dl->AddText({pos.x + 1.f, pos.y + 1.f}, IM_COL32(0, 0, 0, 200), txt);
        dl->AddText(pos, col, txt);
    }

    inline void draw_pill(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* txt, float pad = 4.f) {
        ImVec2 ts = ImGui::CalcTextSize(txt);
        ImVec2 tl(pos.x - pad, pos.y - pad * 0.5f);
        ImVec2 br(pos.x + ts.x + pad, pos.y + ts.y + pad * 0.5f);
        dl->AddRectFilled(tl, br, IM_COL32(10, 10, 15, 210), 4.f);
        dl->AddRect(tl, br, col, 4.f, 0, 1.2f);
        dl->AddText(pos, IM_COL32_WHITE, txt);
    }

    inline void draw() {
        if (!cfg::esp::box && !cfg::esp::name && !cfg::esp::health && !cfg::esp::distance && !cfg::esp::line) return;

        std::vector<PlayerData> snap;
        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            snap = data::players;
        }
        if (snap.empty()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        ImU32 col_box_default = ImGui::ColorConvertFloat4ToU32(cfg::esp::box_col);
        ImU32 col_box_visible = ImGui::ColorConvertFloat4ToU32(cfg::esp::box_visible);
        ImU32 col_name        = ImGui::ColorConvertFloat4ToU32(cfg::esp::name_col);
        ImU32 col_dist        = ImGui::ColorConvertFloat4ToU32(cfg::esp::distance_col);
        ImU32 col_weapon      = ImGui::ColorConvertFloat4ToU32(cfg::esp::weapon_col);
        ImU32 col_bot         = ImGui::ColorConvertFloat4ToU32(cfg::esp::bot_col);
        ImU32 col_tracer      = ImGui::ColorConvertFloat4ToU32(cfg::esp::tracer_col);

        ImVec2 screen(g_sw, g_sh);
        ImVec2 center(screen.x * 0.5f, screen.y * 0.5f);

        for (const auto& p : snap) {
            if (p.Head.x < 0 || p.Toe.x < 0) continue;
            if (!p.isVisible || p.curHP <= 0) continue;

            float toe_y = p.Toe.y;
            float head_y = p.Head.y;
            if (p.Toe.x < 0) {
                float est_h = (p.distance > 0.1f) ? (500.f / p.distance * 2.f) : 100.f;
                if (est_h < 30.f) est_h = 30.f;
                if (est_h > 300.f) est_h = 300.f;
                toe_y = head_y + est_h;
            }

            float top_y  = (head_y < toe_y ? head_y : toe_y);
            float bot_y  = (head_y > toe_y ? head_y : toe_y);
            float body_h = bot_y - top_y;
            if (body_h < 20.f) {
                body_h = 200.f - 1.5f * p.distance;
                if (body_h < 40.f) body_h = 40.f;
                if (body_h > 250.f) body_h = 250.f;
            }
            float w = body_h * 0.48f;
            float cx = (p.Toe.x >= 0) ? ((p.Head.x + p.Toe.x) * 0.5f) : p.Head.x;
            ImVec2 tl(cx - w * 0.5f, top_y - body_h * 0.05f);
            ImVec2 br(cx + w * 0.5f, bot_y);

            ImU32 col_box = p.isKnocked ? IM_COL32(255, 50, 50, 255)
                          : (p.isVisible ? col_box_visible : col_box_default);

            if (cfg::esp::box) {
                if (cfg::esp::box_type == 0) {
                    float len = (br.x - tl.x) * cfg::esp::bracket_frac;
                    float th  = cfg::esp::box_thickness;
                    ImU32 shadow = IM_COL32(0, 0, 0, 180);
                    ImVec2 pts[8][2] = {
                        {tl,                       {tl.x + len, tl.y}},
                        {tl,                       {tl.x, tl.y + len}},
                        {{br.x - len, tl.y},       {br.x, tl.y}},
                        {{br.x, tl.y},             {br.x, tl.y + len}},
                        {{tl.x, br.y - len},       {tl.x, br.y}},
                        {{tl.x, br.y},             {tl.x + len, br.y}},
                        {{br.x - len, br.y},       {br.x, br.y}},
                        {{br.x, br.y - len},       {br.x, br.y}},
                    };
                    for (auto& s : pts) {
                        dl->AddLine({s[0].x + 1.f, s[0].y + 1.f}, {s[1].x + 1.f, s[1].y + 1.f}, shadow, th + 1.f);
                        dl->AddLine(s[0], s[1], col_box, th);
                    }
                } else if (cfg::esp::box_type == 1) {
                    dl->AddRect(tl, br, col_box, cfg::esp::box_rounding, 0, cfg::esp::box_thickness);
                } else {
                    dl->AddRect({tl.x + 1, tl.y + 1}, {br.x + 1, br.y + 1},
                                IM_COL32(0, 0, 0, 200), cfg::esp::box_rounding, 0, cfg::esp::box_thickness + 0.6f);
                    dl->AddRect(tl, br, col_box, cfg::esp::box_rounding, 0, cfg::esp::box_thickness);
                    dl->AddRectFilled(tl, br, IM_COL32(0, 200, 240, 12), cfg::esp::box_rounding);
                }
            }

            if (cfg::esp::skeleton) {
                ImVec2 hp2(p.Head.x, p.Head.y);
                ImVec2 tp2(p.Toe.x,  p.Toe.y);
                dl->AddLine(hp2, tp2, col_box, 1.4f);
                dl->AddCircleFilled(hp2, 3.f, col_box);
                dl->AddCircleFilled(tp2, 3.f, col_box);
            }

            if (cfg::esp::health) {
                float hp01 = p.maxHP > 0 ? (float)p.curHP / (float)p.maxHP : 0.f;
                if (hp01 < 0.f) hp01 = 0.f;
                if (hp01 > 1.f) hp01 = 1.f;
                float bar_x_tl = tl.x - 8.f;
                float bar_x_br = tl.x - 3.f;
                dl->AddRectFilled({bar_x_tl - 1.f, tl.y - 1.f}, {bar_x_br + 1.f, br.y + 1.f},
                                  IM_COL32(0, 0, 0, 220), 2.f);
                dl->AddRectFilled({bar_x_tl, tl.y}, {bar_x_br, br.y}, IM_COL32(30, 30, 35, 220), 2.f);
                float fill_top = br.y - (br.y - tl.y) * hp01;
                dl->AddRectFilled({bar_x_tl, fill_top}, {bar_x_br, br.y}, hp_gradient(hp01), 2.f);

                char hpbuf[16];
                snprintf(hpbuf, sizeof(hpbuf), "%d", p.curHP);
                ImVec2 ts = ImGui::CalcTextSize(hpbuf);
                draw_shadowed_text(dl, {bar_x_tl - ts.x - 3.f, tl.y - ts.y * 0.15f}, IM_COL32_WHITE, hpbuf);
            }

            float top_stack_y = tl.y - 6.f;
            if (cfg::esp::name && p.name[0]) {
                ImVec2 ts = ImGui::CalcTextSize(p.name);
                float nx = (tl.x + br.x - ts.x) * 0.5f;
                float ny = top_stack_y - ts.y;
                draw_pill(dl, {nx, ny}, col_box, p.name);
                top_stack_y = ny - 3.f;
            }
            if (p.isBot && cfg::esp::bot_tag) {
                const char* tag = "BOT";
                ImVec2 ts = ImGui::CalcTextSize(tag);
                float bx = (tl.x + br.x - ts.x) * 0.5f;
                float by = top_stack_y - ts.y;
                draw_pill(dl, {bx, by}, col_bot, tag, 3.f);
                top_stack_y = by - 3.f;
            }

            float bottom_stack_y = br.y + 4.f;
            if (cfg::esp::distance) {
                char buf[24];
                snprintf(buf, sizeof(buf), "%.0f m", p.distance);
                ImVec2 ts = ImGui::CalcTextSize(buf);
                draw_shadowed_text(dl, {(tl.x + br.x - ts.x) * 0.5f, bottom_stack_y}, col_dist, buf);
                bottom_stack_y += ts.y + 2.f;
            }
            if (cfg::esp::weapon && p.weaponType >= 0) {
                static const char* kNames[] = {
                    "AR","SNIPER","SMG","SHOTGUN","PISTOL","MELEE","GRENADE","LMG","RIFLE","MARKSMAN","SPECIAL"
                };
                const char* wn = (p.weaponType < (int)(sizeof(kNames)/sizeof(*kNames)))
                                 ? kNames[p.weaponType] : "WPN";
                ImVec2 ts = ImGui::CalcTextSize(wn);
                draw_shadowed_text(dl, {(tl.x + br.x - ts.x) * 0.5f, bottom_stack_y}, col_weapon, wn);
            }

            if (cfg::esp::line) {
                dl->AddLine(ImVec2(screen.x * 0.5f, 0.f), ImVec2(cx, tl.y), col_box, 1.5f);
            }

            if (cfg::esp::tracer) {
                dl->AddLine(center, {cx, br.y}, col_tracer, 1.6f);
            }
        }
    }
}
