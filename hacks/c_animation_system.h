#pragma once

#include <deque>
#include <utility>
#include <mutex>
#include "../sdk/c_cs_player.h"
#include "../sdk/c_client_state.h"
#include "../sdk/c_global_vars_base.h"
#include "../sdk/c_cvar.h"

enum resolver_state {
	resolver_start,
	resolver_brute_left,
	resolver_brute_right
};
template< typename t = float >
t c_maxes(const t& a, const t& b) {
	// check type.
	static_assert(std::is_arithmetic< t >::value, "math::max only supports integral types.");
	return (t)_mm_cvtss_f32(
		_mm_max_ss(_mm_set_ss((float)a),
			_mm_set_ss((float)b))
	);
}

__forceinline float calculate_lerp()
{
	static auto cl_ud_rate =  cvar()->find_var("cl_updaterate");
	static auto min_ud_rate = cvar()->find_var("sv_minupdaterate");
	static auto max_ud_rate = cvar()->find_var("sv_maxupdaterate");

	int ud_rate = 64;

	if (cl_ud_rate)
		ud_rate = cl_ud_rate->get_int();

	if (min_ud_rate && max_ud_rate)
		ud_rate = max_ud_rate->get_int();

	float ratio = 1.f;
	static auto cl_interp_ratio = cvar()->find_var("cl_interp_ratio");

	if (cl_interp_ratio)
		ratio = cl_interp_ratio->get_float();

	static auto cl_interp =   cvar()->find_var("cl_interp");
	static auto c_min_ratio = cvar()->find_var("sv_client_min_interp_ratio");
	static auto c_max_ratio = cvar()->find_var("sv_client_max_interp_ratio");

	float lerp = global_vars_base->interval_per_tick;

	if (cl_interp)
		lerp = cl_interp->get_float();

	if (c_min_ratio && c_max_ratio && c_min_ratio->get_float() != 1)
		ratio = std::clamp(ratio, c_min_ratio->get_float(), c_max_ratio->get_float());

	return c_maxes(lerp, ratio / ud_rate);
}

class c_animation_system : public c_singleton<c_animation_system>
{
public:
	struct animation
	{
		animation() = default;
		explicit animation(c_cs_player* player);
		explicit animation(c_cs_player* player, c_qangle last_reliable_angle);
		void restore(c_cs_player* player) const;
		void apply(c_cs_player* player) const;
		void build_server_bones(c_cs_player* player);

		bool is_valid(float range = .2f, float max_unlag = .2f);

		/*
		__forceinline bool is_valid(const float range = .2f, const float max_unlag = .2f) const
		{
			if (!net_channel || !valid)
				return false;

			const auto correct = std::clamp(net_channel->get_latency(flow_incoming)
				+ net_channel->get_latency(flow_outgoing)
				+ calculate_lerp(), 0.f, max_unlag);

			return fabsf(correct - (global_vars_base->curtime - sim_time)) <= range;
		}*/



		c_cs_player* player{};
		int32_t index{};

		bool valid{}, has_anim_state{};
		alignas(16) matrix3x4 bones[128]{};

		bool dormant{}, is_backtrackable{};
		c_vector3d velocity;
		c_vector3d abs_velocity;
		c_vector3d origin;
		c_vector3d abs_origin;
		c_vector3d obb_mins;
		c_vector3d obb_maxs;
		c_base_animating::animation_layers layers{};
		c_base_animating::pose_paramater poses{};
		c_csgo_player_anim_state anim_state{};
		float anim_time{};
		float sim_time{};
		float interp_time{};
		float duck{};
		float lby{};
		float last_shot_time{};
		c_qangle last_reliable_angle{};
		c_qangle eye_angles = c_qangle(0, 0, 0);
		c_qangle abs_ang = c_qangle(0, 0, 0);
		int flags{};
		int eflags{};
		int effects{};
		int lag{};
		int shot{};
		int health{};
	};
private:
	struct animation_info {
		animation_info(c_cs_player* player, std::deque<animation> animations)
			: player(player), frames(std::move(animations)), last_spawn_time(0) { }

		void update_animations(animation* to, animation* from);

		c_cs_player* player{};
		std::deque<animation> frames{};

		// latest animation (might be invalid)
		animation latest_animation{};

		// last time this player spawned
		float last_spawn_time;

		// counter of how many shots we missed
		int32_t missed_due_to_spread{};

		// resolver data
		resolver_state brute_state{};
		float brute_yaw{};
		c_vector3d last_reliable_angle{};
	};

	std::unordered_map<c_base_handle, animation_info> animation_infos;

public:
	void update_player(c_cs_player* player);
	void post_player_update();

	animation_info* get_animation_info(c_cs_player* player);
	std::optional<animation*> get_latest_animation(c_cs_player* player);
	std::optional<animation*> get_oldest_animation(c_cs_player* player);
	std::optional<animation*> get_intermediate_anim(c_cs_player* player);
	std::optional<animation*> get_uncrouched_animation(c_cs_player* player);
	std::optional<std::pair<animation*, animation*>> get_intermediate_animations(c_cs_player* player, float range = 1.f);
	std::vector<animation*> get_valid_animations(c_cs_player* player, float range = 1.f);
	std::optional<animation*> get_lastest_animation_unsafe(c_cs_player* player);

	std::optional<animation*> get_custom_backtrack(c_cs_player* player, float time = 0.05);

	animation local_animation;
	bool in_jump{}, enable_bones{};
};

#define animation_system c_animation_system::instance()
#define lerp_ticks time_to_ticks(calculate_lerp())