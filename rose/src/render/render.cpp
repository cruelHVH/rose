    #define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX
#include <render/render.h>
#include "render_helpers.h"
#include "notifications.h"
#include <features/silent/silent.h>
#include <dwmapi.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <shellapi.h>

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

typedef BOOL(WINAPI* SetWindowDisplayAffinityProc)(HWND, DWORD);

#include <settings.h>
#include <check/typing_check.h>
#include <features/esp/esp.h>
#include <features/silent/silent.h>
#include <features/explorer/dex_explorer.h>
#include "visitor.h"
#include "../resources/WeaponIcon.hpp"
#include "../config/config.h"
#include <memory/memory.h>
#include <sdk/offsets.h>
#include <game/rescan.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


// helpers
static int selected_tab_index = 0;
static bool test_label_1 = false;
static bool test_label_2 = false;
static bool test_label_3 = false;
static bool test_label_4 = false;
static float niggerKyzo = 0.0f;
static float test_slider = 0.0f;
static float color_array[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static bool waiting_for_keybind = false;
static std::unordered_map<std::string, bool> waiting_for_keybind_map;

static const char* get_key_name(int vk_code)
{
    switch (vk_code)
    {
    case 0: return "None";
    case VK_LBUTTON: return "LM";
    case VK_RBUTTON: return "RM";
    case VK_MBUTTON: return "MM";
    case VK_XBUTTON1: return "MB1";
    case VK_XBUTTON2: return "MB2";
    case VK_BACK: return "Backspace";
    case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Ctrl";
    case VK_MENU: return "Alt";
    case VK_CAPITAL: return "Caps";
    case VK_ESCAPE: return "Esc";
    case VK_SPACE: return "Space";
    case VK_PRIOR: return "PgUp";
    case VK_NEXT: return "PgDown";
    case VK_END: return "End";
    case VK_HOME: return "Home";
    case VK_LEFT: return "Left";
    case VK_UP: return "Up";
    case VK_RIGHT: return "Right";
    case VK_DOWN: return "Down";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    default:
        if (vk_code >= 'A' && vk_code <= 'Z') { static char buf[2]; buf[0] = (char)vk_code; buf[1] = 0; return buf; }
        if (vk_code >= '0' && vk_code <= '9') { static char buf[2]; buf[0] = (char)vk_code; buf[1] = 0; return buf; }
        return "???";
    }
}

static std::unordered_map<std::string, float> keybind_tween_progress;
static std::unordered_map<std::string, bool> keybind_popup_open;
static std::unordered_map<std::string, ImVec2> keybind_popup_pos;
static std::unordered_map<std::string, bool> multiselect_popup_open;
static std::unordered_map<std::string, ImVec2> multiselect_popup_pos;

static bool inline_keybind_button(const char* label, int* key, int* mode = nullptr)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    std::string key_id = label;
    if (waiting_for_keybind_map.find(key_id) == waiting_for_keybind_map.end())
        waiting_for_keybind_map[key_id] = false;

    bool is_waiting = waiting_for_keybind_map[key_id];
    const char* display_text;
    if (is_waiting)
        display_text = "..";
    else if (*key == 0)
        display_text = "-";
    else
        display_text = get_key_name(*key);

    ImVec2 label_size = ImGui::CalcTextSize(display_text, nullptr, true);
    float checkbox_height = ImGui::GetFrameHeight();
    ImVec2 button_size = ImVec2(label_size.x + style.FramePadding.x * 2.0f + 16.0f, checkbox_height);
    if (button_size.x < 40.0f) button_size.x = 40.0f;

    ImVec2 pos = window->DC.CursorPos;

    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(label)))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

    if (keybind_popup_open.find(key_id) == keybind_popup_open.end())
        keybind_popup_open[key_id] = false;

    if (mode != nullptr && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        keybind_popup_open[key_id] = !keybind_popup_open[key_id];
        keybind_popup_pos[key_id] = ImVec2(pos.x + button_size.x + 5, pos.y);
    }

    if (pressed)
        waiting_for_keybind_map[key_id] = !waiting_for_keybind_map[key_id];

    is_waiting = waiting_for_keybind_map[key_id];
    if (is_waiting)
    {
        for (int i = 1; i < 256; i++)
        {
            if (i == VK_ESCAPE)
            {
                if (GetAsyncKeyState(i) & 0x8000)
                {
                    *key = 0;
                    waiting_for_keybind_map[key_id] = false;
                    break;
                }
                continue;
            }
            
            if (GetAsyncKeyState(i) & 0x8000)
            {
                *key = i;
                waiting_for_keybind_map[key_id] = false;
                break;
            }
        }
    }

    if (keybind_tween_progress.find(key_id) == keybind_tween_progress.end())
        keybind_tween_progress[key_id] = 0.0f;

    float& tween_progress = keybind_tween_progress[key_id];

    ImVec4 base_color = ImVec4(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);
    ImVec4 target_color = menu::accent_color;

    float target_progress = 0.0f;
    if (is_waiting || held)
        target_progress = 1.0f;
    else if (hovered)
        target_progress = 0.5f;

    float tween_speed = 4.0f;
    tween_progress += (target_progress - tween_progress) * g.IO.DeltaTime * tween_speed;

    ImVec4 tweened_color = ImVec4(
        base_color.x + (target_color.x - base_color.x) * tween_progress,
        base_color.y + (target_color.y - base_color.y) * tween_progress,
        base_color.z + (target_color.z - base_color.z) * tween_progress,
        base_color.w + (target_color.w - base_color.w) * tween_progress
    );

    ImDrawList* dl = ImGui::GetWindowDrawList();

    char bracket_text[64];
    sprintf_s(bracket_text, "[ %s ]", display_text);
    ImVec2 bracket_text_size = ImGui::CalcTextSize(bracket_text);
    float text_y = pos.y + style.FramePadding.y - 4.0f;
    ImVec2 text_pos = ImVec2(bb.Min.x + (button_size.x - bracket_text_size.x) * 0.5f, text_y);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            dl->AddText(ImVec2(text_pos.x + x * 1.f, text_pos.y + y * 1.f), IM_COL32_BLACK, bracket_text);
        }
    }
    dl->AddText(text_pos, IM_COL32_WHITE, bracket_text);

    if (mode != nullptr && keybind_popup_open[key_id])
    {
        ImGui::SetNextWindowPos(keybind_popup_pos[key_id], ImGuiCond_Always);
        
        bool is_walkspeed = (strcmp(label, "walkspeed_keybind") == 0);
        bool is_silent = (strcmp(label, "silent_keybind") == 0);
        int mode_count = (is_walkspeed || is_silent) ? 3 : 2;
        ImVec2 popup_size = ImVec2(80, mode_count == 3 ? 100 : 80);
        ImGui::SetNextWindowSize(popup_size);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(menu::accent_color));

        char popup_name[128];
        sprintf_s(popup_name, "##keybind_mode_%s", label);

        ImGui::Begin(popup_name, nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize);

        ImVec2 popup_pos = ImGui::GetWindowPos();
        popup_size = ImGui::GetWindowSize();
        ImDrawList* popup_dl = ImGui::GetWindowDrawList();

        const char* modes[3];
        if (is_walkspeed || is_silent)
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
            modes[2] = "Always";
        }
        else
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
        }

        for (int i = 0; i < mode_count; i++)
        {
            ImVec2 item_pos = ImGui::GetCursorScreenPos();
            ImVec2 item_size = ImVec2(popup_size.x - 12, 20);
            ImRect item_bb(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y));

            bool item_hovered = ImGui::IsMouseHoveringRect(item_bb.Min, item_bb.Max);
            bool item_clicked = item_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            ImU32 item_bg = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(ImVec4(menu::accent_color.x * 0.3f, menu::accent_color.y * 0.3f, menu::accent_color.z * 0.3f, 1.0f)) :
                           (item_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255));
            popup_dl->AddRectFilled(item_bb.Min, item_bb.Max, item_bg);

            ImVec2 mode_text_size = ImGui::CalcTextSize(modes[i]);
            ImVec2 mode_text_pos = ImVec2(item_bb.Min.x + (item_size.x - mode_text_size.x) * 0.5f, item_bb.Min.y + (item_size.y - mode_text_size.y) * 0.5f);

            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    if (x == 0 && y == 0) continue;
                    popup_dl->AddText(ImVec2(mode_text_pos.x + x, mode_text_pos.y + y), IM_COL32(0, 0, 0, 255), modes[i]);
                }
            }
            
            ImU32 text_col = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(menu::accent_color) : IM_COL32(255, 255, 255, 255);
            popup_dl->AddText(mode_text_pos, text_col, modes[i]);

            if (item_clicked)
            {
                *mode = i;
                keybind_popup_open[key_id] = false;
            }

            ImGui::Dummy(item_size);
        }

        ImVec2 separator_start = ImGui::GetCursorScreenPos();
        separator_start.y += 2;
        ImVec2 separator_end = ImVec2(separator_start.x + popup_size.x - 12, separator_start.y);
        popup_dl->AddLine(separator_start, separator_end, IM_COL32(60, 60, 60, 255));
        ImGui::Dummy(ImVec2(0, 6));

        ImVec2 clear_item_pos = ImGui::GetCursorScreenPos();
        ImVec2 clear_item_size = ImVec2(popup_size.x - 12, 20);
        ImRect clear_item_bb(clear_item_pos, ImVec2(clear_item_pos.x + clear_item_size.x, clear_item_pos.y + clear_item_size.y));

        bool clear_hovered = ImGui::IsMouseHoveringRect(clear_item_bb.Min, clear_item_bb.Max);
        bool clear_clicked = clear_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        ImU32 clear_bg = clear_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255);
        popup_dl->AddRectFilled(clear_item_bb.Min, clear_item_bb.Max, clear_bg);

        const char* clear_text = "Clear";
        ImVec2 clear_text_size = ImGui::CalcTextSize(clear_text);
        ImVec2 clear_text_pos = ImVec2(clear_item_bb.Min.x + (clear_item_size.x - clear_text_size.x) * 0.5f, clear_item_bb.Min.y + (clear_item_size.y - clear_text_size.y) * 0.5f);

        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                if (x == 0 && y == 0) continue;
                popup_dl->AddText(ImVec2(clear_text_pos.x + x, clear_text_pos.y + y), IM_COL32(0, 0, 0, 255), clear_text);
            }
        }
        
        popup_dl->AddText(clear_text_pos, IM_COL32(255, 255, 255, 255), clear_text);

        if (clear_clicked)
        {
            *key = 0;
            keybind_popup_open[key_id] = false;
        }

        ImGui::Dummy(clear_item_size);

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    return pressed;
}

static bool keybind_button(const char* label, int* key, int* mode = nullptr)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const char* display_text = waiting_for_keybind ? "..." : get_key_name(*key);
    
    ImVec2 label_size = ImGui::CalcTextSize(display_text, nullptr, true);
    ImVec2 button_size = ImVec2(label_size.x + style.FramePadding.x * 4.0f, label_size.y + style.FramePadding.y * 2.0f);
    if (button_size.x < 50.0f) button_size.x = 50.0f;

    ImVec2 pos = window->DC.CursorPos;
    pos.x += 2;

    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(label)))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

    std::string key_id = label;
    
    if (keybind_popup_open.find(key_id) == keybind_popup_open.end())
        keybind_popup_open[key_id] = false;

    if (mode != nullptr && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        keybind_popup_open[key_id] = !keybind_popup_open[key_id];
        keybind_popup_pos[key_id] = ImVec2(pos.x + button_size.x + 5, pos.y);
    }

    if (pressed)
        waiting_for_keybind = !waiting_for_keybind;

    if (waiting_for_keybind)
    {
        for (int i = 1; i < 256; i++)
        {
            if (i == VK_ESCAPE)
            {
                if (GetAsyncKeyState(i) & 0x8000)
                {
                    *key = 0;
                    waiting_for_keybind = false;
                    break;
                }
                continue;
            }
            
            if (GetAsyncKeyState(i) & 0x8000)
            {
                *key = i;
                waiting_for_keybind = false;
                break;
            }
        }
    }

    ImU32 col = IM_COL32(35, 35, 35, 255);

    if (keybind_tween_progress.find(key_id) == keybind_tween_progress.end())
        keybind_tween_progress[key_id] = 0.0f;

    float& tween_progress = keybind_tween_progress[key_id];

    ImVec4 base_color = ImVec4(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);
    ImVec4 target_color = menu::accent_color;

    float target_progress = 0.0f;
    if (waiting_for_keybind || held)
        target_progress = 1.0f;
    else if (hovered)
        target_progress = 0.5f;

    float tween_speed = 4.0f;
    tween_progress += (target_progress - tween_progress) * g.IO.DeltaTime * tween_speed;

    ImVec4 tweened_color = ImVec4(
        base_color.x + (target_color.x - base_color.x) * tween_progress,
        base_color.y + (target_color.y - base_color.y) * tween_progress,
        base_color.z + (target_color.z - base_color.z) * tween_progress,
        base_color.w + (target_color.w - base_color.w) * tween_progress
    );

    ImDrawList* dl = ImGui::GetWindowDrawList();

    char bracket_text[64];
    const char* display_text_bracket;
    if (waiting_for_keybind)
        display_text_bracket = "..";
    else if (*key == 0)
        display_text_bracket = "-";
    else
        display_text_bracket = display_text;
    
    sprintf_s(bracket_text, "[ %s ]", display_text_bracket);
    ImVec2 bracket_text_size = ImGui::CalcTextSize(bracket_text);
    ImVec2 text_pos = ImVec2(bb.Min.x + (button_size.x - bracket_text_size.x) * 0.5f, bb.Min.y + (button_size.y - bracket_text_size.y) * 0.5f);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            dl->AddText(ImVec2(text_pos.x + x * 1.f, text_pos.y + y * 1.f), IM_COL32_BLACK, bracket_text);
        }
    }
    dl->AddText(text_pos, IM_COL32_WHITE, bracket_text);

    if (mode != nullptr && keybind_popup_open[key_id])
    {
        ImGui::SetNextWindowPos(keybind_popup_pos[key_id], ImGuiCond_Always);
        
        bool is_walkspeed = (strcmp(label, "walkspeed_keybind") == 0);
        bool is_silent = (strcmp(label, "silent_keybind") == 0);
        int mode_count = (is_walkspeed || is_silent) ? 3 : 2;
        ImVec2 popup_size = ImVec2(80, mode_count == 3 ? 100 : 80);
        ImGui::SetNextWindowSize(popup_size);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(menu::accent_color));

        char popup_name[128];
        sprintf_s(popup_name, "##keybind_mode_%s", label);

        ImGui::Begin(popup_name, nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize);

        ImVec2 popup_pos = ImGui::GetWindowPos();
        popup_size = ImGui::GetWindowSize();
        ImDrawList* popup_dl = ImGui::GetWindowDrawList();

        const char* modes[3];
        if (is_walkspeed || is_silent)
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
            modes[2] = "Always";
        }
        else
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
        }

        for (int i = 0; i < mode_count; i++)
        {
            ImVec2 item_pos = ImGui::GetCursorScreenPos();
            ImVec2 item_size = ImVec2(popup_size.x - 12, 20);
            ImRect item_bb(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y));

            bool item_hovered = ImGui::IsMouseHoveringRect(item_bb.Min, item_bb.Max);
            bool item_clicked = item_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            ImU32 item_bg = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(ImVec4(menu::accent_color.x * 0.3f, menu::accent_color.y * 0.3f, menu::accent_color.z * 0.3f, 1.0f)) :
                           (item_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255));
            popup_dl->AddRectFilled(item_bb.Min, item_bb.Max, item_bg);

            ImVec2 mode_text_size = ImGui::CalcTextSize(modes[i]);
            ImVec2 mode_text_pos = ImVec2(item_bb.Min.x + (item_size.x - mode_text_size.x) * 0.5f, item_bb.Min.y + (item_size.y - mode_text_size.y) * 0.5f);

            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    if (x == 0 && y == 0) continue;
                    popup_dl->AddText(ImVec2(mode_text_pos.x + x, mode_text_pos.y + y), IM_COL32(0, 0, 0, 255), modes[i]);
                }
            }
            
            ImU32 text_col = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(menu::accent_color) : IM_COL32(255, 255, 255, 255);
            popup_dl->AddText(mode_text_pos, text_col, modes[i]);

            if (item_clicked)
            {
                *mode = i;
                keybind_popup_open[key_id] = false;
            }

            ImGui::Dummy(item_size);
        }

        ImVec2 separator_start = ImGui::GetCursorScreenPos();
        separator_start.y += 2;
        ImVec2 separator_end = ImVec2(separator_start.x + popup_size.x - 12, separator_start.y);
        popup_dl->AddLine(separator_start, separator_end, IM_COL32(60, 60, 60, 255));
        ImGui::Dummy(ImVec2(0, 6));

        ImVec2 clear_item_pos = ImGui::GetCursorScreenPos();
        ImVec2 clear_item_size = ImVec2(popup_size.x - 12, 20);
        ImRect clear_item_bb(clear_item_pos, ImVec2(clear_item_pos.x + clear_item_size.x, clear_item_pos.y + clear_item_size.y));

        bool clear_hovered = ImGui::IsMouseHoveringRect(clear_item_bb.Min, clear_item_bb.Max);
        bool clear_clicked = clear_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        ImU32 clear_bg = clear_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255);
        popup_dl->AddRectFilled(clear_item_bb.Min, clear_item_bb.Max, clear_bg);

        const char* clear_text = "Clear";
        ImVec2 clear_text_size = ImGui::CalcTextSize(clear_text);
        ImVec2 clear_text_pos = ImVec2(clear_item_bb.Min.x + (clear_item_size.x - clear_text_size.x) * 0.5f, clear_item_bb.Min.y + (clear_item_size.y - clear_text_size.y) * 0.5f);

        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                if (x == 0 && y == 0) continue;
                popup_dl->AddText(ImVec2(clear_text_pos.x + x, clear_text_pos.y + y), IM_COL32(0, 0, 0, 255), clear_text);
            }
        }
        
        popup_dl->AddText(clear_text_pos, IM_COL32(255, 255, 255, 255), clear_text);

        if (clear_clicked)
        {
            *key = 0;
            keybind_popup_open[key_id] = false;
        }

        ImGui::Dummy(clear_item_size);

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    return pressed;
}

static bool styled_button(const char* label, const ImVec2& size = ImVec2(0, 0))
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 button_size = size;
    if (button_size.x == 0) button_size.x = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
    if (button_size.y == 0) button_size.y = ImGui::GetFrameHeight();

    ImVec2 pos = window->DC.CursorPos;
    pos.x += 2;

    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(label)))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

    static std::unordered_map<std::string, float> button_tween_progress;
    std::string button_id = label;
    
    if (button_tween_progress.find(button_id) == button_tween_progress.end())
        button_tween_progress[button_id] = 0.0f;

    float& tween_progress = button_tween_progress[button_id];

    ImVec4 base_color = ImVec4(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);
    ImVec4 target_color = menu::accent_color;

    float target_progress = 0.0f;
    if (held)
        target_progress = 1.0f;
    else if (hovered)
        target_progress = 0.5f;

    float tween_speed = 4.0f;
    tween_progress += (target_progress - tween_progress) * g.IO.DeltaTime * tween_speed;

    ImVec4 tweened_color = ImVec4(
        base_color.x + (target_color.x - base_color.x) * tween_progress,
        base_color.y + (target_color.y - base_color.y) * tween_progress,
        base_color.z + (target_color.z - base_color.z) * tween_progress,
        base_color.w + (target_color.w - base_color.w) * tween_progress
    );

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 bg_col = IM_COL32(35, 35, 35, 255);

    dl->AddRect(ImVec2(pos.x - 1, pos.y - 1), ImVec2(pos.x + button_size.x + 1, pos.y + button_size.y + 1), ImGui::ColorConvertFloat4ToU32(tweened_color), style.FrameRounding);
    dl->AddRect(ImVec2(pos.x - 2, pos.y - 2), ImVec2(pos.x + button_size.x + 2, pos.y + button_size.y + 2), IM_COL32(0, 0, 0, 255), style.FrameRounding);

    dl->AddRectFilled(bb.Min, bb.Max, bg_col, style.FrameRounding);

    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(bb.Min.x + (button_size.x - text_size.x) * 0.5f, bb.Min.y + (button_size.y - text_size.y) * 0.5f);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            dl->AddText(ImVec2(text_pos.x + x * 1.f, text_pos.y + y * 1.f), IM_COL32_BLACK, label);
        }
    }
    dl->AddText(text_pos, IM_COL32_WHITE, label);

    return pressed;
}

static void multiselect_combo(const char* label, bool* fov_check, bool* knocked_check)
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiStyle& style = g.Style;
    
    std::string popup_id = std::string("##multiselect_") + label;
    std::string key_id = std::string(label);
    
    if (multiselect_popup_open.find(key_id) == multiselect_popup_open.end())
        multiselect_popup_open[key_id] = false;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x - 13.f, 20.f);
    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    
    bool hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    
    if (clicked)
    {
        multiselect_popup_open[key_id] = !multiselect_popup_open[key_id];
        if (multiselect_popup_open[key_id])
        {
            multiselect_popup_pos[key_id] = ImVec2(bb.Min.x, bb.Max.y + 2);
        }
    }
    
    ImU32 col = hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255);
    
    std::string display_text = "check";
    std::vector<std::string> selected_items;
    if (*fov_check) selected_items.push_back("fov check");
    if (*knocked_check) selected_items.push_back("knocked check");
    
    if (!selected_items.empty())
    {
        display_text = selected_items[0];
        for (size_t i = 1; i < selected_items.size(); i++)
        {
            display_text += ", " + selected_items[i];
        }
    }
    
    ImVec2 label_size = ImGui::CalcTextSize(display_text.c_str());
    if (label_size.x > button_size.x - 20)
    {
        display_text = std::to_string(selected_items.size()) + " selected";
        label_size = ImGui::CalcTextSize(display_text.c_str());
    }
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(bb.Min, bb.Max, col, style.FrameRounding);
    
    ImVec2 text_pos = ImVec2(bb.Min.x + 8, bb.Min.y + (button_size.y - label_size.y) * 0.5f);
    dl->AddText(text_pos, IM_COL32(255, 255, 255, 255), display_text.c_str());
    
    ImVec2 arrow_pos = ImVec2(bb.Max.x - 15, bb.Min.y + (button_size.y - 8) * 0.5f);
    dl->AddTriangleFilled(
        arrow_pos,
        ImVec2(arrow_pos.x + 8, arrow_pos.y),
        ImVec2(arrow_pos.x + 4, arrow_pos.y + 8),
        IM_COL32(255, 255, 255, 255)
    );
    
    ImGui::Dummy(button_size);
    
    if (multiselect_popup_open[key_id])
    {
        ImGui::SetNextWindowPos(multiselect_popup_pos[key_id], ImGuiCond_Always);
        ImVec2 popup_size = ImVec2(button_size.x, 50);
        ImGui::SetNextWindowSize(popup_size);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(menu::accent_color));
        
        ImGui::Begin(popup_id.c_str(), nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::Checkbox("FOV Check", fov_check);
        ImGui::Checkbox("Knocked Check", knocked_check);
        
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }
    
    if (multiselect_popup_open[key_id] && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 popup_min = multiselect_popup_pos[key_id];
        ImVec2 popup_max = ImVec2(popup_min.x + button_size.x, popup_min.y + 50);
        
        if (!(mouse_pos.x >= popup_min.x && mouse_pos.x <= popup_max.x &&
              mouse_pos.y >= popup_min.y && mouse_pos.y <= popup_max.y) &&
            !(mouse_pos.x >= bb.Min.x && mouse_pos.x <= bb.Max.x &&
              mouse_pos.y >= bb.Min.y && mouse_pos.y <= bb.Max.y))
        {
            multiselect_popup_open[key_id] = false;
        }
    }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_F4) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

render_t::render_t()
{
    detail = std::make_unique<detail_t>();
}

render_t::~render_t()
{
    destroy_imgui();
    destroy_window();
    destroy_device();
}

bool render_t::create_window()
{
    detail->window_class.cbSize = sizeof(detail->window_class);
    detail->window_class.style = CS_CLASSDC;
    detail->window_class.lpszClassName = "T4";
    detail->window_class.hInstance = GetModuleHandleA(0);
    detail->window_class.lpfnWndProc = wnd_proc;

    RegisterClassExA(&detail->window_class);

    detail->window = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        detail->window_class.lpszClassName,
        "T4",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        0,
        0,
        detail->window_class.hInstance,
        0
    );

    if (!detail->window)
    {
        return false;
    }

    SetLayeredWindowAttributes(detail->window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    RECT client_area{};
    RECT window_area{};

    GetClientRect(detail->window, &client_area);
    GetWindowRect(detail->window, &window_area);

    POINT diff{};
    ClientToScreen(detail->window, &diff);

    MARGINS margins
    {
        window_area.left + (diff.x - window_area.left),
        window_area.top + (diff.y - window_area.top),
        window_area.right,
        window_area.bottom,
    };

    DwmExtendFrameIntoClientArea(detail->window, &margins);

    ShowWindow(detail->window, SW_SHOW);
    UpdateWindow(detail->window);

    return true;
}

bool render_t::create_device()
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};

    swap_chain_desc.BufferCount = 1;

    swap_chain_desc.BufferDesc.Width = 0;
    swap_chain_desc.BufferDesc.Height = 0;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    swap_chain_desc.OutputWindow = detail->window;

    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Windowed = 1;

    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    swap_chain_desc.SampleDesc.Count = 2;
    swap_chain_desc.SampleDesc.Quality = 0;

    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL feature_level;
    D3D_FEATURE_LEVEL feature_level_list[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_level_list,
        2,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &detail->swap_chain,
        &detail->device,
        &feature_level,
        &detail->device_context
    );

    if (result == DXGI_ERROR_UNSUPPORTED)
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            feature_level_list,
            2,
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &detail->swap_chain,
            &detail->device,
            &feature_level,
            &detail->device_context
        );
    }

    if (result != S_OK)
    {
        MessageBoxA(nullptr, "This software can not run on your computer.", "Critical Problem", MB_ICONERROR | MB_OK);
    }

    ID3D11Texture2D* back_buffer{ nullptr };
    detail->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));

    if (back_buffer)
    {
        detail->device->CreateRenderTargetView(back_buffer, nullptr, &detail->render_target_view);
        back_buffer->Release();

        return true;
    }

    return false;
}

bool render_t::create_imgui()
{
    using namespace ImGui;
    CreateContext();
    StyleColorsDark();

    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

	io.Fonts->AddFontDefault();
	
	Visualize.visitor = io.Fonts->AddFontFromMemoryTTF((void*)rawData, sizeof(rawData), 9.0f);

	ImFontConfig weaponIconConfig;
	weaponIconConfig.OversampleH = 3;
	weaponIconConfig.OversampleV = 3;
	weaponIconConfig.FontDataOwnedByAtlas = false;
	Visualize.weapon_icon_font = io.Fonts->AddFontFromMemoryTTF((void*)cs_icon, sizeof(cs_icon), 12.0f, &weaponIconConfig);

	if (!ImGui_ImplWin32_Init(detail->window))
    {
        return false;
    }

    if (!detail->device || !detail->device_context)
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(detail->device, detail->device_context))
    {
        return false;
    }

    return true;
}

void render_t::destroy_device()
{
    if (detail->render_target_view) detail->render_target_view->Release();
    if (detail->swap_chain) detail->swap_chain->Release();
    if (detail->device_context) detail->device_context->Release();
    if (detail->device) detail->device->Release();
}

void render_t::destroy_window()
{
    DestroyWindow(detail->window);
    UnregisterClassA(detail->window_class.lpszClassName, detail->window_class.hInstance);
}

void render_t::destroy_imgui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void render_t::start_render()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static HMODULE user32 = GetModuleHandleA("user32.dll");
    static SetWindowDisplayAffinityProc SetWindowDisplayAffinity = nullptr;
    if (!SetWindowDisplayAffinity && user32)
    {
        SetWindowDisplayAffinity = (SetWindowDisplayAffinityProc)GetProcAddress(user32, "SetWindowDisplayAffinity");
    }

    if (SetWindowDisplayAffinity && detail->window)
    {
        if (menu::streamproof)
        {
            SetWindowDisplayAffinity(detail->window, WDA_EXCLUDEFROMCAPTURE);
        }
        else
        {
            SetWindowDisplayAffinity(detail->window, WDA_NONE);
        }
    }

    static HWND console_window = GetConsoleWindow();
    if (console_window)
    {
        if (menu::hide_console)
        {
            ShowWindow(console_window, SW_HIDE);
        }
        else
        {
            ShowWindow(console_window, SW_SHOW);
        }
    }

    // Check if Roblox is the foreground window
    static HWND roblox_window = nullptr;
    static bool last_visibility_state = true;
    HWND foreground_window = GetForegroundWindow();
    
    if (!roblox_window || !IsWindow(roblox_window))
    {
        roblox_window = FindWindowA(nullptr, "Roblox");
    }
    
    bool roblox_is_focused = false;
    bool overlay_is_focused = false;
    
    if (foreground_window && detail->window)
    {
        // Check if the overlay window itself is focused (user clicked on menu)
        overlay_is_focused = (foreground_window == detail->window || IsChild(detail->window, foreground_window));
        
        // Check if the foreground window is Roblox or a child of Roblox
        if (roblox_window)
        {
            roblox_is_focused = (foreground_window == roblox_window || IsChild(roblox_window, foreground_window));
        }
    }
    
    // Show overlay if Roblox is focused OR if the overlay itself is focused
    bool should_be_visible = roblox_is_focused || overlay_is_focused;
    
    // Only update visibility if state changed to prevent flickering
    if (should_be_visible != last_visibility_state && detail->window)
    {
        if (should_be_visible)
        {
            ShowWindow(detail->window, SW_SHOW);
        }
        else
        {
            ShowWindow(detail->window, SW_HIDE);
        }
        last_visibility_state = should_be_visible;
    }

    if (GetAsyncKeyState(menu::menu_keybind) & 1)
    {
        if (!check::textchatopen)
        {
            running = !running;

            if (running)
            {
                SetWindowLong(detail->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
            }
            else
            {
                SetWindowLong(detail->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED);
            }
        }
    }
}

void render_t::end_render()
{
    ImGui::Render();

    float clear_color[4]{ 0, 0, 0, 0 };
    detail->device_context->OMSetRenderTargets(1, &detail->render_target_view, nullptr);
    detail->device_context->ClearRenderTargetView(detail->render_target_view, clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    detail->swap_chain->Present(0, 0);
}

static void draw_custom_cursor()
{
	if (!g_silent_aim_instance.address)
	{
		return;
	}

	bool is_visible = false;
	is_visible = memory->read<bool>(g_silent_aim_instance.address + Offsets::GuiObject::Visible);

	if (!is_visible)
	{
		return;
	}

	POINT pt;
	if (!GetCursorPos(&pt))
	{
		return;
	}

	bool right_click_held = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
	float gap = right_click_held ? 4.0f : 10.0f;
	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	ImU32 col = IM_COL32(255, 255, 255, 255);
	float dot_size = 4.0f;
	float line_width = 2.0f;
	float line_length = 10.0f;
	ImVec2 center = { (float)pt.x, (float)pt.y };
	ImVec2 dot_min(center.x - dot_size * 0.5f, center.y - dot_size * 0.5f);
	ImVec2 dot_max(center.x + dot_size * 0.5f, center.y + dot_size * 0.5f);
	draw->AddRectFilled(dot_min, dot_max, col, 0.0f);
	ImVec2 top_min(center.x - line_width * 0.5f, center.y - gap - line_length);
	ImVec2 top_max(center.x + line_width * 0.5f, center.y - gap);
	draw->AddRectFilled(top_min, top_max, col, 0.0f);
	ImVec2 bottom_min(center.x - line_width * 0.5f, center.y + gap);
	ImVec2 bottom_max(center.x + line_width * 0.5f, center.y + gap + line_length);
	draw->AddRectFilled(bottom_min, bottom_max, col, 0.0f);
	ImVec2 left_min(center.x - gap - line_length, center.y - line_width * 0.5f);
	ImVec2 left_max(center.x - gap, center.y + line_width * 0.5f);
	draw->AddRectFilled(left_min, left_max, col, 0.0f);
	ImVec2 right_min(center.x + gap, center.y - line_width * 0.5f);
	ImVec2 right_max(center.x + gap + line_length, center.y + line_width * 0.5f);
	draw->AddRectFilled(right_min, right_max, col, 0.0f);
}

void render_t::render_menu()
{


    ImGui::SetNextWindowSize(ImVec2(600, 350));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    // main menu col, again. you can tweak all this and attach everything to a global
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.137f, 0.137f, 0.137f, 1.0f));

    ImGui::Begin("hello nigga!", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration);
    ImGui::PopStyleColor();

    // sum needed vars
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* foreground_dl = ImGui::GetForegroundDrawList();

    foreground_dl->Flags &= ImDrawListFlags_AntiAliasedLines;
    draw_list->Flags &= ImDrawListFlags_AntiAliasedLines;

    // inline of the main ui
    draw_list->AddRect(ImVec2(window_pos.x + 1, window_pos.y + 1), ImVec2(window_pos.x + window_size.x - 1, window_pos.y + window_size.y - 1), IM_COL32(60, 60, 60, 255), 0, 1.0f);
    // outline of the main ui
    draw_list->AddRect(ImVec2(window_pos.x + 2, window_pos.y + 2), ImVec2(window_pos.x + window_size.x - 2, window_pos.y + window_size.y - 2), IM_COL32(0, 0, 0, 255), 0, 1.0f);

    // a section for sections?
    draw_list->AddRectFilled(ImVec2(window_pos.x + 5, window_pos.y + 5), ImVec2(window_pos.x + window_size.x - 5, window_pos.y + window_size.y - 5), IM_COL32(20, 20, 20, 255));

    // outline of the main thing
    draw_list->AddRect(ImVec2(window_pos.x + 7, window_pos.y + 7), ImVec2(window_pos.x + window_size.x - 7, window_pos.y + window_size.y - 7), IM_COL32(0, 0, 0, 255));
    // inline of the thing 
    draw_list->AddRect(ImVec2(window_pos.x + 6, window_pos.y + 6), ImVec2(window_pos.x + window_size.x - 6, window_pos.y + window_size.y - 6), IM_COL32(60, 60, 60, 255));

    // section holder
    draw_list->AddRectFilled(ImVec2(window_pos.x + 14, window_pos.y + 71), ImVec2(window_pos.x + window_size.x - 14, window_pos.y + window_size.y - 14), IM_COL32(35, 35, 35, 255));

    // left inline
    draw_list->AddLine(ImVec2(window_pos.x + 15, window_pos.y + 72), ImVec2(window_pos.x + 15, window_pos.y + window_size.y - 15), IM_COL32(60, 60, 60, 255));
    // right inline
    draw_list->AddLine(ImVec2(window_pos.x + window_size.x - 15, window_pos.y + 72), ImVec2(window_pos.x + window_size.x - 15, window_pos.y + window_size.y - 15), IM_COL32(60, 60, 60, 255));
    // bottom inline
    draw_list->AddLine(ImVec2(window_pos.x + 15, window_pos.y + window_size.y - 15), ImVec2(window_pos.x + window_size.x - 15, window_pos.y + window_size.y - 15), IM_COL32(60, 60, 60, 255));

    // left outline
    draw_list->AddLine(ImVec2(window_pos.x + 14, window_pos.y + 71), ImVec2(window_pos.x + 14, window_pos.y + window_size.y - 14), IM_COL32(0, 0, 0, 255));
    // right outline
    draw_list->AddLine(ImVec2(window_pos.x + window_size.x - 14, window_pos.y + 71), ImVec2(window_pos.x + window_size.x - 14, window_pos.y + window_size.y - 14), IM_COL32(0, 0, 0, 255));
    // bottom outline
    draw_list->AddLine(ImVec2(window_pos.x + 14, window_pos.y + window_size.y - 14), ImVec2(window_pos.x + window_size.x - 14, window_pos.y + window_size.y - 14), IM_COL32(0, 0, 0, 255));
    // ok ts doen now ima do beginchild

    const char* charm_text = "ro";
    const char* wtfniggertext = "se";

    ImVec2 charm_size = ImGui::CalcTextSize(charm_text);
    ImVec2 wtf_size = ImGui::CalcTextSize(wtfniggertext);
    float total_width = charm_size.x + wtf_size.x;

    float window_width = ImGui::GetWindowWidth();
    float window_x = ImGui::GetWindowPos().x;
    float start_x = window_x + (window_width - total_width) * 0.5f;
    float start_y = ImGui::GetCursorScreenPos().y + 4.0f;


    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            draw_list->AddText(ImVec2(start_x + x * 1.0f, start_y + y * 1.0f), IM_COL32(0, 0, 0, 255.0f), charm_text);
            draw_list->AddText(ImVec2(start_x + charm_size.x + x * 1.0f, start_y + y * 1.0f), IM_COL32(0, 0, 0, 255.0f), wtfniggertext);
        }
    }

    // wada
    const char* items[] = { "Option 1", "Option 2", "Option 3", "Option 4" };
    static int current_item = 0;
    // wada

    draw_list->AddText(ImVec2(start_x, start_y), ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), charm_text);
    draw_list->AddText(ImVec2(start_x + charm_size.x, start_y), ImGui::ColorConvertFloat4ToU32(menu::accent_color), wtfniggertext);

    ImVec4 accent = menu::accent_color;
    draw_list->AddRectFilledMultiColor(ImVec2(window_pos.x + 20, window_pos.y + 30), ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + 31), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 0), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 255), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 255), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 0));
    draw_list->AddRectFilledMultiColor(ImVec2(window_pos.x + window_size.x * 0.5f, window_pos.y + 30), ImVec2(window_pos.x + window_size.x - 20, window_pos.y + 31), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 255), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 0), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 0), IM_COL32(accent.x * 255, accent.y * 255, accent.z * 255, 255));

    if (add_tab("Aimbot", 0, selected_tab_index == 0, 6))
        selected_tab_index = 0;
    if (add_tab("Silent", 1, selected_tab_index == 1, 6))
        selected_tab_index = 1;
    if (add_tab("Visuals", 2, selected_tab_index == 2, 6))
        selected_tab_index = 2;
    if (add_tab("Misc", 3, selected_tab_index == 3, 6))
        selected_tab_index = 3;
    if (add_tab("Settings", 4, selected_tab_index == 4, 6))
        selected_tab_index = 4;
    if (add_tab("Configs", 5, selected_tab_index == 5, 6))
        selected_tab_index = 5;

    // dont mind anything until this part, this part will be the place where your elements will go!
    switch (selected_tab_index)
    {
    case 0:
    {
        ImGui::SetCursorPos(ImVec2(22.f, 78.f));

        ImGui::BeginChild("Aimbot", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 9.f, ImGui::GetContentRegionAvail().y - 13.f), true);
        
        ImGui::Checkbox("Enable", &settings::aimbot::enabled);
        if (settings::aimbot::enabled)
        {
            ImGui::SameLine();
            inline_keybind_button("aimbot_keybind", &settings::aimbot::keybind, &settings::aimbot::keybind_mode);
        }
        
        ImGui::Checkbox("Sticky Aim", &settings::aimbot::sticky_aim);
        ImGui::Checkbox("Draw FOV", &settings::aimbot::draw_fov);
        ImGui::SameLine();
        if (add_tooltip_trigger("aimbot_fov_tooltip")) {
            if (begin_tooltip_popup("aimbot_fov_tooltip", ImVec2(290, 180))) {
                ImGui::BeginChild("Aimbot FOV Settings", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
                ImGui::SliderFloat("size", &settings::aimbot::fov, 1.0f, 1000.0f, "%.1f");
                
                ImGui::Checkbox("Fill", &settings::aimbot::filled_fov);
                ImGui::SameLine();
                ImGui::ColorEdit4("FOV Color", settings::aimbot::fov_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
                
                ImGui::Checkbox("Rotate", &settings::aimbot::rotate_fov);
                ImGui::Checkbox("Rainbow", &settings::aimbot::rainbow_fov);

                ImGui::EndChild();
                end_tooltip_popup("aimbot_fov_tooltip", ImVec2(290, 180));
            }
        }

        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(303.f, 78.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("Aimbot Settings", ImVec2(ImGui::GetContentRegionAvail().x - 13.f, ImGui::GetContentRegionAvail().y - 13.f), true);

        const char* aimbot_types[] = { "Camera", "Mouse Aim" };
        ImGui::Combo("Aimbot Type", &settings::aimbot::aimbot_type, aimbot_types, IM_ARRAYSIZE(aimbot_types));
        
        const char* aimparts[] = { 
            "Head", "Torso", "Closest Point"
        };
        ImGui::Combo("aimpart", &settings::aimbot::aimpart, aimparts, IM_ARRAYSIZE(aimparts));
        ImGui::Checkbox("FOV Check", &settings::aimbot::fov_check);
        ImGui::Checkbox("Knocked Check", &settings::aimbot::knocked_check);
        
        if (settings::aimbot::aimbot_type == 0) {
            ImGui::Checkbox("Camera Smooth", &settings::aimbot::camera_smooth);
            if (settings::aimbot::camera_smooth) {
                ImGui::SliderFloat("camera smooth x", &settings::aimbot::camera_smooth_x, 1.0f, 200.0f, "%.1f");
                ImGui::SliderFloat("camera smooth y", &settings::aimbot::camera_smooth_y, 1.0f, 200.0f, "%.1f");
            }
            ImGui::Checkbox("Camera Prediction", &settings::aimbot::camera_prediction);
            if (settings::aimbot::camera_prediction) {
                ImGui::SliderFloat("camera prediction x", &settings::aimbot::camera_prediction_x, 1.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("camera prediction y", &settings::aimbot::camera_prediction_y, 1.0f, 20.0f, "%.1f");
            }
        }

        if (settings::aimbot::aimbot_type == 1) {
            ImGui::Checkbox("Mouse Smooth", &settings::aimbot::mouse_smooth);
            if (settings::aimbot::mouse_smooth) {
                ImGui::SliderFloat("mouse smooth x", &settings::aimbot::mouse_smooth_x, 1.0f, 200.0f, "%.1f");
                ImGui::SliderFloat("mouse smooth y", &settings::aimbot::mouse_smooth_y, 1.0f, 200.0f, "%.1f");
                ImGui::SliderFloat("mouse sensitivity", &settings::aimbot::mouse_sensitivity, 0.1f, 10.0f, "%.1f");
            }
            ImGui::Checkbox("Mouse Prediction", &settings::aimbot::mouse_prediction);
            if (settings::aimbot::mouse_prediction) {
                ImGui::SliderFloat("mouse prediction x", &settings::aimbot::mouse_prediction_x, 1.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("mouse prediction y", &settings::aimbot::mouse_prediction_y, 1.0f, 20.0f, "%.1f");
            }
        }

        ImGui::Checkbox("Enable Shake", &settings::aimbot::shake);
        if (settings::aimbot::shake) {
            ImGui::SliderFloat("shake x", &settings::aimbot::shake_x, -5.0f, 5.0f, "%.1f");
            ImGui::SliderFloat("shake y", &settings::aimbot::shake_y, -5.0f, 5.0f, "%.1f");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        break;
    }
    case 1:
    {
        ImGui::SetCursorPos(ImVec2(22.f, 78.f));

        ImGui::BeginChild("Redirection", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 9.f, ImGui::GetContentRegionAvail().y - 13.f), true);
        
        
        ImGui::Checkbox("Enable", &settings::silent::enabled);
        if (settings::silent::enabled)
        {
            ImGui::SameLine();
            inline_keybind_button("silent_keybind", &settings::silent::keybind, &settings::silent::keybind_mode);
        }
        ImGui::Checkbox("Sticky Aim", &settings::silent::sticky_aim);
        ImGui::Checkbox("Spoof Mouse", &settings::silent::spoof_mouse);
        ImGui::Checkbox("Draw FOV", &settings::silent::draw_fov);
        ImGui::SameLine();
        if (add_tooltip_trigger("fov_tooltip")) {
            if (begin_tooltip_popup("fov_tooltip", ImVec2(290, 180))) {
                ImGui::BeginChild("Field Of View Settings", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);

                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.f);
                ImGui::SliderFloat("size", &settings::silent::fov, 10.f, 500.f, "%.1f");
                
                ImGui::Checkbox("Fill", &settings::silent::filled_fov);
                ImGui::SameLine();
                ImGui::ColorEdit4("Fov Color", settings::silent::fov_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
                
                ImGui::Checkbox("Rotate", &settings::silent::rotate_fov);
                ImGui::Checkbox("Rainbow", &settings::silent::rainbow_fov);

                ImGui::EndChild();
                end_tooltip_popup("fov_tooltip", ImVec2(290, 180));

            }
        }

        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(303.f, 78.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("Redirection Settings", ImVec2(ImGui::GetContentRegionAvail().x - 13.f, ImGui::GetContentRegionAvail().y - 13.f));

        const char* aim_parts[] = { "Head", "Torso", "Closest to Mouse" };
        ImGui::Combo("Aim Part", &settings::silent::aim_part, aim_parts, IM_ARRAYSIZE(aim_parts));

        ImGui::Checkbox("Gun Based FOV", &settings::silent::gun_based_fov);

        if (settings::silent::gun_based_fov)
        { 
            ImGui::SliderFloat("Double Barrel", &settings::silent::fov_double_barrel, 10.f, 500.f, "%.1f");
            ImGui::SliderFloat("Tactical Shotgun", &settings::silent::fov_tactical_shotgun, 10.f, 500.f, "%.1f");
            ImGui::SliderFloat("Revolver", &settings::silent::fov_revolver, 10.f, 500.f, "%.1f");
        }

        ImGui::Checkbox("Fov Check", &settings::silent::fov_check);
        ImGui::Checkbox("Knocked Check", &settings::silent::knocked_check);

        ImGui::EndChild();
        ImGui::PopStyleVar();
        break;
    }
    case 2:
        ImGui::SetCursorPos(ImVec2(22.f, 78.f));


        ImGui::BeginChild("WallHack", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 9.f, ImGui::GetContentRegionAvail().y - 13.f), true); // BRO I FORGHOT TO ADD TRUE AND I WAS TRYNA FIX THE PADDING ISSUE ALL ALONG OH MY FUICKING GODODODODOD

		ImGui::Checkbox("Boxes", &settings::visuals::box);
		ImGui::SameLine();
		ImGui::ColorEdit4("box col", settings::visuals::box_color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImGui::Checkbox("Name", &settings::visuals::name);
		ImGui::SameLine();
		ImGui::ColorEdit4("name col", settings::visuals::name_color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImGui::Checkbox("Distance", &settings::visuals::distance);
		ImGui::SameLine();
		ImGui::ColorEdit4("distance col", settings::visuals::distance_color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImGui::Checkbox("Tool", &settings::visuals::tool);
		ImGui::SameLine();
		ImGui::ColorEdit4("tool col", settings::visuals::tool_color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImGui::Checkbox("Weapon Icon", &settings::visuals::weapon_icon);
		ImGui::SameLine();
		ImGui::ColorEdit4("weapon icon col", settings::visuals::weapon_icon_color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
		ImGui::Checkbox("Highlight", &settings::visuals::highlights);
		ImGui::SameLine();
		ImGui::ColorEdit4("highlights col", settings::visuals::highlights_color, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

		ImGui::Checkbox("Healthbar", &settings::visuals::healthbar);
		ImGui::SameLine();
		if (add_tooltip_trigger("healthbar_tooltip")) {
			if (begin_tooltip_popup("healthbar_tooltip", ImVec2(250, 150))) {
				ImGui::BeginChild("Health Bar Settings", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);

				ImGui::Checkbox("Health Text", &settings::visuals::health_text);

				ImGui::Text("Health Bar Color");
				ImGui::SameLine();
				ImGui::ColorEdit3("healthbar color", settings::visuals::healthbar_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

				ImGui::Text("Health Text Color");
				ImGui::SameLine();
				ImGui::ColorEdit3("healthtext color", settings::visuals::health_text_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

				ImGui::EndChild();
				end_tooltip_popup("healthbar_tooltip", ImVec2(250, 150));
			}
		}

		ImGui::EndChild();

		ImGui::SetCursorPos(ImVec2(303.f, 78.f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::BeginChild("WallHack Misc", ImVec2(ImGui::GetContentRegionAvail().x - 13.f, ImGui::GetContentRegionAvail().y - 13.f), true);

		{
			const char* box_types[] = { "Normal", "Corner" };
			ImGui::Combo("Box Type", &settings::visuals::box_type, box_types, IM_ARRAYSIZE(box_types));
		}

		ImGui::Checkbox("Localplayer", &settings::visuals::localplayer);
		ImGui::Checkbox("Target", &settings::visuals::target);
		ImGui::Checkbox("Feature Indicator", &settings::visuals::feature_indicator);
		
		if (settings::visuals::feature_indicator)
		{
			ImGui::SliderFloat("indicator x", &settings::visuals::feature_indicator_x, 0.0f, 1920.0f, "%.0f");
			ImGui::SliderFloat("indicator y", &settings::visuals::feature_indicator_y, 0.0f, 1080.0f, "%.0f");
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
		break;
    case 3:
        ImGui::SetCursorPos(ImVec2(22.f, 78.f));

        ImGui::BeginChild("Movement", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 9.f, ImGui::GetContentRegionAvail().y - 13.f), true);

        ImGui::Checkbox("Walkspeed", &settings::expl::walkspeed);

        if (settings::expl::walkspeed)
        {
            ImGui::SliderFloat("Speed", &settings::expl::walkspeed_speed, 1.0f, 1000.0f, "%.1f");
            
            const char* walkspeed_modes[] = { "Normal", "Reloading", "Low Health" };
            ImGui::Combo("Conditions", &settings::expl::walkspeed_mode, walkspeed_modes, IM_ARRAYSIZE(walkspeed_modes));
            
            if (settings::expl::walkspeed_mode == 2)
            {
                ImGui::SliderFloat("", &settings::expl::walkspeed_health_threshold, 1.0f, 100.0f, "%.1f");
            }
        }

        ImGui::Spacing();
        ImGui::Checkbox("Freeze Players", &settings::expl::freeze_players);
        if (settings::expl::freeze_players)
        {
            ImGui::SameLine();
            inline_keybind_button("freeze_players_keybind", &settings::expl::freeze_players_keybind, &settings::expl::freeze_players_keybind_mode);
        }

        ImGui::Spacing();
        ImGui::Checkbox("Tickrate", &settings::expl::tickrate);
        if (settings::expl::tickrate)
        {
            ImGui::SliderFloat(" ", &settings::expl::tickrate_amount, 30.0f, 1000.0f, "%.1f");
        }


        ImGui::Spacing();
        ImGui::Checkbox("Fly", &settings::expl::fly_enabled);
        if (settings::expl::fly_enabled)
        {
            ImGui::SameLine();
            inline_keybind_button("fly_keybind", &settings::expl::fly_keybind, &settings::expl::fly_keybind_mode);
            ImGui::SliderFloat("Speed", &settings::expl::fly_speed, 1.0f, 1000.0f, "%.1f");
            const char* fly_modes[] = { "Velocity", "CFrame" };
            ImGui::Combo("Fly Mode", &settings::expl::fly_mode, fly_modes, IM_ARRAYSIZE(fly_modes));
        }

        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(303.f, 78.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("Blatant", ImVec2(ImGui::GetContentRegionAvail().x - 13.f, ImGui::GetContentRegionAvail().y - 13.f), true);

        ImGui::Checkbox("Hitbox Expander", &settings::hitbox_expander::enabled);

        if (settings::hitbox_expander::enabled)
        {
            ImGui::SliderFloat("Size X", &settings::hitbox_expander::size_x, 0.1f, 30.0f, "%.1f");
            ImGui::SliderFloat("Size Y", &settings::hitbox_expander::size_y, 0.1f, 30.0f, "%.1f");
            ImGui::SliderFloat("Size Z", &settings::hitbox_expander::size_z, 0.1f, 30.0f, "%.1f");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        break;
    case 4:
    {
        ImGui::SetCursorPos(ImVec2(22.f, 78.f));

        ImGui::BeginChild("User Interface", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 9.f, ImGui::GetContentRegionAvail().y - 13.f), true);

        ImGui::Text("Menu Keybind");
        ImGui::SameLine();
        ImVec2 menu_keybind_start = ImGui::GetCursorScreenPos();
        inline_keybind_button("menu_keybind", &menu::menu_keybind);
        ImVec2 menu_keybind_end = ImGui::GetCursorScreenPos();

        
        ImGui::Checkbox("Watermark", &menu::watermark);
        ImGui::Checkbox("Streamproof", &menu::streamproof);
        ImGui::Checkbox("Hide Console", &menu::hide_console);
        ImGui::Checkbox("Dex Explorer", &settings::dex_explorer::enabled);

        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(303.f, 78.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("User Interface Settings", ImVec2(ImGui::GetContentRegionAvail().x - 13.f, ImGui::GetContentRegionAvail().y - 13.f), true);

        ImGui::Text("Accent");
        ImGui::SameLine();
        ImGui::ColorEdit4("Accent", (float*)&menu::accent_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);

        ImGui::Spacing();
        ImGui::Spacing();
        
        if (styled_button("Unload", ImVec2(ImGui::GetContentRegionAvail().x - 13.f, 30.f)))
        {
            ExitProcess(0);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        break;
    }
    case 5:
    {
        static char config_name[64] = "";
        static int selected_config_index = -1;
        static std::vector<config::config_file_t> config_list;
        
        // Refresh config list
        static float refresh_timer = 0.0f;
        refresh_timer += ImGui::GetIO().DeltaTime;
        if (refresh_timer > 0.5f || config_list.empty())
        {
            config_list = config::get_config_files();
            refresh_timer = 0.0f;
        }

        ImGui::SetCursorPos(ImVec2(22.f, 78.f));

        ImGui::BeginChild("Configs List", ImVec2(ImGui::GetContentRegionAvail().x / 2 - 9.f, ImGui::GetContentRegionAvail().y - 13.f), true);
        
        ImGui::BeginChild("ConfigList", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 100), true);
        
        for (size_t i = 0; i < config_list.size(); i++)
        {
            bool is_selected = (selected_config_index == static_cast<int>(i));
            
            if (is_selected)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x - 1, ImGui::GetStyle().FramePadding.y));
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1);
            }
            
            if (ImGui::Selectable(config_list[i].name.c_str(), is_selected))
            {
                selected_config_index = static_cast<int>(i);
            }
            
            if (is_selected)
            {
                ImGui::PopStyleVar();
            }
        }
        
        ImGui::EndChild();
        
        ImGui::Spacing();
        ImGui::Text("Config Name:");
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1);
        ImGui::InputText("##config_name", config_name, sizeof(config_name));
        
        ImGui::Spacing();
        if (styled_button("Save", ImVec2(100, 0)))
        {
            if (strlen(config_name) > 0)
            {
                if (config::save_config(std::string(config_name)))
                {
                    config_list = config::get_config_files();
                    config_name[0] = '\0';
                }
            }
        }
        
        ImGui::SameLine();
        
        if (styled_button("Refresh", ImVec2(100, 0)))
        {
            config_list = config::get_config_files();
        }
        
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(303.f, 78.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::BeginChild("Config Actions", ImVec2(ImGui::GetContentRegionAvail().x - 13.f, ImGui::GetContentRegionAvail().y - 13.f), true);

        if (selected_config_index >= 0 && selected_config_index < static_cast<int>(config_list.size()))
        {
            ImGui::Text("Selected: %s", config_list[selected_config_index].name.c_str());
            ImGui::Spacing();
            
            if (styled_button("Load", ImVec2(100, 0)))
            {
                config::load_config(config_list[selected_config_index].name);
            }
            
            ImGui::SameLine();
            
            if (styled_button("Delete", ImVec2(100, 0)))
            {
                if (config::delete_config(config_list[selected_config_index].name))
                {
                    config_list = config::get_config_files();
                    selected_config_index = -1;
                }
            }
            
            ImGui::Spacing();
            
            if (styled_button("Location", ImVec2(100, 0)))
            {
                std::string folder = config::get_config_folder();
                if (!folder.empty())
                {
                    ShellExecuteA(NULL, "open", folder.c_str(), NULL, NULL, SW_SHOWDEFAULT);
                }
            }
        }
        else
        {
            ImGui::Text("No config selected");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        break;
    }
    }

    ImGui::End();
    ImGui::PopStyleVar();

    // Render dex explorer in separate window
    dex_explorer::DexExplorer::render();
}

void render_t::render_visuals()
{
    if (menu::watermark)
    {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;

        ImVec2 size = ImVec2(120, 28);
        ImVec2 display_size = ImGui::GetIO().DisplaySize;

        if (menu::watermark_pos.x < 0)
            menu::watermark_pos.x = (display_size.x - size.x) * 0.5f;

        static bool dragging = false;
        static ImVec2 drag_offset;

        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        bool mouse_down = ImGui::GetIO().MouseDown[0];

        ImVec2 pos = menu::watermark_pos;

        bool hovered = mouse_pos.x >= pos.x && mouse_pos.x <= pos.x + size.x &&
                       mouse_pos.y >= pos.y && mouse_pos.y <= pos.y + size.y;

        if (hovered && mouse_down && !dragging)
        {
            dragging = true;
            drag_offset = ImVec2(mouse_pos.x - pos.x, mouse_pos.y - pos.y);
        }

        if (dragging)
        {
            if (mouse_down)
            {
                menu::watermark_pos.x = mouse_pos.x - drag_offset.x;
                menu::watermark_pos.y = mouse_pos.y - drag_offset.y;
                pos = menu::watermark_pos;
            }
            else
            {
                dragging = false;
            }
        }

        draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(20, 20, 20, 255));
        draw->AddRect(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + size.x - 1, pos.y + size.y - 1), IM_COL32(60, 60, 60, 255));
        draw->AddRect(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pos.x + size.x - 2, pos.y + size.y - 2), IM_COL32(0, 0, 0, 255));

        ImU32 accent = ImGui::ColorConvertFloat4ToU32(menu::accent_color);
        draw->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pos.x + size.x - 2, pos.y + 4), accent);

        draw->AddRectFilled(ImVec2(pos.x + 5, pos.y + 7), ImVec2(pos.x + size.x - 5, pos.y + size.y - 5), IM_COL32(35, 35, 35, 255));
        draw->AddRect(ImVec2(pos.x + 5, pos.y + 7), ImVec2(pos.x + size.x - 5, pos.y + size.y - 5), IM_COL32(0, 0, 0, 255));
        draw->AddRect(ImVec2(pos.x + 6, pos.y + 8), ImVec2(pos.x + size.x - 6, pos.y + size.y - 6), IM_COL32(60, 60, 60, 255));

        ImVec2 text_size = ImGui::CalcTextSize("Rose");
        ImVec2 text_pos = ImVec2(pos.x + (size.x - text_size.x) * 0.5f, pos.y + 7 + ((size.y - 12) - text_size.y) * 0.5f);
        
        const char* text = "Rose";
        ImU32 outline_color = IM_COL32(0, 0, 0, 255);
        ImU32 text_color = IM_COL32(255, 255, 255, 255);
        
        draw->AddText(ImVec2(text_pos.x - 1, text_pos.y - 1), outline_color, text);
        draw->AddText(ImVec2(text_pos.x + 1, text_pos.y - 1), outline_color, text);
        draw->AddText(ImVec2(text_pos.x - 1, text_pos.y + 1), outline_color, text);
        draw->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), outline_color, text);
        draw->AddText(text_pos, text_color, text);
    }

    esp::run();
    
    render_feature_indicator();
    
    draw_custom_cursor();
}

void render_t::render_feature_indicator()
{
    if (!settings::visuals::feature_indicator) {
        return;
    }
    
    if (!Visualize.visitor) {
        return;
    }
    
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    
    float start_x = settings::visuals::feature_indicator_x;
    float start_y = settings::visuals::feature_indicator_y;
    
    if (start_y == 0.0f) {
        start_y = display_size.y * 0.5f;
    }
    
    float font_size = 9.0f;
    
    std::vector<std::string> lines;
    
    bool silent_enabled = settings::silent::enabled;
    bool silent_active = silent_enabled && g_silent_aim_locked;
    
    lines.push_back("SILENT AIM: " + std::string(silent_active ? "ON" : "OFF"));
    
    std::string target_name = "NONE";
    if (silent_active && g_silent_cached_target.instance.address != 0 && !g_silent_cached_target.name.empty()) {
        target_name = g_silent_cached_target.name;
    }
    lines.push_back("SILENT AIM TARGET: " + target_name);
    
    float text_y = start_y;
    ImU32 text_color = IM_COL32(255, 255, 255, 255);
    
    for (const auto& line : lines) {
        ImVec2 text_size = Visualize.visitor->CalcTextSizeA(font_size, FLT_MAX, 0.0f, line.c_str());
        float text_x = start_x;
        
        Visualize.DrawTextWithSpacingAndOutline(draw, Visualize.visitor, font_size, ImVec2(text_x, text_y), text_color, IM_COL32(0, 0, 0, 255), line);
        
        text_y += text_size.y + 2.0f;
    }
}

void render_t::render_notifications()
{
    notifications::update();
    notifications::render();
}