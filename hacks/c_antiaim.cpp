#include "c_antiaim.h"
#include "../utils/math.h"
#include "../sdk/c_game_rules.h"
#include "../sdk/c_client_state.h"
#include "../sdk/c_debug_overlay.h"
#include "../sdk/c_input.h"
#include "../sdk/c_prediction.h"
#include "../sdk/c_weapon_system.h"
#include "../menu/c_menu.h"
#include "c_aimhelper.h"
#include "c_miscellaneous.h"
#include "c_ragebot.h"
#include "c_trace_system.h"
#include "c_prediction_system.h"
#include "../hooks/c_events.h"
#include <random>
#include "c_animation_system.h"
#include "../baim.h"

static std::random_device rd;
static std::mt19937 rng(rd());

namespace antiaim_globals {
	float to_choke = 0;
	float max_delta = 0;
	bool init_antaims = false;
	matrix3x4 fake_matrix[128] = { matrix3x4() };
	matrix3x4 real_matrix[128] = { matrix3x4() };
}

int choking_amt = 0;

void c_antiaim::fakelag(c_cs_player* local, c_user_cmd* cmd, bool& send_packet)
{
	//auto anim_state = local->get_anim_state();

	static constexpr auto target_standing = 1;
	static constexpr auto target_moving = 8;
	static constexpr auto target_air = 6;
	static constexpr auto target_slowwalk = 6;

	auto target_standing_custom = config->Ragebot.stand_fakelag;
	auto target_moving_custom = config->Ragebot.move_fakelag;
	auto target_air_custom = config->Ragebot.air_fakelag;
	auto target_slowwalk_custom = config->Ragebot.variance;

	static auto last_origin = c_vector3d();
	static auto last_simtime = 0.f;
	static auto last_ducked = false;
	static auto fakeland = false;
	static auto fakeair = false;
	static auto should_send_packet = false;
	static auto can_fake_land = false;

	static auto onpeek_timer = 0.f;
	static auto unduck_timer = 0.f;

	static auto onpeek = false;

	auto choke_amount = target_standing_custom;
	if (!local || !local->is_alive() || !cmd || game_rules->is_freeze_period() || !config->Ragebot.fakelag_enable)
	{
		choke_amount = 0;
		estimated_choke = 0;
		choking_amt = 0;
	//	unit::bsendpacket = true;
		return;
	}

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon)
	{
		choke_amount = 0;
		estimated_choke = 0;
		choking_amt = 0;
	//	unit::bsendpacket = true;
		return;
	}

	const auto info = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!info)
	{
		choke_amount = 0;
		estimated_choke = 0;
		choking_amt = 0;
	//	unit::bsendpacket = true;
		return;
	}

	const auto max_choke_amount = game_rules->is_valve_ds() ? 9u : 15u;

	/*if (GetKeyState(config->Ragebot._auto.dt_key))
	{
		choke_amount = 0;
		estimated_choke = 0;
		choking_amt = 0;
	//	unit::bsendpacket = true;
		return;
	}*/

	if (local->get_velocity().length2d() > info->get_standing_accuracy(weapon) || local->get_duck_amount() > 0.f)
		choke_amount = target_moving_custom;

	if (GetAsyncKeyState(config->Misc.slowwalk_key) && local->is_on_ground())
	{
		choke_amount =  target_slowwalk_custom;
	}

	if (!local->is_on_ground() && GetAsyncKeyState(config->Misc.slowwalk_key))
		choke_amount = target_air_custom;

	if (weapon->is_grenade())
		choke_amount = 1;

	if ((weapon->get_item_definition() == weapon_taser))
		choke_amount = 1;

	if ((weapon->get_item_definition() == weapon_knife || weapon->get_item_definition() == weapon_knife_t || weapon->get_item_definition() == weapon_taser))
		choke_amount = 1;

	// are we in on peek?
	if (onpeek_timer > global_vars_base->curtime + ticks_to_time(16))
	{
		onpeek_timer = 0.f;
		onpeek = false;
		is_on_peek = false;
	}

	// are we in unduck?
	if (unduck_timer > global_vars_base->curtime + ticks_to_time(16))
		unduck_timer = 0.f;

	// extend fake lag to maximum.
	if (onpeek_timer >= global_vars_base->curtime || unduck_timer >= global_vars_base->curtime)
		choke_amount = max_choke_amount + 1;

	// force enemy to extrapolate us.
	if (last_simtime == global_vars_base->curtime - global_vars_base->interval_per_tick && (local->get_origin() - last_origin).length2dsqr() > 4096.f)
		choke_amount = 1;

	auto missing_target = false;

	if (on_peek(local, missing_target))
	{
		if (!onpeek && onpeek_timer < global_vars_base->curtime)
		{
			onpeek_timer = global_vars_base->curtime + ticks_to_time(16);

			if (client_state->choked_commands > 1)
				choke_amount = 1;
		}

		onpeek = true;
		is_on_peek = true;

	}
	else if (missing_target)
	{
		onpeek = false;
		is_on_peek = false;
	}

	if (local->get_flags() & c_base_player::flags::on_ground && !(cmd->buttons & c_user_cmd::flags::duck) && last_ducked)
	{
		if (unduck_timer < global_vars_base->curtime)
		{
			unduck_timer = global_vars_base->curtime + ticks_to_time(16);

			if (client_state->choked_commands > 1)
			{
				cmd->buttons |= c_user_cmd::flags::duck;
				choke_amount = 1;
			}
		}
	}

	if (unduck_timer > global_vars_base->curtime - ticks_to_time(1) && !client_state->choked_commands)
		cmd->buttons |= c_user_cmd::flags::duck;

	// fake land to desync animations for pasted cheats.
	auto origin =   local->get_origin();
	auto velocity = local->get_velocity();
	auto flags =    local->get_flags();

	auto backup_origin = origin;

	c_trace_system::extrapolate(local, origin, velocity, flags, c_cs_player::get_local_player()->get_flags() & c_base_player::on_ground);

/*	if (flags & c_base_player::flags::on_ground && !(local->get_flags() & c_base_player::flags::on_ground))
	{
		if (can_fake_land)
		{
			can_fake_land = false;
			fakeland = false;
		}

		fakeland = fabsf(math::normalize_yaw(local->get_lby() - animation_system->local_animation.eye_angles.y)) > 35.f;
		choke_amount = fakeland ? max_choke_amount : 0;
	}
	else if (local->get_flags() & c_base_player::flags::on_ground && fakeland) // we just landed, so we choke this time...
	{
		can_fake_land = true;
		choke_amount = max_choke_amount;
	}
	else if (can_fake_land && !(local->get_flags() & c_base_player::flags::on_ground) && !(flags & c_base_player::flags::on_ground))
	{
		choke_amount = 1;
		can_fake_land = false;
		fakeland = false;
	}

	// unchoke on max air memez done by Solpadoin //
	if (backup_origin.z > origin.z && should_send_packet)
	{
		choke_amount = max_choke_amount; // meme , default is 1
		should_send_packet = false;
	}

	// fake air *roflan ebalo* zeus king by Solpadoin
	if (!(flags & c_base_player::flags::on_ground) && (local->get_flags() & c_base_player::flags::on_ground))
	{
		choke_amount = 1;
		fakeair = true;
		should_send_packet = true;
	}
	else if (!(local->get_flags() & c_base_player::flags::on_ground) && fakeair)
	{
		fakeair = false;
		choke_amount = max_choke_amount;
	}
	// */

	if ((global_vars_base->curtime + global_vars_base->interval_per_tick > next_lby_update || lby_update >= global_vars_base->curtime - global_vars_base->interval_per_tick) && local->get_velocity().length2d() < 0.1f)
		choke_amount = max_choke_amount;

	//c_menu::HandleInput(config.rage.fake_duck, config.rage.fake_type)
	if (GetAsyncKeyState(config->Ragebot.fakeduck))
	{
		antiaim->is_fakeducking = true;
		choke_amount = 14;
	}
	else
		antiaim->is_fakeducking = false;

	if (!antiaim->is_fakeducking && weapon->get_item_definition() == weapon_revolver)
		choke_amount = 1;

	if (!antiaim->is_fakeducking && config->Ragebot.fl_disable_shot && cmd->buttons & c_user_cmd::attack)
		choke_amount = 1;

	// clamp choke limit when exploit's enabled...
	auto cfg = config->Ragebot.pistol;
	if (info->get_weapon_id() == weapon_g3sg1 || info->get_weapon_id() == weapon_scar20)
		cfg = config->Ragebot._auto;
	else if (info->get_weapon_id() == weapon_ssg08)
		cfg = config->Ragebot.scout;
	else if (info->get_weapon_id() == weapon_awp)
		cfg = config->Ragebot.awp;
	else if (info->get_weapon_id() == weapon_deagle || info->get_weapon_id() == weapon_revolver)
		cfg = config->Ragebot.heavy;
	else if (info->WeaponType == weapontype_pistol)
		cfg = config->Ragebot.pistol;
	else if (info->get_weapon_id() == weapon_taser)
		cfg = config->Ragebot.taser;
	else
		cfg = config->Ragebot.other;

	if (cfg.exploits >= 1 && choke_amount > 2) choke_amount = 2;

	// set send packet and stats.
	const auto shot_last_tick = antiaim->shot_cmd <= cmd->command_number && antiaim->shot_cmd > cmd->command_number - client_state->choked_commands;
	unit::bsendpacket = client_state->choked_commands >= choke_amount || (!antiaim->is_fakeducking && shot_last_tick);

	estimated_choke = choke_amount;

	// store data of current tick for next evaluation.
	last_origin = local->get_origin();
	last_simtime = global_vars_base->curtime;
	last_ducked = cmd->buttons & c_user_cmd::flags::duck;

	choking_amt = choke_amount;
}

void c_antiaim::run(c_cs_player* local, c_user_cmd* cmd, bool& send_packet, bool is_revolver_shooting)
{
	static const auto get_check_sum = reinterpret_cast<uint32_t(__thiscall*)(c_user_cmd*)>(sig("client.dll", "53 8B D9 83 C8"));
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	
	if (!weapon)
		return;

	const auto info = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (
		!local ||
		!local->is_alive() ||
		!weapon ||
		!info ||
		local->get_move_type() == c_base_player::movetype_observer ||
		local->get_move_type() == c_base_player::movetype_noclip ||
		local->get_move_type() == c_base_player::movetype_ladder ||
		cmd->buttons & c_user_cmd::flags::use ||
		info->WeaponType == weapontype_grenade ||
		weapon->get_item_definition() == weapon_hegrenade || weapon->get_item_definition() == weapon_molotov || weapon->get_item_definition() == weapon_incgrenade || weapon->get_item_definition() == weapon_smokegrenade ||
		!config->Ragebot._antiaim ||
		(config->Ragebot.disableaaonknife && info->WeaponType == weapontype_knife) ||
		(config->Ragebot.disableaaonfreeze && (local->get_flags() & c_cs_player::frozen || game_rules->is_freeze_period())))
	{
		last_real = cmd->viewangles.y;
		last_fake = cmd->viewangles.y;
		inied = false;
		return;
	}

	//if (cmd->buttons & c_user_cmd::flags::attack) //????? ? ??????? ?????? @PiZZY
	//return;

	inied = true;
	jitter = !jitter;

	const auto standing = local->get_velocity().length2d() <= 15 && !(cmd->buttons & c_user_cmd::jump);
	const auto moving =   local->get_velocity().length2d() > 15 && !(cmd->buttons & c_user_cmd::jump);
	const auto air = !(local->get_flags() & c_cs_player::flags::on_ground) || cmd->buttons & c_user_cmd::jump;
	Menu22::ragebot::antiaim_config cfg;

	if (standing && !air)
		cfg = config->Ragebot.stand;

	else if (moving && !air)
		cfg = config->Ragebot.moving;

	if (air)
		cfg = config->Ragebot.air;

	static bool is_on_shot = false;

	for (auto i = 0; i <= client_state->choked_commands; i++)
	{
		auto& c = input->commands[(cmd->command_number + i - client_state->choked_commands) % 150];
		auto& verified = input->verified_commands[(cmd->command_number + i - client_state->choked_commands) % 150];

		//if ((c.command_number == shot_cmd) || (GetKeyState(config->Ragebot._auto.dt_key) && abs(c.command_number - (int)shot_cmd) < 2)) //&& shot_cmd >= cmd->command_number - client_state->choked_commands)
		if (c.command_number >= shot_cmd && shot_cmd >= c.command_number - client_state->choked_commands)
		{
			auto target_angle = c.viewangles;
			c.viewangles = input->commands[shot_cmd % 150].viewangles;
			c.tick_count = input->commands[shot_cmd % 150].tick_count;
			c_aimhelper::fix_movement(&c, target_angle);
			math::ensure_bounds(c.viewangles, *reinterpret_cast<c_vector3d*>(&c.forwardmove));
			c_miscellaneous::set_buttons_for_direction(&c);

			verified.cmd = c;
			verified.crc = get_check_sum(&verified.cmd);
			is_on_shot = true;

			last_real = c.viewangles.y;
			last_fake = c.viewangles.y;

			return;
		}
	}

	auto target_angle = cmd->viewangles;
	view_angles = cmd->viewangles;

	switch (cfg.pitch) {
		case 1: { cmd->viewangles.x = -89.f;  }  break;
		case 2: { cmd->viewangles.x =  89.f;  }  break;
		case 3: { cmd->viewangles.x = cmd->tick_count % 2 ? -cfg.pitch_range : cfg.pitch_range; }  break;
	}

	static float manual_add_yaw = 0.f;
	static bool jitter_side_left = false;

	if (is_on_shot)
		jitter_side_left = !jitter_side_left;

	if (GetAsyncKeyState(config->Ragebot.left) && config->Ragebot.left_enable)
		manual_add_yaw = -90.f;

	if (GetAsyncKeyState(config->Ragebot.right) && config->Ragebot.right_enable)
		manual_add_yaw = 90.f;

	if (GetAsyncKeyState(config->Ragebot.back) && config->Ragebot.back_enable)
		manual_add_yaw = 0.f;

	static bool  invert = false;
	static float lastTime = 0;

	if (GetAsyncKeyState(config->Ragebot.desync_invert) && fabs(global_vars_base->curtime - lastTime) > 0.75f) {
		invert = !invert;
		lastTime = global_vars_base->curtime;
	}

	static bool can_move_sway = false;

	if (moving || air)
	{
		if (send_packet)
		{
			cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw);
			last_fake = (cmd->viewangles.y);
		}
		else
		{
			if (cfg.fake_type == 1)
			{
				if (invert)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + math::random_float(90, 130)); // breaking overlay resolver's
				else
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw - math::random_float(90, 130));

				last_real = (cmd->viewangles.y);
			}
			else if (cfg.fake_type == 2)
			{
				if (invert)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + math::random_float(90, 130)); // breaking overlay resolver's
				else
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw - math::random_float(90, 130));

				last_real = (cmd->viewangles.y);
			}
			else if (cfg.fake_type == 3) // jitter aa
			{
				/* jitter settings */
				static const float time_to_change_side = 2;  // in seconds, 1.0 - it's just one second
				static const bool  flip_after_shot = true;   // change side after onshot
				static const float time_to_breaking_resolvers = 2.2; // time when low-desync-delta applied.
				static const float time_how_much_low_delta_applied = 0.25; // time how much low-desync-delta work
				//

				/* Don't Change THIS */
				static bool  side_right = false;
				static float last_time_flip = 0;
				static float last_time_resolver_break = 0;
				static bool  is_on_low_delta = false;
				
				if (fabs(global_vars_base->curtime - last_time_flip) > time_to_change_side)
				{
					side_right = !side_right;
					last_time_flip = global_vars_base->curtime;
				}

				if (flip_after_shot && is_on_shot)
					side_right = !side_right;

				if (fabs(global_vars_base->curtime - last_time_resolver_break) > time_to_breaking_resolvers)
				{
					last_time_resolver_break = global_vars_base->curtime;
					is_on_low_delta = true;
				}

				if (fabs(global_vars_base->curtime - last_time_resolver_break) > time_how_much_low_delta_applied)
					is_on_low_delta = false;

				if (is_on_low_delta)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw - math::random_float(16, 19));
				else if (side_right)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + math::random_float(90, 130)); // breaking overlay resolver's
				else
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw - math::random_float(90, 130));

				last_real = (cmd->viewangles.y);
			}
			else if (cfg.fake_type == 4 || cfg.fake_type == 5)
			{
				if (invert)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + (max_delta - 5));
				else
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw - (max_delta - 5));

				last_real = (cmd->viewangles.y);
			}
		}
	}
	else
	{
		if (send_packet)
		{
			if (cfg.fake_type == 1 || cfg.fake_type == 2)
			{
				static bool jitter = false;
				jitter = !jitter;

				if (jitter)
				{
					if (invert)
						cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 - (cfg.jitter_length) + manual_add_yaw - (cfg.lean_break));
					else
						cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + cfg.jitter_length + manual_add_yaw + cfg.lean_break);
				}
				else
				{
					if (invert)
						cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw - (cfg.lean_break));
					else
						cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break);
				}		
			}
			else
			{
				if (invert)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw - (cfg.lean_break));
				else
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break);
			}

			last_fake = cmd->viewangles.y;
			can_move_sway = true; // we should enable sway on send packet
		}
		else
		{
			if (cfg.fake_type == 1)
			{
				if (invert)
					cmd->viewangles.y = math::normalize_yaw(last_fake + 120);
				else
					cmd->viewangles.y = math::normalize_yaw(last_fake - 120);

				last_real = cmd->viewangles.y;
			}
			else if (cfg.fake_type == 2)
			{
				if (invert)
					cmd->viewangles.y = math::normalize_yaw(last_fake + 120);
				else
					cmd->viewangles.y = math::normalize_yaw(last_fake - 120);

				last_real = cmd->viewangles.y;
			}
			else if (cfg.fake_type == 3)
			{
				if (jitter_side_left)
				{
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break - 120);
					last_real = (cmd->viewangles.y);
				}
				else
				{
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break + 120);
					last_real = (cmd->viewangles.y);
				}
			}
			else if (cfg.fake_type == 4)
			{
				static int  desync_add = 0;
				static int  add_angle = 0;
				static bool invertor_angle = false;

				if (can_move_sway)
				{
					if (add_angle > 1)
						invertor_angle = !invertor_angle;
					else if (add_angle < -1)
						invertor_angle = !invertor_angle;

					if (invertor_angle)
						add_angle--;
					else if (!invertor_angle)
						add_angle++;

					can_move_sway = false; // we are waiting for next send packet tick to allow cheat break angle.
				}

				desync_add = min_delta + add_angle;

				if (invert)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break + desync_add * (-1));
				else
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break + desync_add);

				last_real = (cmd->viewangles.y);
			}
			else if (cfg.fake_type == 5)
			{
				if (invert)
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break + (max_delta - 5));
				else
					cmd->viewangles.y = math::normalize_yaw(target_angle.y + 180 + manual_add_yaw + cfg.lean_break - (max_delta - 5));

				last_real = (cmd->viewangles.y);
			}
		}
	}

	c_aimhelper::fix_movement(cmd, target_angle);
	math::ensure_bounds(cmd->viewangles, *reinterpret_cast<c_vector3d*>(&cmd->forwardmove));
	c_miscellaneous::set_buttons_for_direction(cmd);

	//	const auto in_attack = (cmd->buttons & c_user_cmd::attack) || (cmd->buttons & c_user_cmd::attack2);

	/* Hide velocity to force OTC users to dump our desync */
	if (fabs(cmd->sidemove) < 5
		&& !(cmd->buttons & c_user_cmd::move_left)
		&& !(cmd->buttons & c_user_cmd::move_right)
		&& config->Ragebot.fakelag_enable && !air)
	{
		if (cfg.fake_type == 1 && !send_packet && antiaim->current_choke_cycle > 4 && (client_state->choked_commands == (antiaim->current_choke_cycle - 1) || client_state->choked_commands == (antiaim->current_choke_cycle - 2)))
		{
			cmd->sidemove = cmd->command_number % 2 == 0 ? 0.55f : -0.55f;
			if (cmd->buttons & c_user_cmd::duck)
				cmd->sidemove *= 3;
		}
		else if (cfg.fake_type == 2)
		{
			cmd->sidemove = cmd->command_number % 2 ? 1.1f : -1.1f;
			if (cmd->buttons & c_user_cmd::duck)
				cmd->sidemove *= 3;
		}
		else if (cfg.fake_type == 3)
		{
			cmd->sidemove = cmd->command_number % 2 ? 1.15f : -1.15f;
			if (cmd->buttons & c_user_cmd::duck)
				cmd->sidemove *= 3;
		}
		else if (cfg.fake_type == 4)
		{
			cmd->sidemove = cmd->command_number % 2 ? 1.19f : -1.19f;
			if (cmd->buttons & c_user_cmd::duck)
				cmd->sidemove *= 3;
		}
		else if (cfg.fake_type == 5 && !send_packet && antiaim->current_choke_cycle > 4 && (client_state->choked_commands == (antiaim->current_choke_cycle - 1) || client_state->choked_commands == (antiaim->current_choke_cycle - 2)))
		{
			cmd->sidemove = cmd->command_number % 2 == 0 ? 0.55f : -0.55f;
			if (cmd->buttons & c_user_cmd::duck)
				cmd->sidemove *= 3;
		}
	}

	is_on_shot = false;

	/*
	if (!in_attack
		&& local->get_velocity().length2d() < 15.f
		&& !(cmd->buttons & c_user_cmd::move_left)
		&& !(cmd->buttons & c_user_cmd::move_right)
		&& config->Ragebot.fakelag_enable)
	{
		cmd->sidemove = cmd->command_number % 2 == 0 ? 0.55f : -0.55f;
		if (cmd->buttons & c_user_cmd::duck)
			cmd->sidemove *= 3.2;
	}
	*/
}

void c_antiaim::prepare_animation(c_cs_player* local)
{
	const auto state = local->get_anim_state();

	*local->get_animation_layers() = animation_system->local_animation.layers;
	local->get_pose_parameter() = animation_system->local_animation.poses;

	if (local->get_move_type() == c_base_player::movetype_observer ||
		local->get_move_type() == c_base_player::movetype_noclip ||
		local->get_move_type() == c_base_player::movetype_ladder ||
		local->get_flags() & c_base_player::frozen ||
		game_rules->is_freeze_period() ||
		//!c_events::is_active_round ||
		!state ||
		!config->Ragebot.enable)
		return;

	local->get_animation_layers()->at(7).weight = 0.f;
}

void c_antiaim::predict(c_cs_player* local, c_user_cmd* cmd)
{
	const auto state = local->get_anim_state();
	if (!local->is_local_player() || !state)return;

	const auto time = ticks_to_time(local->get_tick_base());

	if (local->get_velocity().length2d() >= .1f || fabsf(local->get_velocity().z) >= 100.f)
		next_lby_update = time + .22f;
	else if (time > next_lby_update)
	{
		lby_update = time;
		next_lby_update = time + 1.1f;
	}

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	min_delta = *reinterpret_cast<float*>(&state->pad10[512]);
	max_delta = *reinterpret_cast<float*>(&state->pad10[516]);

	float max_speed = 260.f;

	if (weapon)
	{
		const auto info = weapon_system->get_weapon_data(weapon->get_item_definition());
#undef max

		if (info)
			max_speed = std::max(.001f, info->flMaxPlayerSpeed);
	}

	auto velocity = local->get_velocity();
	const auto abs_velocity_length = powf(velocity.length(), 2.f);
	const auto fraction = 1.f / (abs_velocity_length + .00000011920929f);

	if (abs_velocity_length > 97344.008f)
		velocity *= velocity * 312.f;

	auto speed = velocity.length();

	if (speed >= 260.f)
		speed = 260.f;

	feet_speed_stand = (1.923077f / max_speed) * speed;
	feet_speed_ducked = (2.9411764f / max_speed) * speed;

	auto feet_speed = (((stop_to_full_running_fraction * -.3f) - .2f) * std::clamp(feet_speed_stand, 0.f, 1.f)) + 1.f;

	if (state->duck_amount > 0.f)
		feet_speed = feet_speed + ((std::clamp(feet_speed_ducked, 0.f, 1.f) * state->duck_amount) * (.5f - feet_speed));

	min_delta *= feet_speed;
	max_delta *= feet_speed;

	if (stop_to_full_running_fraction > 0.0 && stop_to_full_running_fraction < 1.0)
	{
		const auto interval = global_vars_base->interval_per_tick * 2.f;

		if (inied)
			stop_to_full_running_fraction = stop_to_full_running_fraction - interval;
		else
			stop_to_full_running_fraction = interval + stop_to_full_running_fraction;

		stop_to_full_running_fraction = std::clamp(stop_to_full_running_fraction, 0.f, 1.f);
	}

	if (speed > 135.2f && inied)
	{
		stop_to_full_running_fraction = fmaxf(stop_to_full_running_fraction, .0099999998f);
		inied = false;
	}

	if (speed < 135.2f && !inied)
	{
		stop_to_full_running_fraction = fminf(stop_to_full_running_fraction, .99000001f);
		inied = true;
	}
}

#ifdef OLD_ON_PEEK_CHECKER
bool c_antiaim::on_peek(c_cs_player* local, bool& target)
{
	target = true;

	const auto weapon_cfg = c_aimhelper::get_weapon_conf();

	if (local->get_abs_velocity().length2d() < 2.f || !weapon_cfg.has_value())
		return false;

	target = false;

	//const auto velocity = local->get_velocity() * (2.f / 3.f);
	//const auto ticks = client_state->choked_commands > 1 ? 14 : 10;
	const auto pos = local->get_shoot_position(); // + velocity * ticks_to_time(ticks);

	auto found = false;

	client_entity_list()->for_each_player([&](c_cs_player* player) -> void
		{
			if (!player->is_enemy() || player->is_dormant() || player->is_local_player())
				return;

			const auto record = animation_system->get_latest_animation(player);

			if (!record.has_value())
				return;

			const auto scan = c_ragebot::scan_record_gun(local, record.value(), pos);

			if (scan.has_value() && (scan.value().damage >= 4 /*weapon_cfg.value().min_dmg*/
				|| record.value()->health < scan.value().damage))
				found = true;
		});

	if (found)
		return true;

	target = true;
	return false;
}
#else
bool c_antiaim::on_peek(c_cs_player* local, bool& target)
{
	target = true;

	const auto weapon_cfg = c_aimhelper::get_weapon_conf();

	if (local->get_velocity().length2d() < 10.f || local->get_velocity().length2d() > 220 || !weapon_cfg.has_value() || !local->is_on_ground()) // avoid to scan this always..
		return false;

	target = false;

	c_vector3d velocity_per_tick = local->get_velocity() * global_vars_base->interval_per_tick;
	const auto pos = local->get_shoot_position() + (velocity_per_tick * 4);

	auto found = false;
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	if (!weapon)
		return;

	client_entity_list()->for_each_player([&](c_cs_player* player) -> void
	{
		if (!player->is_enemy() || player->is_dormant() || player->is_local_player() || !player->is_alive())
			return;

		const auto record = animation_system->get_latest_animation(player);
		if (!record.has_value())
			return;

		const auto scan = c_ragebot::scan_record_gun(local, record.value(), weapon, weapon_cfg, pos);

		if (scan.has_value() && scan.value().damage >= 1)
			found = true;
	});

	if (found)
		return true;

	target = true;
	return false;
}
#endif

float c_antiaim::calculate_ideal_yaw(c_cs_player* local, bool estimate)
{
	// step in which we test for damage.
	static constexpr auto step = 90.f;

	// maybe the name and the arguments of this function are questionable.
	const auto target = c_aimhelper::get_legit_target(28738174.56f, 0.f, c_cs_player::hitbox::head);

	if (!target.has_value())
		return engine_client()->get_view_angles().y + 180;

	const auto anim = std::get<2>(target.value());
	const auto head = anim->player->get_shoot_position();
	const auto shoot = local->get_shoot_position();
	const auto direction = math::calc_angle(local->get_origin(), anim->player->get_origin());

	// determine damage points.
	float angle_to_damage_ratio[4] = { };
	c_vector3d positions[7] = { };
	for (auto i = 0; i < 4; i++)
	{
		const auto current_angle = direction.y + i * step;
		auto& ratio = angle_to_damage_ratio[i];

		auto back_pos = math::rotate_2d(shoot, current_angle, 19.f);

		if (i == 0 || i == 2)
		{
			positions[0] = math::rotate_2d(back_pos + c_vector3d(0.f, 0.f, -6.f), current_angle + 90.f, 5.f);
			positions[1] = math::rotate_2d(back_pos + c_vector3d(0.f, 0.f, -6.f), current_angle + 90.f, -5.f);
			positions[2] = math::rotate_2d(back_pos + c_vector3d(0.f, 0.f, 0.f), current_angle + 90.f, 0.f);
			positions[3] = math::rotate_2d(back_pos + c_vector3d(0.f, 0.f, -6.f), current_angle + 90.f, 0.f);
			positions[4] = math::rotate_2d(back_pos + c_vector3d(0.f, 0.f, -6.f), current_angle + 90.f, 2.5f);
			positions[5] = math::rotate_2d(back_pos + c_vector3d(0.f, 0.f, -6.f), current_angle + 90.f, -2.5f);
			positions[6] = math::rotate_2d(back_pos + c_vector3d(0.f, 0.f, -3.f), current_angle + 90.f, 0.f);
		}
		else
		{
			positions[0] = math::rotate_2d(shoot + c_vector3d(0.f, 0.f, -6.f), current_angle, 27.f);
			positions[1] = math::rotate_2d(shoot + c_vector3d(0.f, 0.f, -6.f), current_angle, 21.f);
			positions[2] = math::rotate_2d(shoot + c_vector3d(0.f, 0.f, 0.f), current_angle, 21.f);
			positions[3] = math::rotate_2d(shoot + c_vector3d(0.f, 0.f, -7.f), current_angle, 13.f);
			positions[4] = math::rotate_2d(shoot + c_vector3d(0.f, 0.f, -6.f), current_angle, 24.f);
			positions[5] = math::rotate_2d(shoot + c_vector3d(0.f, 0.f, -6.f), current_angle, 17.f);
			positions[6] = math::rotate_2d(shoot + c_vector3d(0.f, 0.f, -3.f), current_angle, 21.f);
		}

		// the run is only for visual representation, we do not need accurate data, so we drop multipoints.
		if (estimate)
		{
			const auto wall = trace_system->wall_penetration(head, positions[2], nullptr, nullptr, local);
			if (!wall.has_value()) continue;

			if (wall.value().damage > ratio)
				ratio = wall.value().damage;

			continue;
		}

		for (auto& pos : positions)
		{
			const auto wall = trace_system->wall_penetration(head, pos, nullptr, nullptr, local);
			if (!wall.has_value()) continue;

			if (wall.value().damage > ratio)
				ratio = wall.value().damage;

			// abort if angle is already out in the open.
			if (ratio >= 60000.f)
				break;
		}
	}

	// determine lowest and highest damage.
	auto lowest_dmg = std::make_pair(0, FLT_MAX), highest_dmg = std::make_pair(0, 0.f);
	for (auto i = 3; i >= 0; i--)
	{
		const auto& ratio = angle_to_damage_ratio[i];

		// only ever face the enemy forwards if we gain a significant
		// advantage compared to all alternatives.
		if (i == 0 && lowest_dmg.second > 0.f
			&& fabsf(lowest_dmg.second - ratio) < 100.f)
			continue;

		if (lowest_dmg.second > ratio)
			lowest_dmg = std::make_pair(i, ratio);

		if (ratio > highest_dmg.second)
			highest_dmg = std::make_pair(i, ratio);
	}

	// determine second highest damage.
	auto second_highest_dmg = 0.f;
	for (auto i = 3; i >= 0; i--)
	{
		const auto& ratio = angle_to_damage_ratio[i];

		if (ratio > second_highest_dmg && highest_dmg.first != i)
			second_highest_dmg = ratio;
	}

	// no suitable point found.
	if (fabsf(lowest_dmg.second - highest_dmg.second) < 20.f && highest_dmg.second <= 40000.f)
		return engine_client()->get_view_angles().y + 180.f;

	// force backwards at target.
	if (fabsf(lowest_dmg.second - highest_dmg.second) < 100.f)
		return direction.y + 2 * step;

	// force opposite when head on edge.
	if (fabsf(second_highest_dmg - highest_dmg.second) >= 50000.f)
		return direction.y + highest_dmg.first * step + 180.f;

	// set target yaw.
	return direction.y + lowest_dmg.first * step;
}

float c_antiaim::at_target(c_cs_player* local) const
{
	c_cs_player* target = nullptr;
	c_qangle target_angle;

	c_qangle original_viewangles;
	original_viewangles = engine_client()->get_view_angles();

	auto lowest_fov = 90.f;
	for (auto i = 0; i < global_vars_base->max_clients; i++)
	{
		auto player = reinterpret_cast<c_cs_player*>(client_entity_list()->get_client_entity(i));
		if (!player || !player->is_alive() || !player->is_enemy() || player == c_cs_player::get_local_player())
			continue;

		if (player->is_dormant() && (player->get_simtime() > global_vars_base->curtime || player->get_simtime() + 5.f < global_vars_base->curtime))
			continue;

		auto enemy_pos = player->get_origin();
		enemy_pos.z += 64.f;

		const auto angle = math::calc_angle(c_cs_player::get_local_player()->get_shoot_position(), enemy_pos);
		const auto fov = math::get_fov(original_viewangles, angle);

		if (fov < lowest_fov)
		{
			target = player;
			lowest_fov = fov;
			target_angle = angle;
		}
	}

	if (!target)
		return 0.f;

	if (target->is_dormant())
		return 0.f;

	return target_angle.y + 180.f;
}

float c_antiaim::get_visual_choke()
{
	return visual_choke;
}

void c_antiaim::increment_visual_progress()
{
	if (!config->Ragebot._antiaim || !engine_client()->is_ingame())
		return;

	visual_choke = 1.f;

	if (estimated_choke >= 2)
		visual_choke = static_cast<float>(client_state->choked_commands) / static_cast<float>(estimated_choke);
}

float c_antiaim::get_pitch(Menu22::ragebot::antiaim_config& conf) {
	float pitch = 0.f;

	switch (conf.pitch) {
	case 1: { pitch = -89.f; } break;
	case 2: { pitch = 89.f; } break;
	case 3: { pitch = jitter ? -89 : 0.f; } break;
	}

	return pitch;
}

float c_antiaim::get_last_real()
{
	return last_real;
}

float c_antiaim::get_last_fake()
{
	return last_fake;
}

float c_antiaim::get_last_lby()
{
	return last_lby;
}