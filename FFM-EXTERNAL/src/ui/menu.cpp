#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "widgets/widgets.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Android_draw/draw.h"
#include "protect/oxorany.hpp"
#include "styles/styles.h"
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <inttypes.h>
#include <thread>
#include "png/image.hpp"

#include "png/images/trick.hpp"

#include "func/esp.hpp"
#include "func/silent.hpp"
#include "cfg.hpp"

namespace ui::menu {
    using namespace style;
    using namespace widgets;
    
    static float  ma   = 0.f;
    static int    tab  = 0;
    static bool   drag = false;
    static ImVec2 doff(0, 0);
    static float  scr_tgt = 0.f;
    static float  scr_cur = 0.f;

    static GLuint g_TrickTex = 0;
    static bool   g_TrickLoaded = false;

    static float mw = 920.f;
    static float mh = 620.f;
    static float sw = 260.f;
    
    static constexpr float R  = 14.f;
    static constexpr float Rt =  6.f;

    const char* tabs[] = {"Info", "Visuals", "Silent", "Settings"};
    static constexpr int tc = sizeof(tabs) / sizeof(tabs[0]);

    static float lrp(float a, float b, float t) { 
        return a + (b - a) * t; 
    }

    static void slider_cfg(const char* label, float* val, float min, float max, float alpha, const char* fmt = "%.1f") {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::TextColored({0.5f,0.5f,0.5f,alpha}, "%s", label);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::PushItemWidth(120.f * S);
        ImGui::SliderFloat((std::string("##") + label).c_str(), val, min, max, fmt);
        ImGui::PopItemWidth();
    }

    static void check(const char* label, bool* v) {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::Checkbox(label, v);
    }

    static void shadow(ImDrawList* bg, ImVec2 p, ImVec2 s, float a) {
        if (a < 0.01f) return;
        bg->AddRectFilled({p.x-3, p.y-3}, {p.x+s.x+3, p.y+s.y+3}, IM_COL32(0,0,0,(int)(50*a)), R+3.f);
        bg->AddRectFilled({p.x-6, p.y-6}, {p.x+s.x+6, p.y+s.y+6}, IM_COL32(0,0,0,(int)(35*a)), R+5.f);
        bg->AddRectFilled({p.x-10, p.y-10}, {p.x+s.x+10, p.y+s.y+10}, IM_COL32(0,0,0,(int)(20*a)), R+8.f);
        bg->AddRectFilled({p.x-15, p.y-15}, {p.x+s.x+15, p.y+s.y+15}, IM_COL32(0,0,0,(int)(10*a)), R+12.f);
    }

    static void group_header(const char* title, float a) {
        ImGuiWindow* w  = ImGui::GetCurrentWindow();
        ImDrawList*  dl = w->DrawList;
        ImVec2 p  = w->DC.CursorPos;
        float  ww = content_w > 0 ? content_w : ImGui::GetContentRegionAvail().x;
        float  h  = 18.f * S;

        dl->AddRectFilled(p, {p.x+ww, p.y+h}, IM_COL32(0,0,0,(int)(130*a)), Rt);
        dl->AddRectFilled(p, {p.x+4.f, p.y+h}, col(UI::accent, a), Rt, ImDrawFlags_RoundCornersLeft);

        ImGui::PushFont(fontBold);
        dl->AddText({p.x+12.f, p.y+(h-fontBold->FontSize)*0.5f}, col(UI::text, a), title);
        ImGui::PopFont();
        ImGui::Dummy({0, h+3.f*S});
    }

    static void tab_info(float a) {
        group_header(oxorany("Information"), a);

        ImGui::PushFont(fontBold);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::TextColored(ImVec4(UI::accent.x, UI::accent.y, UI::accent.z, a), "%s", oxorany("Triple Boys"));
        ImGui::PopFont();

        ImGui::PushFont(fontDesc);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::TextColored({0.85f,0.85f,0.85f,a}, "%s", oxorany("Version: 1.0"));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::TextColored({0.85f,0.85f,0.85f,a}, "%s", oxorany("By: @FFDKH4X"));
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::TextColored({0.5f,0.5f,0.5f,a}, oxorany("LIB: 0x%lx"), proc::lib);
        ImGui::PopFont();

        if (g_TrickLoaded) {
            float img_sz = 160.f * S;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 55.f * S);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 90.f * S);
            ImGui::Image((void*)(intptr_t)g_TrickTex, ImVec2(img_sz, img_sz), ImVec2(0,0), ImVec2(1,1),
                         ImVec4(1,1,1,a), ImVec4(UI::accent.x, UI::accent.y, UI::accent.z, 0.45f * a));
        }
        separator(a);
    }

    static void tab_visuals(float a) {
        group_header(oxorany("Visuals (ESP)"), a);

        begin_card(oxorany("ESP Display Elements"), a);
        toggle(oxorany("Target Box"),        &cfg::esp::box, a);
        toggle(oxorany("Snapline (Line)"),   &cfg::esp::line, a);
        toggle(oxorany("Health Bar"),        &cfg::esp::health, a);
        toggle(oxorany("Distance Label"),    &cfg::esp::distance, a);
        end_card(a);

        begin_card(oxorany("Visual Appearance"), a);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        const char* box_types[] = { oxorany("Corner Box"), oxorany("Full Box") };
        ImGui::PushItemWidth(130.f * S);
        ImGui::Combo(oxorany("##BoxType"), &cfg::esp::box_type, box_types, 2);
        ImGui::PopItemWidth();
        ImGui::Dummy({0, 4.f*S});

        slider_styled(oxorany("Box Thickness"), &cfg::esp::box_thickness, 0.5f, 4.0f, a, "%.1f px");
        slider_styled(oxorany("ESP Max Range"),  &cfg::esp::max_range,     50.f, 500.f, a, "%.0f m");

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::ColorEdit4(oxorany("Box Color"), &cfg::esp::box_col.x,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();
        ImGui::PushFont(fontMedium);
        ImGui::TextColored({UI::text_dim.x, UI::text_dim.y, UI::text_dim.z, a}, "%s", oxorany("Box Accent Color"));
        ImGui::PopFont();
        ImGui::Dummy({0, 4.f*S});
        end_card(a);

        separator(a);
    }

    static void tab_silent(float a) {
        group_header(oxorany("Silent Aim"), a);

        begin_card(oxorany("Aim Trigger & Protection"), a);
        toggle(oxorany("Enable Silent Aim"), &cfg::aim::visible_enabled, a);
        toggle(oxorany("Target Knocked"),    &cfg::aim::target_knocked, a);
        toggle(oxorany("Wipe Aim Counter"),  &cfg::aim::wipe_aim_counter, a);
        toggle(oxorany("Draw FOV Circle"),   &cfg::aim::fov_circle, a);
        end_card(a);

        begin_card(oxorany("Targeting Tuning"), a);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        const char* target_positions[] = { oxorany("Head"), oxorany("Neck"), oxorany("Chest") };
        ImGui::PushItemWidth(140.f * S);
        ImGui::Combo(oxorany("##TargetPosition"), &cfg::aim::target_position, target_positions, 3);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::PushFont(fontMedium);
        ImGui::TextColored({UI::text_dim.x, UI::text_dim.y, UI::text_dim.z, a}, "%s", oxorany("Target Location"));
        ImGui::PopFont();
        ImGui::Dummy({0, 4.f*S});

        slider_styled(oxorany("FOV Radius"), &cfg::aim::visible_fov, 30.f, 500.f, a, "%.0f px");
        slider_styled(oxorany("Max Range"),  &cfg::aim::visible_max, 50.f, 500.f, a, "%.0f m");
        slider_styled(oxorany("Head Bias"),  &cfg::aim::head_bias,   -0.30f, 0.30f, a, "%.2f");

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        ImGui::ColorEdit4(oxorany("FOV Color"), &cfg::aim::fov_col.x,
                          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
        ImGui::SameLine();
        ImGui::PushFont(fontMedium);
        ImGui::TextColored({UI::text_dim.x, UI::text_dim.y, UI::text_dim.z, a}, "%s", oxorany("FOV Circle Color"));
        ImGui::PopFont();
        ImGui::Dummy({0, 4.f*S});
        end_card(a);

        separator(a);
    }

    static void tab_settings(float a) {
        group_header(oxorany("Settings & Performance"), a);

        begin_card(oxorany("Thermal Pacing (FPS Limit)"), a);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        const char* fps_modes[] = {
            oxorany("60 FPS (Cool / Battery Saver)"),
            oxorany("90 FPS (Balanced)"),
            oxorany("120 FPS (Ultra Smooth)"),
            oxorany("Max (Uncapped)")
        };
        ImGui::PushItemWidth(220.f * S);
        ImGui::Combo(oxorany("##FPSLimit"), &cfg::settings::fps_choice, fps_modes, 4);
        ImGui::PopItemWidth();
        ImGui::Dummy({0, 6.f*S});

        ImGui::PushFont(fontMedium);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        float current_fps = ImGui::GetIO().Framerate;
        float frame_ms = 1000.f / ImMax(current_fps, 1.f);
        ImGui::TextColored({UI::text_dim.x, UI::text_dim.y, UI::text_dim.z, a},
                           oxorany("Screen: %.0f x %.0f  |  FPS: %.0f (%.1f ms)"),
                           g_sw, g_sh, current_fps, frame_ms);
        ImGui::PopFont();
        ImGui::Dummy({0, 4.f*S});
        end_card(a);

        begin_card(oxorany("Client Control"), a);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f*S);
        UI::ButtonStyle();
        if (ImGui::Button(oxorany("Minimize Overlay"))) { bar::set_open(false); }
        ImGui::SameLine();
        if (ImGui::Button(oxorany("Unload Client"))) { exit(0); }
        UI::PopButtonStyle();
        ImGui::Dummy({0, 4.f*S});
        end_card(a);

        separator(a);
    }

    void render() {
        func::esp::draw();
        silent::draw_overlay();
        bar::render();

        if (!g_TrickLoaded) {
            int w, h;
            if (ui::image::LoadTextureFromMemory(trick_data.data(), trick_SIZE, &g_TrickTex, &w, &h)) {
                g_TrickLoaded = true;
            }
        }

        float dt = ImGui::GetIO().DeltaTime;
        ma = lrp(ma, bar::g_open ? 1.f : 0.f, ImClamp(12.f*dt, 0.f, 1.f));

        if (ma > 0.01f) {
            int da = (int)(200*ma*bar::game_alpha());
            ImGui::GetBackgroundDrawList()->AddRectFilled({0,0},{g_sw,g_sh},IM_COL32(0,0,0,da));
        }
        if (ma < 0.01f) return;

        tick();

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 4.f*S});
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_Border, {0,0,0,0});

        ImVec2 wsz(mw, mh);
        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        ImGui::SetNextWindowPos({(g_sw-wsz.x)*0.5f,(g_sh-wsz.y)*0.5f}, ImGuiCond_Once);

        ImGui::Begin("##m", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            ImVec2 wp  = ImGui::GetWindowPos();
            ImVec2 mp  = ImGui::GetMousePos();
            ImDrawList* dl  = ImGui::GetWindowDrawList();
            ImDrawList* bg = ImGui::GetBackgroundDrawList();

            shadow(bg, wp, wsz, ma);
            bg->AddRectFilled(wp, {wp.x+wsz.x, wp.y+wsz.y}, IM_COL32(8,8,10,(int)(245*ma)), R);

            ImVec2 smin = wp;
            ImVec2 smax = {wp.x+sw, wp.y+wsz.y};
            bg->AddRectFilled(smin, smax, IM_COL32(5,5,7,(int)(250*ma)), R, ImDrawFlags_RoundCornersLeft);
            bg->AddLine({smax.x, smin.y+R}, {smax.x, smax.y-R}, col(UI::border,ma));

            float hh = 44.f;
            bg->AddRectFilled(smin, {smax.x, smin.y+hh}, IM_COL32(14,8,10,(int)(250*ma)), R, ImDrawFlags_RoundCornersTopLeft);
            bg->AddLine({smin.x+R, smin.y+hh}, {smax.x, smin.y+hh}, col(UI::border,ma));

            ImDrawList* fg = ImGui::GetForegroundDrawList();
            fg->AddRect(wp, {wp.x+wsz.x, wp.y+wsz.y}, col(UI::border,ma), R);

            // Breathing neon crimson outer glow around main window
            float pulse = 0.5f + 0.5f * sinf(ImGui::GetTime() * 2.2f);
            int glow_a = (int)((35 + 30 * pulse) * ma);
            fg->AddRect(wp, {wp.x+wsz.x, wp.y+wsz.y}, IM_COL32(195, 24, 38, glow_a), R, 0, 1.8f);

            // Minimize button in top-right
            float mbtn_w = 32.f;
            float mbtn_h = 24.f;
            ImVec2 mmin(wp.x + wsz.x - mbtn_w - 12.f, wp.y + 10.f);
            ImVec2 mmax(mmin.x + mbtn_w, mmin.y + mbtn_h);
            bool mhov = (mp.x >= mmin.x && mp.x <= mmax.x && mp.y >= mmin.y && mp.y <= mmax.y);
            dl->AddRectFilled(mmin, mmax, mhov ? IM_COL32(195, 24, 38, (int)(220*ma)) : IM_COL32(24, 20, 24, (int)(180*ma)), Rt);
            dl->AddRect(mmin, mmax, col(UI::border, ma), Rt);
            ImGui::PushFont(fontBold);
            const char* msign = "-";
            ImVec2 mst = ImGui::CalcTextSize(msign);
            dl->AddText({mmin.x + (mbtn_w - mst.x)*0.5f, mmin.y + (mbtn_h - mst.y)*0.5f}, col(UI::text, ma), msign);
            ImGui::PopFont();
            if (mhov && ImGui::IsMouseClicked(0)) {
                bar::set_open(false);
            }

            ImGui::PushFont(fontBold);
            const char* ttl = "Crazy CHEATS";
            ImVec2 ts = ImGui::CalcTextSize(ttl);
            dl->AddText({smin.x+(sw-ts.x)*0.5f, smin.y+(hh-ts.y)*0.5f}, col(UI::accent,ma), ttl);
            ImGui::PopFont();

            float th  = 32.f, tg = 3.f;
            float tsy = smin.y + hh + 8.f;

            // Smooth sliding active tab indicator
            float target_ind_y = tsy + tab * (th + tg);
            float current_ind_y = style::anim("sidebar_slide_y", target_ind_y, 18.f);
            ImVec2 ind_min = {smin.x + 4.f, current_ind_y};
            ImVec2 ind_max = {smax.x - 4.f, current_ind_y + th};
            dl->AddRectFilled(ind_min, ind_max, IM_COL32(32, 12, 16, (int)(220 * ma)), Rt);
            dl->AddRectFilled(ind_min, {ind_min.x + 4.f, ind_max.y}, col(UI::accent, ma), Rt, ImDrawFlags_RoundCornersLeft);

            for (int i = 0; i < tc; i++) {
                float   ty   = tsy + i*(th+tg);
                ImVec2  tmin {smin.x+4.f, ty};
                ImVec2  tmax {smax.x-4.f, ty+th};
                bool    hov  = (mp.x>=tmin.x&&mp.x<=tmax.x&&mp.y>=tmin.y&&mp.y<=tmax.y);
                bool    sel  = (tab == i);

                // If hovered and not selected, show a subtle hover background
                if (hov && !sel) {
                    dl->AddRectFilled(tmin, tmax, IM_COL32(20, 10, 12, (int)(140 * ma)), Rt);
                }

                ImGui::PushFont(fontMedium);
                ImVec2 ts2 = ImGui::CalcTextSize(tabs[i]);
                dl->AddText({tmin.x+14.f, ty+(th-ts2.y)*0.5f}, col(sel ? UI::text : (hov ? UI::text_light : UI::text_dim),ma), tabs[i]);
                ImGui::PopFont();
                if (hov && ImGui::IsMouseClicked(0) && !popup()) tab = i;
            }

            // Hide Menu button at bottom of sidebar
            float hbw = sw - 16.f;
            float hbh = 32.f;
            ImVec2 hmin(smin.x + 8.f, smax.y - hbh - 10.f);
            ImVec2 hmax(hmin.x + hbw, hmin.y + hbh);
            bool hov_hide = (mp.x >= hmin.x && mp.x <= hmax.x && mp.y >= hmin.y && mp.y <= hmax.y);
            dl->AddRectFilled(hmin, hmax, hov_hide ? IM_COL32(120, 18, 26, (int)(220*ma)) : IM_COL32(18, 16, 20, (int)(180*ma)), Rt);
            dl->AddRect(hmin, hmax, col(hov_hide ? UI::accent : UI::border, ma), Rt);
            ImGui::PushFont(fontMedium);
            const char* htxt = oxorany("Hide Menu");
            ImVec2 hts = ImGui::CalcTextSize(htxt);
            dl->AddText({hmin.x + (hbw - hts.x) * 0.5f, hmin.y + (hbh - hts.y) * 0.5f}, col(hov_hide ? UI::accent : UI::text_dim, ma), htxt);
            ImGui::PopFont();
            if (hov_hide && ImGui::IsMouseClicked(0) && !popup()) {
                bar::set_open(false);
            }

            float cx = wp.x+sw+6.f, cy = wp.y+6.f;
            float cw = wsz.x-sw-16.f, ch = wsz.y-12.f;
            ImVec2 cpos{cx,cy}, cmax_{cx+cw,cy+ch};

            // reserva ~8px na direita pra scrollbar visível
            constexpr float SBW = 8.f;
            float cw_body = cw - (SBW + 4.f);

            content_w = cw_body; content_alpha = ma;
            dl->PushClipRect(cpos, {cpos.x + cw_body, cmax_.y}, true);
            ImGui::SetCursorScreenPos(cpos);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0,0,0,0});
            ImGui::BeginChild("##c", {cw_body, ch}, false, ImGuiWindowFlags_NoScrollbar);

            float sm = ImGui::GetScrollMaxY();

            bool inContent = (mp.x >= cpos.x && mp.x <= cpos.x + cw_body &&
                              mp.y >= cpos.y && mp.y <= cmax_.y);

            // wheel (desktop)
            if (inContent && !popup()) {
                float w = ImGui::GetIO().MouseWheel;
                if (w != 0.f) scr_tgt -= w * 60.f;
            }

            // ── DRAG-TO-SCROLL: arrastar dedo em qualquer área vazia do conteúdo ──
            // Só ativa quando nada tá sendo interagido (slider/checkbox/etc.).
            static bool  content_drag = false;
            static float content_drag_start_y   = 0.f;
            static float content_drag_start_off = 0.f;
            if (!popup() && inContent && !ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered()
                && ImGui::IsMouseClicked(0)) {
                content_drag = true;
                content_drag_start_y   = mp.y;
                content_drag_start_off = scr_tgt;
            }
            if (content_drag) {
                if (ImGui::IsMouseDown(0)) {
                    scr_tgt = content_drag_start_off - (mp.y - content_drag_start_y);
                    scr_cur = scr_tgt;                       // resposta imediata
                } else {
                    content_drag = false;
                }
            }

            scr_tgt = ImClamp(scr_tgt, 0.f, ImMax(sm, 0.f));
            if (!content_drag)
                scr_cur = lrp(scr_cur, scr_tgt, ImClamp(16.f * dt, 0.f, 1.f));
            else
                scr_cur = ImClamp(scr_cur, 0.f, ImMax(sm, 0.f));
            ImGui::SetScrollY(scr_cur);

            switch (tab) {
                case 0: tab_info(ma); break;
                case 1: tab_visuals(ma); break;
                case 2: tab_silent(ma); break;
                case 3: tab_settings(ma); break;
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            dl->PopClipRect();

            // ── scrollbar visível na direita, arrastável ──
            if (sm > 0.f) {
                float sbx = cpos.x + cw_body + 2.f;
                float sb_top = cpos.y + 4.f;
                float sb_bot = cmax_.y - 4.f;
                float sb_h   = sb_bot - sb_top;

                float visible_ratio = ch / (ch + sm);
                if (visible_ratio > 1.f) visible_ratio = 1.f;
                float thumb_h = ImMax(sb_h * visible_ratio, 24.f);
                float scroll_ratio = scr_cur / sm;
                if (scroll_ratio < 0.f) scroll_ratio = 0.f;
                if (scroll_ratio > 1.f) scroll_ratio = 1.f;
                float thumb_y = sb_top + (sb_h - thumb_h) * scroll_ratio;

                // hit-test drag: só na área da scrollbar (não conflita com sliders)
                bool sb_hovered = (mp.x >= sbx - 4.f && mp.x <= sbx + SBW + 4.f &&
                                   mp.y >= sb_top    && mp.y <= sb_bot);
                static bool  sb_dragging = false;
                static float sb_grab_off = 0.f;
                if (sb_hovered && !popup() && ImGui::IsMouseClicked(0)) {
                    sb_dragging = true;
                    // se clicou fora do thumb, salta pra posição
                    if (mp.y < thumb_y || mp.y > thumb_y + thumb_h) {
                        sb_grab_off = thumb_h * 0.5f;
                    } else {
                        sb_grab_off = mp.y - thumb_y;
                    }
                }
                if (sb_dragging) {
                    if (ImGui::IsMouseDown(0)) {
                        float new_thumb_y = mp.y - sb_grab_off;
                        float new_ratio = (new_thumb_y - sb_top) / (sb_h - thumb_h);
                        if (new_ratio < 0.f) new_ratio = 0.f;
                        if (new_ratio > 1.f) new_ratio = 1.f;
                        scr_tgt = new_ratio * sm;
                        scr_cur = scr_tgt;   // resposta imediata durante drag
                    } else {
                        sb_dragging = false;
                    }
                }

                ImDrawList* fg = ImGui::GetForegroundDrawList();
                // track
                fg->AddRectFilled({sbx, sb_top}, {sbx + SBW, sb_bot},
                                  IM_COL32(255, 255, 255, (int)(20 * ma)), SBW * 0.5f);
                // thumb
                ImU32 thumb_col = sb_dragging ? col(UI::accent_light, ma) : col(UI::accent, ma);
                fg->AddRectFilled({sbx, thumb_y}, {sbx + SBW, thumb_y + thumb_h},
                                  thumb_col, SBW * 0.5f);
            }

            if ((mp.x>=wp.x&&mp.x<=wp.x+wsz.x&&mp.y>=wp.y&&mp.y<=wp.y+wsz.y) && !popup() && ImGui::IsMouseClicked(0)) {
                if (mp.y < wp.y + hh) { 
                    drag = true; 
                    doff = {mp.x-wp.x, mp.y-wp.y}; 
                }
            }
            if (drag) {
                if (ImGui::IsMouseDown(0)) {
                    ImVec2 np{mp.x-doff.x, mp.y-doff.y};
                    np.x = ImClamp(np.x, 0.f, g_sw-wsz.x);
                    np.y = ImClamp(np.y, 0.f, g_sh-wsz.y);
                    ImGui::SetWindowPos("##m", np);
                } else {
                    drag = false;
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
        popups();
    }
}