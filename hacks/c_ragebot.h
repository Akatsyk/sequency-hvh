#pragma once

#include "../includes.h"
#include "c_animation_system.h"
#include "c_aimhelper.h"

class c_ragebot
{
	using aim_info = c_aimhelper::aim_info;

	struct autostop_info
	{
		float call_time;
		bool did_stop;
	};

public:
	static bool IsFireTime(c_user_cmd* cmd, const float time, const bool manual_shot);
	static void no_recoil(c_cs_player* local, c_user_cmd* cmd);
	static void aim(c_cs_player* local, c_user_cmd* cmd, bool& send_packet, c_base_combat_weapon* weapon);
	static bool IsTickValid(float simtime);
	static void autostop(c_cs_player* local, c_user_cmd* cmd);

	static std::optional<aim_info> scan_record_gun(c_cs_player* local, c_animation_system::animation* animation, c_base_combat_weapon* weapon, std::optional<Menu22::ragebot::weapon_config> weapon_cfg,
		std::optional<c_vector3d> pos = std::nullopt);

	static std::optional<aim_info> scan_record_lag_peek(c_cs_player* local, c_animation_system::animation* animation,
		std::optional<c_vector3d> pos = std::nullopt);

	static std::optional<aim_info> simple_backtrack_record_gun(c_cs_player* local, c_animation_system::animation* animation, std::optional<c_vector3d> pos = std::nullopt);

	static autostop_info& get_autostop_info();

	inline static std::optional<float> last_pitch = std::nullopt;
private:
	static std::optional<aim_info> scan_record(c_cs_player* local, c_animation_system::animation* animation);
	static std::optional<aim_info> scan_record_knife(c_cs_player* local, c_animation_system::animation* animation);

	static bool is_breaking_lagcomp(c_animation_system::animation* animation);
};