#pragma once

#include "c_animation_system.h"
#include "../sdk/c_game_event_manager.h"

namespace resolver_shot
{
	struct shot
	{
		c_animation_system::animation record{};
		c_vector3d start{}, end{};
		uint32_t hitgroup{};
		c_cs_player::hitbox hitbox{};
		float time{}, damage{};
		bool confirmed{}, impacted{}, skip{}, manual{}, delayed{}, sideways{}, uppitch{}, shotting{}, playerhurt{};

		struct server_info_t
		{
			std::vector<c_vector3d> impacts{};
			uint32_t hitgroup{}, damage{}, index{};
		} server_info;
	};
}

class c_resolver
{
public:
	bool has_target;

	c_qangle last_aim_angle;
	c_qangle last_eye_pos;
	matrix3x4 last_bones[128];
	c_cs_player::hitbox last_hitbox;

	int last_shot_missed_index;
	int missed_due_to_spread[65];
	int missed_due_to_bad_resolve[65];
	int resolver_method[65];
	int base_weight[65];
	int base_prev_weight[65];
	int cycle_difference[65];

	static void on_player_hurt(c_game_event* event);
	static float GetCurtime2222();
	static void on_bullet_impact(c_game_event* event);
	static void on_weapon_fire(c_game_event* event);
	static void on_round_start(c_game_event* event);
	static void on_player_death(c_game_event* event);
	static void on_render_start();
	static void register_shot(resolver_shot::shot&& s);
	

	static bool get_target_freestand_yaw(c_cs_player* target, float* yaw);
	static bool get_enemy_freestand_yaw(c_cs_player* target, float* yaw);

	static void AntiFreestanding();

	static void resolve(c_animation_system::animation* record);

	static float get_max_desync_delta(c_cs_player* player);

private:
	static void resolve_shot(resolver_shot::shot& shot);
	inline static std::deque<resolver_shot::shot> shots = {};

}; extern c_resolver resolver;