#include "c_prediction_system.h"
#include "../utils/c_memory.h"
#include "../utils/math.h"
#include "../sdk/c_global_vars_base.h"
#include "../sdk/c_client_entity_list.h"
#include "../sdk/c_move_helper.h"
#include "../sdk/c_prediction.h"
#include "../sdk/c_cvar.h"
#include "../sdk/c_client_state.h"
#include "../sdk/c_input.h"
#include "c_animation_system.h"
#include "../sdk/c_weapon_system.h"

c_prediction_system::c_prediction_system()
{

	m_prediction_random_seed = *reinterpret_cast<uint32_t**>(reinterpret_cast<uint32_t>(
		sig("client.dll", "A3 ? ? ? ? 66 0F 6E 86")) + 1);
	m_prediction_player = *reinterpret_cast<c_cs_player***>(reinterpret_cast<uint32_t>(
		sig("client.dll", "89 35 ? ? ? ? F3 0F 10 48")) + 2);
	_md5_pseudo_random = reinterpret_cast<md5_pseudo_random>(sig("client.dll", "55 8B EC 83 E4 F8 83 EC 70 6A"));
}



void c_prediction_system::start(c_cs_player* player, c_user_cmd* cmd) {
	if (!player)
		return;

	m_backup.m_curtime = global_vars_base->curtime;
	m_backup.m_frametime = global_vars_base->frametime;

	m_backup.m_in_prediction = prediction()->m_in_prediction;
	m_backup.m_first_time_predicted = prediction()->m_first_time_predicted;

	global_vars_base->curtime = ticks_to_time(player->get_tick_base());
	global_vars_base->frametime = prediction()->m_engine_paused ? 0.f : global_vars_base->interval_per_tick;

	prediction()->m_in_prediction = true;
	prediction()->m_first_time_predicted = false;

	player->get_cur_cmd() = cmd;
	*reinterpret_cast<c_user_cmd**>(reinterpret_cast<uintptr_t>(player) + 0x3288) = cmd;

	static const auto md5_pseudo_random_fn = SIG("client.dll", "55 8B EC 83 E4 F8 83 EC 70 6A").cast<uint32_t(__thiscall*)(uint32_t)>();

	*m_prediction_player = player;
	*m_prediction_random_seed = md5_pseudo_random_fn(cmd->command_number) & 0x7FFFFFFF;

	auto buttons_forced = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(player) + 0x3334);
	auto buttons_disabled = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(player) + 0x3330);

	cmd->buttons |= buttons_forced;
	cmd->buttons &= ~buttons_disabled;

	move_helper->set_host(player);
	game_movement()->start_track_prediction_errors(player);

	if (cmd->weapon_select) {
		auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity(cmd->weapon_select));

		if (weapon) {
	
			 auto weapon_data = weapon_system->get_weapon_data(weapon->get_item_definition());
	

			weapon_data ? player->select_item(weapon_data->szWeaponName, cmd->weapon_select_subtype) : 0;
		}
	}

	//auto vehicle_handle = player->get_vehicle();
	//auto vehicle = vehicle_handle.is_valid() ? reinterpret_cast<c_base_entity*>(vehicle_handle.get()) : nullptr;

	if (cmd->impulse)
		//&& (!vehicle || player->using_standard_weapons_in_vehicle()))
		player->get_impulse() = cmd->impulse;

	auto buttons = cmd->buttons;
	auto buttons_changed = buttons ^ player->get_buttons();

	player->get_buttons_last() = player->get_buttons();
	player->get_buttons() = buttons;
	player->get_buttons_pressed() = buttons_changed & buttons;
	player->get_buttons_released() = buttons_changed & ~buttons;

	prediction()->check_moving_on_ground(player, global_vars_base->frametime);

	player->set_local_view_angles(cmd->viewangles);

	player->physics_run_think(0) ? player->pre_think() : 0;

	if (player->get_next_think_tick()
		&& player->get_next_think_tick() != -1
		&& player->get_next_think_tick() <= player->get_tick_base()) {
		player->get_next_think_tick() = -1;
		player->unknown_think(0);
		player->think();
	}

	prediction()->setup_move(player, cmd, move_helper, &m_move_data);

//	vehicle
//		? get_vfun_c<void(__thiscall*)(c_base_entity*, c_cs_player*, c_move_data*)>(vehicle, 5)(vehicle, player, reinterpret_cast<c_move_data*>(m_move_data))
	game_movement()->process_movement(player, &m_move_data);

	prediction()->finish_move(player, cmd, &m_move_data);

	move_helper->process_impacts();

	post_think(player);

	prediction()->m_in_prediction = m_backup.m_in_prediction;
	prediction()->m_first_time_predicted = m_backup.m_first_time_predicted;
}

void c_prediction_system::end(c_cs_player* player, c_user_cmd* cmd) {
	if (!player)
		return;

	game_movement()->finish_track_prediction_errors(player);
    move_helper->set_host2(nullptr);
	game_movement()->reset();

	player->get_cur_cmd() = nullptr;
	*m_prediction_random_seed = -1;
	*m_prediction_player = 0;

	!prediction()->m_engine_paused && global_vars_base->frametime ? player->get_tick_base()++ : 0;

	global_vars_base->curtime =   m_backup.m_curtime;
	global_vars_base->frametime = m_backup.m_frametime;
}