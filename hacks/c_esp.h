#pragma once
#include "../includes.h"
#include "../hooks/idirect3ddevice9.h"
#include <mutex>
#include "../sdk/c_debug_overlay.h"
#include "./../menu/c_menu.h"

#include "../baim.h"
namespace fonts {
	extern DWORD ESP;
	extern DWORD health_font;
	extern DWORD weaponnamefont;
	extern DWORD weapon_font;
	extern DWORD flagfont;
	extern DWORD arialfont;
} 

namespace log_esp {
	struct esp_log_infos {
		char text[16]{};
	};
	extern std::vector<esp_log_infos> esp_log_info;
};

namespace snipe_scope {
	extern c_vector3d pos;
	extern int hitdamage;
};

namespace mouse {
	extern c_vector2d main_mouse;
	extern c_vector2d last_mouse;
	extern c_vector2d i_drag;
	extern c_vector2d s_drag;
	extern c_vector2d d_drag;
	extern c_vector2d k_drag;
};

class c_esp
{
public:
	static void fsn_nightmode();
	static void clear_dmg();
	static void draw_dmg();
	static void adddmg(c_game_event* event);
	static void draw_penetration_crosshair();
	static void draw_molotov_timer();
	static void draw_smoke_timer();
	static void draw_taser_esp();
	static void draw_spread_crosshair();
	static void esp_keybinds();
	static void sniper_scope();
	static bool get_espbox(c_cs_player* entity, int& x, int& y, int& w, int& h);
	static void esp_radar(c_cs_player* player);
	static void esp_skeleton(c_cs_player* player, Menu22::visaul::esp cfg);
	static void bomb_timer(c_cs_player* entity);
	static void esp_history_skeleton(c_cs_player* player, Menu22::visaul::esp cfg);
	static void esp_nades(c_cs_player* nade);
	static void render_player();
	static void indicator();
	static void render_panel();
	static void draw_spectators();
	static void draw_progresstext(c_vector2d& position, const char* name, const float progress);
	static void draw_progressbar(c_vector2d& position, const char* name, const c_color color, const float progress);
	static void draw_progressbar(c_vector2d& position, const char* name, const c_color color, const float progress, DWORD font);
	static void indicator_ini();
	static void draw_enemy_impact(c_game_event* event);
	static void draw_trails(c_cs_player* local);
	static void draw_firstperson_indicators();
	static void draw_thirdperson_indicators();

	static bool world_to_screen(c_vector3d world, c_vector3d& screen)
	{
		return (debug_overlay()->ScreenPosition(world, screen) != 1);
	}
	
	/*c_vector3d world_to_screen(c_vector3d world_pos) {
		c_vector3d tmp;
		debug_overlay()->world_to_screen(world_pos, tmp);
		const matrix3x4& w2s_matrix = engine_client()->get_world_to_screen_matrix();
		int w = w2s_matrix[3][3];
		for (int i{}; i < 3; i++) {
			w += w2s_matrix[3][i] * world_pos[i];
		}
		if (w < 0.001f)
			return{ 10000.f, 10000.f };

		return{ tmp.x, tmp.y };
	}*/



	static bool screen_transform(const c_vector3d& in, c_vector3d& out)
	{
		const auto& w2_s_matrix = engine_client()->get_world_to_screen_matrix();
		out.x = w2_s_matrix[0][0] * in[0] + w2_s_matrix[0][1] * in[1] + w2_s_matrix[0][2] * in[2] + w2_s_matrix[0][3];
		out.y = w2_s_matrix[1][0] * in[0] + w2_s_matrix[1][1] * in[1] + w2_s_matrix[1][2] * in[2] + w2_s_matrix[1][3];
		out.z = 0.0f;

		const auto w = w2_s_matrix[3][0] * in.x + w2_s_matrix[3][1] * in.y + w2_s_matrix[3][2] * in.z + w2_s_matrix[3][3];

		if (w < 0.001f)
		{
			out.x *= 100000;
			out.y *= 100000;
			return false;
		}

		const auto invw = 1.0f / w;
		out.x *= invw;
		out.y *= invw;

		return true;
	}

	/*static bool world_to_screen(const c_vector3d& in, c_vector3d& out)
	{
		const auto result = screen_transform(in, out);

		int w, h;
		engine_client()->get_screen_size(w, h);

		out.x = (w / 2.0f) + (out.x * w) / 2.0f;
		out.y = (h / 2.0f) - (out.y * h) / 2.0f;

		return result;
	}*/

	static bool should_run_dmg;
private:
	static constexpr auto esp_flags = c_font::font_flags::centered_x | c_font::font_flags::centered_y | c_font::font_flags::drop_shadow;
	static constexpr auto collision_box_top = 72.f;
	static constexpr auto collision_box_mod = 18.f;
	static constexpr auto head_radius = 6.5f;
	static constexpr auto fade_frequency =  255 / 1.f;

	static void esp_no_scope();
	static void render_watermark();
	static void draw_impact(c_vector3d start, c_vector3d end, c_color color);
};