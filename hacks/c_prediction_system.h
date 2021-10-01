#pragma once
#include "../utils/c_singleton.h"
#include "../sdk/c_cs_player.h"
#include <cstdint>
#include "../sdk/c_game_movement.h"
#include "../sdk/c_prediction.h"

template<typename T>
__forceinline T get_vfun_c(void* base, int index) { return (*static_cast<T**>(base))[index]; }

class c_prediction_system : public c_singleton<c_prediction_system>
{
	using md5_pseudo_random = uint32_t(__thiscall*)(uint32_t);

	struct prediction_info
	{
		c_user_cmd* cmd{};
		float curtime{};
		c_move_data move_data{};
		int32_t flags{};
		c_base_handle ground_entity{};
	};

public:
	c_prediction_system();
		void start(c_cs_player* player, c_user_cmd* cmd);

	 void end(c_cs_player* player, c_user_cmd* cmd);

	struct {
		float m_curtime, m_frametime;
		bool m_in_prediction, m_first_time_predicted;
	} m_backup;

	c_move_data m_move_data;


	uint32_t* m_prediction_random_seed;
	c_cs_player** m_prediction_player;
	md5_pseudo_random _md5_pseudo_random;


	/*prediction_info animation_info[150]{};
	c_user_cmd original_cmd{};
	int32_t unpredicted_flags{};
	c_vector3d unpredicted_velocity{};

	c_move_data move_data{}, unpredicted_move_data{};*/

private:

	int post_think(c_cs_player* player) {
		get_vfun_c<void(__thiscall*)(void*)>(model_cache(), 33)(model_cache());

		if (player->is_alive()
			|| *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(player) + 0x3A81)) {
			get_vfun_c<void(__thiscall*)(void*)>(player, 339)(player);

			player->get_flags()& c_base_player::flags::on_ground ? player->get_fall_velocity() = 0.f : 0;

			player->get_sequence() == -1 ? get_vfun_c<void(__thiscall*)(void*, int)>(player, 218)(player, 0) : 0;

			get_vfun_c<void(__thiscall*)(void*)>(player, 219)(player);

			static const auto post_think_v_physics_fn = SIG("client.dll", "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 8B D9 56 57 83 BB").cast<bool(__thiscall*)(void*)>();
			post_think_v_physics_fn(player);
		}

		static const auto simulate_player_simulated_entities_fn = SIG("client.dll", "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 72").cast<bool(__thiscall*)(void*)>();
		simulate_player_simulated_entities_fn(player);

		return get_vfun_c<int(__thiscall*)(void*)>(model_cache(), 34)(model_cache());
	}
/*	uint32_t* prediction_random_seed;
	c_cs_player** prediction_player;
	md5_pseudo_random _md5_pseudo_random;

	c_user_cmd* last_cmd{};
	int32_t tick_base{}, seq_diff{};

	float backup_curtime{}, backup_frametime{};
	int backup_tickbase{};
	c_vector3d backup_origin{};
	c_qangle backup_aim_punch{}, backup_aim_punch_velocity{}, backup_view_offset{};
	float backup_accuracy_penalty{}, backup_recoil_index{}, backup_duck_amount{};*/

};

#define prediction_system c_prediction_system::instance()
