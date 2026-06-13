#pragma once
#include "../ext/imgui/imgui.h"
	#include <Windows.h>

namespace menu
{
	inline ImVec4 accent_color = ImVec4(255.f / 255.0f, 0.f / 255.0f, 0.f / 255.0f, 1.0f);
	inline bool watermark{ false };
	inline ImVec2 watermark_pos = ImVec2(-1, 10);
	inline bool streamproof{ false };
	inline bool hide_console{ true };
	inline int menu_keybind{ VK_INSERT };
}

namespace settings
{
	namespace aimbot
	{
		inline bool enabled{ false };
		inline int aimbot_type{ 0 };
		inline bool sticky_aim{ false };
		inline bool draw_fov{ false };
		inline bool filled_fov{ false };
		inline bool rotate_fov{ false };
		inline bool rainbow_fov{ false };
		inline float fov{ 200.0f };
		inline float fov_color[4]{ 1.f, 1.f, 1.f, 0.5f };
		inline bool fov_check{ true };
		inline bool knocked_check{ false };
		inline int keybind{ 0 };
		inline int keybind_mode{ 0 };
		inline int aimpart{ 0 };
		
		inline bool mouse_smooth{ false };
		inline float mouse_smooth_x{ 1.0f };
		inline float mouse_smooth_y{ 1.0f };
		inline float mouse_sensitivity{ 1.0f };
		inline bool mouse_prediction{ false };
		inline float mouse_prediction_x{ 1.0f };
		inline float mouse_prediction_y{ 1.0f };
		
		inline bool camera_smooth{ false };
		inline float camera_smooth_x{ 1.0f };
		inline float camera_smooth_y{ 1.0f };
		inline bool camera_prediction{ false };
		inline float camera_prediction_x{ 1.0f };
		inline float camera_prediction_y{ 1.0f };
		
		inline bool shake{ false };
		inline float shake_x{ 0.0f };
		inline float shake_y{ 0.0f };
	}
	namespace silent
	{
		inline bool enabled{ false };
		inline bool sticky_aim{ false };
		inline bool spoof_mouse{ true };
		inline bool draw_fov{ false };
		inline bool filled_fov{ false };
		inline bool rotate_fov{ false };
		inline bool rainbow_fov{ false };
		inline float fov{ 200.0f };
		inline int keybind{ 0 };
		inline int keybind_mode{ 0 }; // 0 = hold, 1 = toggle
		inline int aim_part{ 0 };
		inline bool fov_check{ true };
		inline bool knocked_check{ false };
		inline bool gun_based_fov{ false };
		inline float fov_double_barrel{ 200.0f };
		inline float fov_tactical_shotgun{ 200.0f };
		inline float fov_revolver{ 200.0f };
		inline float fov_color[4]{ 1.f, 1.f, 1.f, 0.5f };
	}
	namespace visuals
	{
		inline bool box{ false };
		inline int box_type{ 0 }; // 0 = normal, 1 = corner
		inline float box_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool name{ false };
		inline float name_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool distance{ false };
		inline float distance_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool tool{ false };
		inline float tool_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool weapon_icon{ false };
		inline float weapon_icon_color[4]{ 1.f, 1.f, 1.f, 1.f };

		inline bool localplayer{ false };

		inline bool highlights{ false };
		inline float highlights_color[4]{ 1.f, 1.f, 1.f, 0.4f };

		inline bool healthbar{ false };
		inline float healthbar_color[3]{ 0.f, 1.f, 0.f };
		inline bool health_text{ false };
		inline float health_text_color[3]{ 1.f, 1.f, 1.f };
		inline bool feature_indicator{ false };
		inline float feature_indicator_x{ 50.0f };
		inline float feature_indicator_y{ 0.0f };
		inline bool target{ false };
	}

	namespace expl
	{
		inline bool walkspeed{ false };
		inline float walkspeed_speed{ 16.0f };
		inline int walkspeed_mode{ 0 };
		inline float walkspeed_health_threshold{ 50.0f };
		inline int walkspeed_keybind{ 0 };
		
		inline bool freeze_players{ false };
		inline int freeze_players_keybind{ 0 };
		inline int freeze_players_keybind_mode{ 0 };

		inline bool tickrate{ false };
		inline float tickrate_amount{ 0 };
		
		inline bool fly_enabled{ false };
		inline float fly_speed{ 50.0f };
		inline int fly_mode{ 0 }; // 0 = velocity, 1 = cframe
		inline int fly_keybind{ 0 };
		inline int fly_keybind_mode{ 0 };
	}

	namespace hitbox_expander
	{
		inline bool enabled{ false };
		inline float size_x{ 2.2f };
		inline float size_y{ 2.2f };
		inline float size_z{ 1.2f };
	}

	namespace dex_explorer
	{
		inline bool enabled{ false };
	}
}