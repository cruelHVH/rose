#define NOMINMAX
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <Windows.h>
#include <cmath>
#include <cstring>
#include <algorithm>

#include "../resources/GetWeaponIcon.h"
#include "esp.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>
#include <game/game.h>
#include <cache/cache.h>
#include <features/silent/silent.h>
#include <features/aimbot/aimbot.h>

static float get_effective_fov()
{
	if (!settings::silent::gun_based_fov)
		return settings::silent::fov;

	std::string tool_name;
	{
		std::lock_guard<std::mutex> lock(cache::mtx);
		tool_name = cache::cached_local_player.tool_name;
	}

	std::string tool_name_lower = tool_name;
	std::transform(tool_name_lower.begin(), tool_name_lower.end(), tool_name_lower.begin(), ::tolower);

	if (tool_name_lower.find("double-barrel") != std::string::npos || 
		tool_name_lower.find("double barrel") != std::string::npos ||
		tool_name_lower.find("doublebarrel") != std::string::npos)
		return settings::silent::fov_double_barrel;
	else if (tool_name_lower.find("tacticalshotgun") != std::string::npos ||
		tool_name_lower.find("tactical shotgun") != std::string::npos)
		return settings::silent::fov_tactical_shotgun;
	else if (tool_name_lower.find("revolver") != std::string::npos)
		return settings::silent::fov_revolver;

	return settings::silent::fov;
}

#include <clipper2/clipper.h>

#define M_PI 3.14159265358979323846

namespace helper
{
	__forceinline void corner_box(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 col, float thickness = 1.f)
	{
		float x1 = std::round(min.x) - 1;
		float y1 = std::round(min.y) - 1;
		float x2 = std::round(max.x) + 1;
		float y2 = std::round(max.y) + 1;

		ImU32 outline_col = IM_COL32(0, 0, 0, 255);

		float box_width = x2 - x1;
		float box_height = y2 - y1;
		float length = std::min(box_width * 0.3f, box_height * 0.3f);
		length = std::max(length, 5.f);
		length = std::min(length, 15.f);

		float x1_len = std::round(x1 + length);
		float y1_len = std::round(y1 + length);
		float x2_len = std::round(x2 - length);
		float y2_len = std::round(y2 - length);

		draw->AddRectFilled(ImVec2(x1 - 1.f, y1 - 1.f), ImVec2(x1_len + 1.f, y1 + thickness + 1.f), outline_col);
		draw->AddRectFilled(ImVec2(x1 - 1.f, y1 - 1.f), ImVec2(x1 + thickness + 1.f, y1_len + 1.f), outline_col);

		draw->AddRectFilled(ImVec2(x2_len - 1.f, y1 - 1.f), ImVec2(x2 + 1.f, y1 + thickness + 1.f), outline_col);
		draw->AddRectFilled(ImVec2(x2 - thickness - 1.f, y1 - 1.f), ImVec2(x2 + 1.f, y1_len + 1.f), outline_col);

		draw->AddRectFilled(ImVec2(x1 - 1.f, y2 - thickness - 1.f), ImVec2(x1_len + 1.f, y2 + 1.f), outline_col);
		draw->AddRectFilled(ImVec2(x1 - 1.f, y2_len - 1.f), ImVec2(x1 + thickness + 1.f, y2 + 1.f), outline_col);

		draw->AddRectFilled(ImVec2(x2_len - 1.f, y2 - thickness - 1.f), ImVec2(x2 + 1.f, y2 + 1.f), outline_col);
		draw->AddRectFilled(ImVec2(x2 - thickness - 1.f, y2_len - 1.f), ImVec2(x2 + 1.f, y2 + 1.f), outline_col);

		draw->AddRectFilled(ImVec2(x1, y1), ImVec2(x1_len, y1 + thickness), col);
		draw->AddRectFilled(ImVec2(x1, y1), ImVec2(x1 + thickness, y1_len), col);

		draw->AddRectFilled(ImVec2(x2_len, y1), ImVec2(x2, y1 + thickness), col);
		draw->AddRectFilled(ImVec2(x2 - thickness, y1), ImVec2(x2, y1_len), col);

		draw->AddRectFilled(ImVec2(x1, y2 - thickness), ImVec2(x1_len, y2), col);
		draw->AddRectFilled(ImVec2(x1, y2_len), ImVec2(x1 + thickness, y2), col);

		draw->AddRectFilled(ImVec2(x2_len, y2 - thickness), ImVec2(x2, y2), col);
		draw->AddRectFilled(ImVec2(x2 - thickness, y2_len), ImVec2(x2, y2), col);
	}

	__forceinline void box(ImVec2& c1, ImVec2& c2, ImU32 color)
	{
		c1.x = std::round(c1.x);
		c1.y = std::round(c1.y);
		c2.x = std::round(c2.x);
		c2.y = std::round(c2.y);

		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		draw->Flags &= ImDrawListFlags_AntiAliasedLines;

		if (settings::visuals::box_type == 1) // corner box
		{
			ImVec2 min = c1;
			ImVec2 max = ImVec2(c1.x + c2.x, c1.y + c2.y);
			corner_box(draw, min, max, color, 1.f);
		}
		else // normal box
		{
			ImRect rect(c1.x, c1.y, c1.x + c2.x, c1.y + c2.y);
			ImVec2 shadow = { cosf(0.f) * 2.f, sinf(0.f) * 2.f };

			draw->AddRect(rect.Min, rect.Max, IM_COL32(0, 0, 0, color >> 24));
			draw->AddRect({ rect.Min.x - 1.f, rect.Min.y - 1.f }, { rect.Max.x + 1.f, rect.Max.y + 1.f }, color);
			draw->AddRect({ rect.Min.x - 2.f, rect.Min.y - 2.f }, { rect.Max.x + 2.f, rect.Max.y + 2.f }, IM_COL32(0, 0, 0, color >> 24));
		}
	}
}

void DrawPolygonalFOV(ImDrawList* draw, ImVec2 center, float radius, ImU32 color, bool filled = false, float rotation = 0.0f) 
{
	const int segments = 12;
	const float angle_step = 2.0f * M_PI / segments;

	ImVec2 vertices[segments + 1];
	for (int i = 0; i < segments; i++) 
	{
		float angle = i * angle_step + rotation;
		vertices[i] = ImVec2(
			center.x + radius * cosf(angle),
			center.y + radius * sinf(angle)
		);
	}
	vertices[segments] = vertices[0];

	if (filled)
	{
		draw->AddConvexPolyFilled(vertices, segments, color);
	}

	ImU32 outline_color = (color & 0x00FFFFFF) | 0xFF000000;
	for (int i = 0; i < segments; i++) 
	{
		draw->AddLine(vertices[i], vertices[i + 1], IM_COL32(0, 0, 0, 255), 4.0f);
		draw->AddLine(vertices[i], vertices[i + 1], outline_color, 2.0f);
	}
}

void esp::run()
{
	static math::vector3 corners[8] =
	{
		{-1, -1, -1}, {1, -1, -1}, {-1, 1, -1},{1, 1, -1},
		{-1, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 1}
	};

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	draw->Flags |= ImDrawListFlags_AntiAliasedLines;

	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	ScreenToClient(FindWindowA(nullptr, "Roblox"), &cursor_pos);

	math::vector2 dims = game::visengine.get_dimensions();
	math::matrix4 view = game::visengine.get_viewmatrix();

	if (settings::silent::draw_fov)
	{
		ImVec2 center(cursor_pos.x, cursor_pos.y);
		
		float rotation = 0.0f;
		if (settings::silent::rotate_fov)
		{
			static float rotation_time = 0.0f;
			rotation_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rotation_time > 2.0f * M_PI)
				rotation_time -= 2.0f * M_PI;
			rotation = rotation_time;
		}
		
		ImVec4 color_vec;
		if (settings::silent::rainbow_fov)
		{
			static float rainbow_time = 0.0f;
			rainbow_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rainbow_time > 2.0f * M_PI)
				rainbow_time -= 2.0f * M_PI;
			
			float h = rainbow_time / (2.0f * M_PI);
			float s = 1.0f;
			float v = 1.0f;
			
			int i = (int)(h * 6.0f);
			float f = (h * 6.0f) - i;
			float p = v * (1.0f - s);
			float q = v * (1.0f - s * f);
			float t = v * (1.0f - s * (1.0f - f));
			
			i %= 6;
			switch (i)
			{
			case 0: color_vec = ImVec4(v, t, p, settings::silent::fov_color[3]); break;
			case 1: color_vec = ImVec4(q, v, p, settings::silent::fov_color[3]); break;
			case 2: color_vec = ImVec4(p, v, t, settings::silent::fov_color[3]); break;
			case 3: color_vec = ImVec4(p, q, v, settings::silent::fov_color[3]); break;
			case 4: color_vec = ImVec4(t, p, v, settings::silent::fov_color[3]); break;
			case 5: color_vec = ImVec4(v, p, q, settings::silent::fov_color[3]); break;
			}
		}
		else
		{
			color_vec = ImVec4(
				settings::silent::fov_color[0],
				settings::silent::fov_color[1],
				settings::silent::fov_color[2],
				settings::silent::fov_color[3]
			);
		}
		
		ImU32 fov_color = ImGui::ColorConvertFloat4ToU32(color_vec);
		DrawPolygonalFOV(draw, center, get_effective_fov() - 1.0f, fov_color, settings::silent::filled_fov, rotation);
	}

	if (settings::aimbot::draw_fov)
	{
		ImVec2 center(cursor_pos.x, cursor_pos.y);
		
		float rotation = 0.0f;
		if (settings::aimbot::rotate_fov)
		{
			static float rotation_time = 0.0f;
			rotation_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rotation_time > 2.0f * M_PI)
				rotation_time -= 2.0f * M_PI;
			rotation = rotation_time;
		}
		
		ImVec4 color_vec;
		if (settings::aimbot::rainbow_fov)
		{
			static float rainbow_time = 0.0f;
			rainbow_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rainbow_time > 2.0f * M_PI)
				rainbow_time -= 2.0f * M_PI;
			
			float h = rainbow_time / (2.0f * M_PI);
			float s = 1.0f;
			float v = 1.0f;
			
			int i = (int)(h * 6.0f);
			float f = (h * 6.0f) - i;
			float p = v * (1.0f - s);
			float q = v * (1.0f - s * f);
			float t = v * (1.0f - s * (1.0f - f));
			
			i %= 6;
			switch (i)
			{
			case 0: color_vec = ImVec4(v, t, p, settings::aimbot::fov_color[3]); break;
			case 1: color_vec = ImVec4(q, v, p, settings::aimbot::fov_color[3]); break;
			case 2: color_vec = ImVec4(p, v, t, settings::aimbot::fov_color[3]); break;
			case 3: color_vec = ImVec4(p, q, v, settings::aimbot::fov_color[3]); break;
			case 4: color_vec = ImVec4(t, p, v, settings::aimbot::fov_color[3]); break;
			case 5: color_vec = ImVec4(v, p, q, settings::aimbot::fov_color[3]); break;
			}
		}
		else
		{
			color_vec = ImVec4(
				settings::aimbot::fov_color[0],
				settings::aimbot::fov_color[1],
				settings::aimbot::fov_color[2],
				settings::aimbot::fov_color[3]
			);
		}
		
		ImU32 fov_color = ImGui::ColorConvertFloat4ToU32(color_vec);
		DrawPolygonalFOV(draw, center, settings::aimbot::fov - 1.0f, fov_color, settings::aimbot::filled_fov, rotation);
	}

	std::vector<cache::entity_t> snapshot;
	{
		std::lock_guard<std::mutex> lock(cache::mtx);
		snapshot = cache::cached_players;
	}

	for (cache::entity_t& entity : snapshot)
	{
		bool valid = false;
		float left = FLT_MAX, top = FLT_MAX;
		float right = -FLT_MAX, bottom = -FLT_MAX;

		if (entity.instance.address == 0)
		{
			continue;
		}

		if (!settings::visuals::localplayer && entity.instance.address == game::local_player.address)
		{
			continue;
		}

		if (settings::visuals::target && g_silent_aim_locked)
		{
			if (entity.instance.address != g_silent_cached_target.instance.address)
			{
				continue;
			}
		}

		for (auto& parts : entity.parts)
		{
			rbx::primitive_t prim = parts.second.get_primitive();
			auto size = prim.get_size();
			
			if (settings::hitbox_expander::enabled && parts.first == "HumanoidRootPart")
			{
				size = math::vector3{ 2.0f, 2.0f, 1.0f };
			}
			
			auto pos = prim.get_position();
			auto rot = prim.get_rotation();

			if (size.x == 0 || size.y == 0 || size.z == 0)
			{
				continue;
			}

			for (auto& corner : corners)
			{
				math::vector3 world = pos + rot * math::vector3
				{
					corner.x * size.x * 0.5f,
					corner.y * size.y * 0.5f,
					corner.z * size.z * 0.5f
				};

				math::vector2 out{};
				if (game::visengine.world_to_screen(world, out, dims, view))
				{
					valid = true;
					left = std::min(left, out.x);
					top = std::min(top, out.y);
					right = std::max(right, out.x);
					bottom = std::max(bottom, out.y);
				}
			}
		}

		if (!valid || left >= right || top >= bottom)
		{
			continue;
		}

		ImVec2 c1(left, top);
		ImVec2 c2(right - left, bottom - top);

		if (settings::visuals::box)
		{
			helper::box(c1, c2, ImGui::ColorConvertFloat4ToU32
			(
				{
					settings::visuals::box_color[0],
					settings::visuals::box_color[1],
					settings::visuals::box_color[2],
					settings::visuals::box_color[3]
				}
			));
		}

		if (settings::visuals::highlights)
		{
			ImDrawList* draw = ImGui::GetBackgroundDrawList();
			draw->Flags &= ImDrawListFlags_AntiAliasedLines;

			auto project_part = [&](rbx::part_t& part, const std::string& part_name) -> std::vector<ImVec2> {
				std::vector<ImVec2> projected;
				if (!part.address) return projected;

				rbx::primitive_t prim = part.get_primitive();
				math::vector3 size = prim.get_size();
				
				if (settings::hitbox_expander::enabled && part_name == "HumanoidRootPart")
				{
					size = math::vector3{ 2.0f, 2.0f, 1.0f };
				}
				
				math::vector3 pos = prim.get_position();
				math::matrix3 rot = prim.get_rotation();

				for (const auto& lc : corners) {
					math::vector3 world = pos + rot * math::vector3{ lc.x * size.x * 0.5f, lc.y * size.y * 0.5f, lc.z * size.z * 0.5f };
					math::vector2 screen{};
					if (game::visengine.world_to_screen(world, screen, dims, view)) {
						if (screen.x >= 0.f && screen.y >= 0.f)
							projected.push_back(ImVec2(screen.x, screen.y));
					}
				}

				if (projected.size() < 3) return {};

				std::sort(projected.begin(), projected.end(), [](const ImVec2& a, const ImVec2& b) {
					return a.x < b.x || (a.x == b.x && a.y < b.y);
					});

				std::vector<ImVec2> hull;
				auto cross = [](const ImVec2& O, const ImVec2& A, const ImVec2& B) {
					return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
					};

				for (auto& p : projected) {
					while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
						hull.pop_back();
					hull.push_back(p);
				}

				size_t t = hull.size() + 1;
				for (int i = (int)projected.size() - 1; i >= 0; --i) {
					auto& p = projected[i];
					while (hull.size() >= t && cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
						hull.pop_back();
					hull.push_back(p);
				}

				hull.pop_back();
				return hull;
				};

			Clipper2Lib::Paths64 all_parts;
			for (auto& part_pair : entity.parts) {
				if (part_pair.first == "HumanoidRootPart") continue;

				rbx::part_t part = part_pair.second;
				if (!part.address) continue;
				auto hull = project_part(part, part_pair.first);
				if (hull.size() < 3) continue;

				Clipper2Lib::Path64 path;
				for (auto& pt : hull)
					path.push_back({ static_cast<int64_t>(pt.x * 1000.0), static_cast<int64_t>(pt.y * 1000.0) });
				all_parts.push_back(path);
			}

			if (all_parts.empty()) continue;

			auto unified_solution = Clipper2Lib::Union(all_parts, Clipper2Lib::FillRule::NonZero);

			std::vector<std::vector<ImVec2>> all_polys;
			for (auto& sp : unified_solution) {
				std::vector<ImVec2> poly;
				for (auto& pt : sp) poly.push_back(ImVec2(pt.x / 1000.0f, pt.y / 1000.0f));
				if (poly.size() >= 3) all_polys.push_back(poly);
			}

			ImU32 fill_color = ImGui::ColorConvertFloat4ToU32({
				settings::visuals::highlights_color[0],
				settings::visuals::highlights_color[1],
				settings::visuals::highlights_color[2],
				settings::visuals::highlights_color[3]
			});

			for (auto& poly : all_polys) {
				draw->AddConcavePolyFilled(poly.data(), poly.size(), fill_color);
			}

			for (auto& poly : all_polys) {
				draw->AddPolyline(poly.data(), poly.size(), IM_COL32_WHITE, true, 1.0f);
			}
		}

		ImVec2 boxPos = c1;
		ImVec2 boxBR = ImVec2(c1.x + c2.x, c1.y + c2.y);

		if (settings::visuals::name && Visualize.visitor)
		{
			std::string player_name = entity.name;
			
			ImFont* font = Visualize.visitor;
			float font_size = 9.0f;
			float char_spacing = 1.0f;
			
			float total_width = 0.0f;
			float max_height = 0.0f;
			for (size_t i = 0; i < player_name.length(); i++) {
				char c = player_name[i];
				char char_str[2] = { c, '\0' };
				ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
				total_width += char_size.x + (i < player_name.length() - 1 ? char_spacing : 0.0f);
				max_height = std::max(max_height, char_size.y);
			}
			
			float centerX = (boxPos.x + boxBR.x) * 0.5f;
			float textX = centerX - (total_width * 0.5f);
			float textY = boxPos.y - max_height - 2.0f;
			
			ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
			
			ImU32 nameColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
				settings::visuals::name_color[0],
				settings::visuals::name_color[1],
				settings::visuals::name_color[2],
				settings::visuals::name_color[3]
			));
			
			Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, nameColor, IM_COL32(0, 0, 0, 255), player_name, char_spacing);
		}

		if (settings::visuals::distance && Visualize.visitor)
		{
			auto hrp_it = entity.parts.find("HumanoidRootPart");
			if (hrp_it != entity.parts.end() && game::local_character.address != 0)
			{
				rbx::part_t local_hrp = { game::local_character.find_first_child("HumanoidRootPart").address };
				math::vector3 local_pos = local_hrp.get_primitive().get_position();
				math::vector3 enemy_pos = hrp_it->second.get_primitive().get_position();
				
				float dx = enemy_pos.x - local_pos.x;
				float dy = enemy_pos.y - local_pos.y;
				float dz = enemy_pos.z - local_pos.z;
				float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
				
				char distanceStr[32];
				snprintf(distanceStr, sizeof(distanceStr), "[%.1fM]", dist);
				
				ImFont* font = Visualize.visitor;
				float font_size = 9.0f;
				float char_spacing = 1.0f;
				
				float total_width = 0.0f;
				float max_height = 0.0f;
				for (size_t i = 0; i < strlen(distanceStr); i++) {
					char c = distanceStr[i];
					char char_str[2] = { c, '\0' };
					ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
					total_width += char_size.x + (i < strlen(distanceStr) - 1 ? char_spacing : 0.0f);
					max_height = std::max(max_height, char_size.y);
				}
				
				float centerX = (boxPos.x + boxBR.x) * 0.5f;
				float textX = centerX - (total_width * 0.5f);
				float textY = boxBR.y + 2.0f;
				
				ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
				
				ImU32 distColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
					settings::visuals::distance_color[0],
					settings::visuals::distance_color[1],
					settings::visuals::distance_color[2],
					settings::visuals::distance_color[3]
				));
				
				Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, distColor, IM_COL32(0, 0, 0, 255), std::string(distanceStr), char_spacing);
			}
		}

		if (settings::visuals::tool && Visualize.visitor)
		{
			std::string toolName = entity.tool_name;
			
			if (!toolName.empty())
			{
				
				ImFont* font = Visualize.visitor;
				float font_size = 9.0f;
				float char_spacing = 1.0f;
				
				float total_width = 0.0f;
				float max_height = 0.0f;
				for (size_t i = 0; i < toolName.length(); i++) {
					char c = toolName[i];
					char char_str[2] = { c, '\0' };
					ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
					total_width += char_size.x + (i < toolName.length() - 1 ? char_spacing : 0.0f);
					max_height = std::max(max_height, char_size.y);
				}
				
				float centerX = (boxPos.x + boxBR.x) * 0.5f;
				float textX = centerX - (total_width * 0.5f);
				float textY = boxBR.y + 2.0f;
				
				if (settings::visuals::distance)
					textY += 8.0f;
				
				ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
				
				ImU32 toolColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
					settings::visuals::tool_color[0],
					settings::visuals::tool_color[1],
					settings::visuals::tool_color[2],
					settings::visuals::tool_color[3]
				));
				
				Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, toolColor, IM_COL32(0, 0, 0, 255), toolName, char_spacing);
			}
		}

		if (settings::visuals::weapon_icon && Visualize.weapon_icon_font)
		{
			std::string toolName = entity.tool_name;
			
			if (!toolName.empty())
			{
				const char* weaponIcon = GetWeaponIcon(toolName);
				
				if (weaponIcon && strlen(weaponIcon) > 0)
				{
					float iconFontSize = 12.0f;
					ImVec2 iconTextSize = Visualize.weapon_icon_font->CalcTextSizeA(iconFontSize, FLT_MAX, 0.0f, weaponIcon);
					
					float centerX = (boxPos.x + boxBR.x) * 0.5f;
					float iconX = centerX - (iconTextSize.x * 0.5f);
					float iconY = boxBR.y + 2.0f;
					
					if (settings::visuals::distance)
						iconY += 8.0f;
					if (settings::visuals::tool)
						iconY += 8.0f;
					
					ImVec2 iconPos = ImVec2(std::floor(iconX + 0.5f), std::floor(iconY + 0.5f));
					
					ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
						settings::visuals::weapon_icon_color[0],
						settings::visuals::weapon_icon_color[1],
						settings::visuals::weapon_icon_color[2],
						settings::visuals::weapon_icon_color[3]
					));
					
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x + 1, iconPos.y + 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x - 1, iconPos.y + 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x + 1, iconPos.y - 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x - 1, iconPos.y - 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, iconPos, iconColor, weaponIcon);
				}
			}
		}

		if (settings::visuals::healthbar)
		{
			float health = 0.0f;
			float max_health = 100.0f;
			
			if (entity.humanoid.address != 0)
			{
				health = entity.humanoid.get_health();
				max_health = entity.humanoid.get_max_health();
			}

			if (max_health > 0.0f)
			{
				constexpr float healthBarWidth = 1.0f;
				float boxHeight = c2.y;

				ImVec2 barPos = ImVec2(boxPos.x - 5.0f, boxPos.y - 1.0f);
				ImVec2 barSize = ImVec2(healthBarWidth, boxHeight + 2.0f);

				Visualize.draw_health_bar(draw, max_health, health, barPos, barSize, 1.0f, settings::visuals::health_text);
			}
		}

	}

}

void esp_render_t::draw_health_bar(ImDrawList* draw, float max, float current, ImVec2 pos, ImVec2 size, float alpha_factor, bool show_text)
{
	if (max <= 0.0f) max = 100.0f;
	
	float clamped_current = std::clamp(current, 0.0f, max);
	float health_percent = (max > 0.0f) ? (clamped_current / max) : 0.0f;
	health_percent = std::clamp(health_percent, 0.0f, 1.0f);
	
	float bar_width = 1.0f;
	float bar_height = size.y;
	float bar_x = std::round(pos.x);
	float bar_y = std::round(pos.y);

	ImDrawListFlags old_flags = draw->Flags;
	draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;

	float outline_x1 = std::round(bar_x - 1);
	float outline_y1 = std::round(bar_y - 1);
	float outline_x2 = std::round(bar_x + bar_width + 1);
	float outline_y2 = std::round(bar_y + bar_height + 1);
	
	draw->AddRectFilled(
		ImVec2(outline_x1, outline_y1),
		ImVec2(outline_x2, outline_y2),
		IM_COL32(0, 0, 0, 255)
	);

	float fill_height = bar_height * health_percent;
	
	ImU32 health_color = IM_COL32(
		static_cast<int>(settings::visuals::healthbar_color[0] * 255.f),  
		static_cast<int>(settings::visuals::healthbar_color[1] * 255.f),          
		static_cast<int>(settings::visuals::healthbar_color[2] * 255.f),
		static_cast<int>(255 * alpha_factor)
	);

	float bg_x1 = std::round(bar_x);
	float bg_y1 = std::round(bar_y);
	float bg_x2 = std::round(bar_x + bar_width);
	float bg_y2 = std::round(bar_y + bar_height);
	
	draw->AddRectFilled(
		ImVec2(bg_x1, bg_y1),
		ImVec2(bg_x2, bg_y2),
		IM_COL32(50, 50, 50, 255)
	);
	
	if (fill_height > 0.1f) {
		float fill_start_y = bar_y + bar_height - fill_height;
		float fill_y1 = std::round(fill_start_y);
		float fill_y2 = std::round(bar_y + bar_height);
		
		draw->AddRectFilled(
			ImVec2(bg_x1, fill_y1),
			ImVec2(bg_x2, fill_y2),
			health_color
		);
	}
	
	draw->Flags = old_flags;

	if (show_text && settings::visuals::health_text && health_percent < 1.0f && visitor)
	{
		char buffer[32];
		sprintf_s(buffer, "%.0f", current);

		ImFont* font = visitor;
		float font_size = 9.0f;
		ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, buffer);

		float text_center_y = bar_y + (bar_height * 0.5f);
		
		ImVec2 text_pos = ImVec2(
			std::round(bar_x - (text_size.x / 2.0f) + (bar_width / 2.0f)),
			std::round(text_center_y - (text_size.y / 2.0f))
		);

		ImU32 text_color = IM_COL32(
			static_cast<int>(settings::visuals::health_text_color[0] * 255.f),
			static_cast<int>(settings::visuals::health_text_color[1] * 255.f),
			static_cast<int>(settings::visuals::health_text_color[2] * 255.f),
			255
		);

		draw->AddText(font, font_size, ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 255), buffer);
		draw->AddText(font, font_size, ImVec2(text_pos.x - 1, text_pos.y + 1), IM_COL32(0, 0, 0, 255), buffer);
		draw->AddText(font, font_size, ImVec2(text_pos.x + 1, text_pos.y - 1), IM_COL32(0, 0, 0, 255), buffer);
		draw->AddText(font, font_size, ImVec2(text_pos.x - 1, text_pos.y - 1), IM_COL32(0, 0, 0, 255), buffer);
		
		draw->AddText(font, font_size, text_pos, text_color, buffer);
	}
}
