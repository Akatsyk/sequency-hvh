#include "c_ragebot.h"
#include "c_aimhelper.h"
#include "c_trace_system.h"
#include "../utils/math.h"
#include "../sdk/c_weapon_system.h"
#include "../sdk/c_debug_overlay.h"
#include "c_prediction_system.h"
#include "c_antiaim.h"
#include "c_resolver.h"
#include "../hooks/c_client.h"
#include "../sdk/c_cvar.h"
#include "../baim.h"

bool c_ragebot::IsFireTime(c_user_cmd* cmd, const float time, const bool manual_shot) //h1 ledaris хуесос
{
	const auto local = c_cs_player::get_local_player();
	if (!local)
		return false;
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	if (!weapon)return false;
	const auto wpn_info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!wpn_info)return false;
	if (cmd->weapon_select || game_rules == nullptr || game_rules->is_freeze_period())
	{
		return false;
	}

	if (!weapon)
	{
		return false;
	}

	const auto info = wpn_info;

	if (!info)
	{
		return false;
	}

	const auto is_zeus = weapon->get_item_definition() == weapon_taser;
	const auto is_knife = !is_zeus && info->WeaponType == weapontype_knife;

	if (weapon->get_item_definition() == weapon_c4 || !weapon->IsGun())
	{
		return false;
	}

	if (weapon->get_current_clip() < 1 && !is_knife)
	{
		return false;
	}

	if (weapon->get_in_reload())
	{
		return false;
	}

	if (weapon->get_next_primary_attack() > time)
	{
		if (weapon->get_item_definition() != weapon_revolver && info->WeaponType == weapontype_pistol)
			cmd->buttons &= ~c_user_cmd::attack;

		return false;
	}

	return true;
}

void VectorAngles(const c_vector3d& forward, c_vector3d& angles)
{
	float tmp, yaw, pitch;

	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 270;
		else
			pitch = 90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / pi);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt(forward[0] * forward[0] + forward[1] * forward[1]);
		pitch = (atan2(-forward[2], tmp) * 180 / pi);
		if (pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

void inline SinCos23(float radians, float* sine, float* cosine)
{
	*sine = sin(radians);
	*cosine = cos(radians);
}

void AngleVectors(const c_vector3d& angles, c_vector3d* forward)
{
	float	sp, sy, cp, cy;

	SinCos23(deg2rad(angles[1]), &sy, &cy);
	SinCos23(deg2rad(angles[0]), &sp, &cp);

	forward->x = cp * cy;
	forward->y = cp * sy;
	forward->z = -sp;
}

void Autostop(c_cs_player* local, c_user_cmd* cmd, float MaxVelocity)
{
	//quick_stop_called = prediction::get().get_unpred_curtime();
	cmd->viewangles.clamp_angles();
	auto unpred_vel = local->get_velocity(); //prediction_system->m_move_data.velocity;
	const auto speed = unpred_vel.length();
	if (speed > 15.f)
	{
		c_qangle dir;
		math::vector_angles(unpred_vel, dir);
		dir.y = cmd->viewangles.y - dir.y;
		c_vector3d new_move;
		math::angle_vectors(dir, new_move);
		const auto max = std::max(std::fabs(cmd->forwardmove), std::fabs(cmd->sidemove));
		const auto mult = 450.f / max;
		new_move *= -mult;

		cmd->forwardmove = new_move.x;
		cmd->sidemove = new_move.y;
	}
	else
	{
		//cmd->forwardmove = 0.f;
		//cmd->sidemove = 0.f;
	}

	/*
	const auto speed = local->get_velocity().length();
	if (speed > 15.f)
	{
		c_qangle dir;
		math::vector_angles(local->get_velocity(), dir);
		dir.y = cmd->viewangles.y - dir.y;

		c_vector3d new_move;
		math::angle_vector___(dir, new_move);
		const auto max = std::max(std::fabs(cmd->forwardmove), std::fabs(cmd->sidemove));
		const auto mult = 450.f / max;
		new_move *= -mult;

		cmd->forwardmove = new_move.x;
		cmd->sidemove = new_move.y;
	}
	else
	{
		cmd->forwardmove = 0.f;
		cmd->sidemove = 0.f;
	}
	*/
}

float aim_simtime;

float accepted_inaccuracy()
{
	if (!c_cs_player::get_local_player() || c_cs_player::get_local_player()->get_health() <= 0)
		return 0;

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(c_cs_player::get_local_player()->get_current_weapon_handle()));
	if (!weapon)
		return 0;

	float inaccuracy = weapon->get_inaccuracy();
	if (inaccuracy == 0) inaccuracy = 0.0000001;
	inaccuracy = 1 / inaccuracy;
	return inaccuracy;
}

float GetCurtime()
{
	auto local = c_cs_player::get_local_player();
	if (!local)
		return 0;

	return global_vars_base->curtime;
}

/*
bool can_shoot(c_user_cmd* cmd)
{
	auto local = c_cs_player::get_local_player();
	if (!local)
		return false;

	if (!local->get_health() <= 0)
		return false;

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	return (weapon->get_next_primary_attack() < GetCurtime()) && (local->get_next_attack() < GetCurtime());
}
*/

void c_ragebot::no_recoil(c_cs_player* local, c_user_cmd* cmd)
{
	if (!engine_client()->is_ingame() || !engine_client()->is_connected() || !local || !local->is_alive())
		return;

	static auto weapon_recoil_scale = cvar()->find_var("weapon_recoil_scale")->get_float();
	cmd->viewangles -= local->get_punch_angle() * weapon_recoil_scale;
}

void c_ragebot::aim(c_cs_player* local, c_user_cmd* cmd, bool& send_packet, c_base_combat_weapon* weapon)
{
	if (!engine_client()->is_ingame() || !engine_client()->is_connected() || !local || !local->is_alive())
		return;

	resolver.has_target = false;
	last_pitch = std::nullopt;

	//const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	if (!weapon)
		return;

	const auto wpn_info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!wpn_info)
		return;

	if (!local->can_shoot(cmd, global_vars_base->curtime, weapon))
		return;

	//if (local->get_velocity().length2d() > (wpn_info->flMaxPlayerSpeed * 3.1) || !local->is_on_ground())
	//	return;

	auto weapon_cfg = c_aimhelper::get_weapon_conf();
	if (!weapon_cfg.has_value())
		return;

	auto backup = weapon_cfg.value().min_dmg;
	float max_damage = 0;

	std::vector<aim_info> hitpoints = {};
	std::deque<std::tuple<float, c_cs_player*>> target_selection = {};

	auto local_origin = local->get_origin();

	client_entity_list()->for_each_player([&](c_cs_player* player) -> void
		{
			if (!player || player->is_dormant() || !player->is_enemy() ||
				!player->is_alive() || player->get_gun_game_immunity()/* || (player->get_velocity().length2d() > 220 && player->is_on_ground())*/)
				return;

			float health = (float)player->get_health();
			auto distance = (local_origin - player->get_origin()).length2d();
			if (distance > 1.f && health > 0.f)  // e.g. 10 health = distance * 0.1f
				distance *= (health / 100.f);    // e.g. 100 health = distance * 1.f

			target_selection.push_back(std::make_tuple(distance, player));
		});

	std::sort(std::begin(target_selection), std::end(target_selection),
		[](auto const& t1, auto const& t2)
		{
			return std::get<0>(t1) < std::get<0>(t2);
		});

	for (auto current_entity = target_selection.cbegin();
		current_entity != target_selection.cend(); current_entity++)
	{
		// std::get c_cs_player*
		const auto player = (c_cs_player*)(std::get<1>(*current_entity));
		if (!player)
			continue;

		const auto latest = animation_system->get_latest_animation(player);

		if (!latest.has_value())
			continue;

		const auto is_knife = wpn_info->WeaponType == weapontype_knife;

		std::optional<aim_info> aimbot_backtrack = {};
		std::optional<aim_info> target = {};

		auto all_valid_anims = animation_system->get_valid_animations(player);
			for (auto& animation : all_valid_anims)
			{
				if (animation->is_backtrackable && animation->velocity.length2d() < 100)
				{
					const auto alternative = is_knife ? scan_record_knife(local, animation) :
						scan_record_gun(local, animation, weapon, weapon_cfg);

					if (alternative->damage > 0.f && alternative->damage > max_damage)
					{
						aimbot_backtrack = alternative;
						max_damage = alternative->damage;
						break;
					}
				}
				else if (animation->velocity.length2d() > 45 && animation->velocity.length2d() < 170 && animation->player->is_on_ground()) // don't need to scan if they are standing or backing window so small
				{
					const auto alternative = is_knife ? scan_record_knife(local, animation) :
						simple_backtrack_record_gun(local, animation);

					if (alternative->damage > 0.f && alternative->damage > max_damage)
					{
						aimbot_backtrack = alternative;
						max_damage = alternative->damage;
						break;
					}
				}
			}

			const auto latest_record = is_knife ? scan_record_knife(local, latest.value()) :
				scan_record_gun(local, latest.value(), weapon, weapon_cfg);

			if (aimbot_backtrack.has_value() && latest_record.has_value())
			{
				if (latest_record.value().damage > aimbot_backtrack.value().damage) {
					target = latest_record;    // break; 
				}
				else {
					target = aimbot_backtrack; // break;
				}
			}
			else if (latest_record.has_value() && latest_record.value().damage > 0.f)
			{
				target = latest_record;    // break;
			}
			else if (aimbot_backtrack.has_value() && aimbot_backtrack.value().damage > 0.f)
			{
				target = aimbot_backtrack; //break;
			}
			else
			{
				const auto oldest = animation_system->get_oldest_animation(player);
				if (oldest.has_value() && oldest.value() != latest.value() && oldest.value()->velocity.length2d() > 75 && latest.value()->velocity.length2d() > 50)
				{
					const auto oldest_record = is_knife ?
						scan_record_knife(local, oldest.value()):
						scan_record_gun(local,   oldest.value(), weapon, weapon_cfg);

					if (oldest_record.has_value() && oldest_record.value().damage > 0.f)
					{
						target = oldest_record; //break;
					}
					else
						continue;
				}
				else if (oldest.has_value() && oldest.value() != latest.value() && oldest.value()->velocity.length2d() < 60 && latest.value()->velocity.length2d() > 75)
				{
					const auto oldest_record = is_knife ?
						scan_record_knife(local, oldest.value()):
						scan_record_gun(local,   oldest.value(), weapon, weapon_cfg);

					if (oldest_record.has_value() && oldest_record.value().damage > 0.f)
					{
						target = oldest_record; //break;
					}
					else
						continue;
				}
			}

		if (target.has_value())
			hitpoints.push_back(target.value());
	}

	aim_info best_match = { c_vector3d(), -FLT_MAX, nullptr, false, c_vector3d(), 0.f, 0.f, c_cs_player::hitbox::head, 0 };

	// find best target spot of all valid spots.
	for (auto& hitpoint : hitpoints)
		if (hitpoint.damage > best_match.damage)
			best_match = hitpoint;

	// stop if no target found.
	if (best_match.damage < 0.f)
		return;

	float MinimumVelocity = 0;
	if (weapon->get_zoom_level() != 0)
		MinimumVelocity = wpn_info->flMaxPlayerSpeedAlt * .32f;
	else
		MinimumVelocity = wpn_info->flMaxPlayerSpeed * .32f;

	bool canShoot = true;
	if (canShoot)
	{
		if (GetKeyState(config->Ragebot._auto.dt_key))
		{
			// scope the weapon.
			if ((wpn_info->get_weapon_id() == weapon_g3sg1 || wpn_info->get_weapon_id() == weapon_scar20
				|| wpn_info->get_weapon_id() == weapon_ssg08 || wpn_info->get_weapon_id() == weapon_awp
				|| wpn_info->get_weapon_id() == weapon_sg556 || wpn_info->get_weapon_id() == weapon_aug) && weapon->get_zoom_level() == 0 && config->Ragebot.autoscope)
				cmd->buttons |= c_user_cmd::flags::attack2;

			Autostop(local, cmd, MinimumVelocity);

			auto shoot_pos = local->get_shoot_position();
			if (shoot_pos == c_vector3d(0, 0, 0))
				return;

			auto angle = math::calc_angle(shoot_pos, best_match.position);

			// store pitch for eye correction.
			last_pitch = angle.x;

			if (c_aimhelper::hit_chance(angle, best_match.animation->player, weapon_cfg.value().hitchance, best_match.hitbox, best_match.animation, best_match.position, shoot_pos, weapon, local))
			{
				// set correct information to user_cmd.
				cmd->viewangles = angle;
				no_recoil(local, cmd);

				cmd->tick_count = time_to_ticks(best_match.animation->sim_time + calculate_lerp());

				if (config->Ragebot.autofire) {
					antiaim->set_last_shot(true);
					if ((!send_packet && config->Ragebot.fakeduck_enable && GetAsyncKeyState(config->Ragebot.fakeduck)) || (!config->Ragebot.fakeduck_enable || !GetAsyncKeyState(config->Ragebot.fakeduck)))
						cmd->buttons |= best_match.alt_attack ? c_user_cmd::attack2 : c_user_cmd::attack;

					// store shot info for resolver.
					if (!best_match.alt_attack) {
						resolver.last_aim_angle = angle;
						resolver.last_eye_pos = shoot_pos;
						resolver.last_shot_missed_index = best_match.animation->index;
						resolver.last_hitbox = best_match.hitbox;
						memcpy(resolver.last_bones, best_match.animation->bones, 128 * sizeof(float) * 12);
						resolver.has_target = true;

						resolver_shot::shot shot{};
						shot.damage = best_match.damage;
						shot.start = shoot_pos;
						shot.end = best_match.position;
						shot.hitgroup = best_match.hitgroup;
						shot.hitbox = best_match.hitbox;
						shot.time = global_vars_base->curtime;
						shot.record = *best_match.animation;
						shot.manual = false;
						shot.confirmed = false;
						shot.skip = false;
						shot.impacted = false;
						c_resolver::register_shot(std::move(shot));
					}
					if (config->Visuals.lagcompydady)
						best_match.animation->player->draw_hitboxes(best_match.animation->bones, 5.f,
							config->Colors.lagcompcolor[0] * 255,
							config->Colors.lagcompcolor[1] * 255,
							config->Colors.lagcompcolor[2] * 255,
							config->Colors.lagcompcolor[3] * 255);
				}
				else {
					// store shot info for resolver.
					if (!best_match.alt_attack)
					{
						resolver.last_aim_angle = angle;
						resolver.last_eye_pos = shoot_pos;
						resolver.last_shot_missed_index = best_match.animation->index;
						resolver.last_hitbox = best_match.hitbox;
						memcpy(resolver.last_bones, best_match.animation->bones, 128 * sizeof(float) * 12);
						resolver.has_target = true;

						resolver_shot::shot shot{};
						shot.damage = best_match.damage;
						shot.start = shoot_pos;
						shot.end = best_match.position;
						shot.hitgroup = best_match.hitgroup;
						shot.hitbox = best_match.hitbox;
						shot.time = global_vars_base->curtime;
						shot.record = *best_match.animation;
						shot.manual = false;
						shot.confirmed = false;
						shot.skip = false;
						shot.impacted = false;
						c_resolver::register_shot(std::move(shot));
					}

					//if (config->Visuals.lagcompydady)
						//best_match.animation->player->draw_hitboxes(best_match.animation->bones, 5.f, config->Colors.lagcompcolor[0] * 255, config->Colors.lagcompcolor[1] * 255, config->Colors.lagcompcolor[2] * 255, config->Colors.lagcompcolor[3] * 255);
				}
			}
		}
		else
		{
			// scope the weapon.
			if ((wpn_info->get_weapon_id() == weapon_g3sg1 || wpn_info->get_weapon_id() == weapon_scar20
				|| wpn_info->get_weapon_id() == weapon_ssg08 || wpn_info->get_weapon_id() == weapon_awp
				|| wpn_info->get_weapon_id() == weapon_sg556 || wpn_info->get_weapon_id() == weapon_aug) && weapon->get_zoom_level() == 0 && config->Ragebot.autoscope)
				cmd->buttons |= c_user_cmd::flags::attack2;

			Autostop(local, cmd, MinimumVelocity);

			// calculate angle.
			const auto shoot_pos = local->get_shoot_position();
			auto angle = math::calc_angle(shoot_pos, best_match.position);

			// store pitch for eye correction.
			last_pitch = angle.x;

			if (c_aimhelper::hit_chance(angle, best_match.animation->player, weapon_cfg.value().hitchance, best_match.hitbox, best_match.animation, best_match.position, shoot_pos, weapon, local))
			{
				// set correct information to user_cmd.
				cmd->viewangles = angle;
				no_recoil(local, cmd);

				cmd->tick_count = time_to_ticks(best_match.animation->sim_time + calculate_lerp());

				if (config->Ragebot.autofire) {

					antiaim->set_last_shot(true);
					if ((!send_packet && config->Ragebot.fakeduck_enable && GetAsyncKeyState(config->Ragebot.fakeduck)) || (!config->Ragebot.fakeduck_enable || !GetAsyncKeyState(config->Ragebot.fakeduck)))
						cmd->buttons |= best_match.alt_attack ? c_user_cmd::attack2 : c_user_cmd::attack;

					// store shot info for resolver.
					if (!best_match.alt_attack) {
						resolver.last_aim_angle = angle;
						resolver.last_eye_pos = shoot_pos;
						resolver.last_shot_missed_index = best_match.animation->index;
						resolver.last_hitbox = best_match.hitbox;
						memcpy(resolver.last_bones, best_match.animation->bones, 128 * sizeof(float) * 12);
						resolver.has_target = true;

						resolver_shot::shot shot{};
						shot.damage = best_match.damage;
						shot.start = shoot_pos;
						shot.end = best_match.position;
						shot.hitgroup = best_match.hitgroup;
						shot.hitbox = best_match.hitbox;
						shot.time =  global_vars_base->curtime;
						shot.record = *best_match.animation;
						shot.manual = false;
						shot.confirmed = false;
						shot.skip = false;
						shot.impacted = false;

						c_resolver::register_shot(std::move(shot));
					}

					//if (config->Visuals.lagcompydady)
						//best_match.animation->player->draw_hitboxes(best_match.animation->bones, 5.f,
						//	config->Colors.lagcompcolor[0] * 255,
						//	config->Colors.lagcompcolor[1] * 255,
						//	config->Colors.lagcompcolor[2] * 255,
						//	config->Colors.lagcompcolor[3] * 255);
				}

			}
		}
	}
}

 bool c_ragebot::IsTickValid(float simtime)
{
	float correct = 0;

	correct += net_channel->get_latency(flow_outgoing);
	correct += net_channel->get_latency(flow_incoming);
	correct += calculate_lerp();

	static convar* sv_maxunlag = cvar()->find_var("sv_maxunlag");
	correct = std::clamp<float>(correct, 0.0f, sv_maxunlag->get_float());

	float deltaTime = correct - (global_vars_base->curtime - simtime);
	float ping = 0.2f; // latency ne budet

	return fabsf(deltaTime) < ping;
}

void c_ragebot::autostop(c_cs_player* local, c_user_cmd* cmd)
{
	if (cmd->buttons & c_user_cmd::jump)
		return;

	static const auto nospread = cvar()->find_var("weapon_accuracy_nospread");

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (nospread->get_int() || !local->is_on_ground() ||
		(weapon && weapon->get_item_definition() == weapon_taser) && local->is_on_ground())
		return;

	const auto wpn_info = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!wpn_info)
		return;

	auto& info = get_autostop_info();

	if (info.call_time == global_vars_base->curtime)
	{
		info.did_stop = true;
		return;
	}

	info.did_stop = false;
	info.call_time = global_vars_base->curtime;

	if (local->get_velocity().length2d() <= wpn_info->get_standing_accuracy(weapon))
		return;
	else
	{

		cmd->forwardmove = 0.f;
		cmd->sidemove = 0.f;

		//prediction_system->repredict(local, cmd);

		if (local->get_velocity().length2d() <= wpn_info->get_standing_accuracy(weapon))
			return;
	}

	c_qangle dir;
	math::vector_angles(c_cs_player::get_local_player()->get_velocity(), dir);
	const auto angles = engine_client()->get_view_angles();
	dir.y = angles.y - dir.y;

	c_vector3d move;
	math::angle_vectors(dir, move);

	if (c_cs_player::get_local_player()->get_velocity().length2d() > .1f)
		move *= -math::forward_bounds / std::max(std::abs(move.x), std::abs(move.y));

	cmd->forwardmove = move.x;
	cmd->sidemove = move.y;

	const auto backup = cmd->viewangles;
	cmd->viewangles = angles;
//prediction_system->repredict(local, cmd);
	cmd->viewangles = backup;

	if (local->get_velocity().length2d() > c_cs_player::get_local_player()->get_velocity().length2d())
	{
		cmd->forwardmove = 0.f;
		cmd->sidemove = 0.f;
	}

//	prediction_system->repredict(local, cmd);
}

std::optional<c_ragebot::aim_info> c_ragebot::scan_record(c_cs_player* local, c_animation_system::animation* animation)
{
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	if (!weapon)return std::nullopt;
	const auto info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!info)return std::nullopt;

	const auto is_zeus = weapon->get_item_definition() == weapon_taser;
	const auto is_knife = !is_zeus && info->WeaponType == weapontype_knife;

	if (is_knife)
		return scan_record_knife(local, animation);

//	return scan_record_gun(local, animation, weapon);
}

std::optional<c_ragebot::aim_info> c_ragebot::scan_record_knife(c_cs_player* local, c_animation_system::animation* animation)
{
	static const auto is_behind = [](c_cs_player* local, c_animation_system::animation* animation) -> bool
	{
		auto vec_los = animation->origin - local->get_origin();
		vec_los.z = 0.f;

		c_vector3d forward;
		math::angle_vectors(c_vector3d(animation->eye_angles.x, antiaim->get_last_real(), animation->eye_angles.x), forward);
		forward.z = 0.f;

		return vec_los.normalize().dot(forward) > 0.475f;
	};

	static const auto should_stab = [](c_cs_player* local, c_animation_system::animation* animation) -> bool
	{
		struct table_t
		{
			unsigned char swing[2][2][2];
			unsigned char stab[2][2];
		};

		static const table_t table = {
			{
				{
					{ 25, 90 },
					{ 21, 76 }
				},
				{
					{ 40, 90 },
					{ 34, 76 }
				}
			},
			{
				{ 65, 180 },
				{ 55, 153 }
			}
		};

		const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
			client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

		if (!weapon)
			return false;

		const auto has_armor = animation->player->get_armor() > 0;
		const auto first_swing = weapon->get_next_primary_attack() + 0.4f < global_vars_base->curtime;
		const auto behind = is_behind(local, animation);

		const int stab_dmg = table.stab[has_armor][behind];
		const int slash_dmg = table.swing[false][has_armor][behind];
		const int swing_dmg = table.swing[first_swing][has_armor][behind];

		if (animation->health <= swing_dmg)
			return false;

		if (animation->health <= stab_dmg)
			return true;

		if (animation->health > swing_dmg + slash_dmg + stab_dmg)
			return true;

		return false;
	};

	const auto studio_model = model_info_client()->get_studio_model(animation->player->get_model());

	if (!studio_model)
		return std::nullopt;

	const auto stab = should_stab(local, animation);
	const auto range = stab ? 32.f : 48.f;
	game_trace tr;
	auto spot = animation->player->get_hitbox_position(c_cs_player::hitbox::upper_chest, animation->bones);
	const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(c_cs_player::hitbox::upper_chest), 0);

	if (!spot.has_value() || !hitbox)
		return std::nullopt;

	c_vector3d forward;
	const auto calc = math::calc_angle(local->get_shoot_position(), spot.value());
	math::angle_vectors(calc, forward);

	spot.value() += forward * hitbox->radius;

	c_trace_system::run_emulated(animation, [&]() -> void
		{
			uint32_t filter[4] = { c_engine_trace::get_filter_simple_vtable(), reinterpret_cast<uint32_t>(local), 0, 0 };

			ray_t r;
			c_vector3d aim;
			const auto angle = math::calc_angle(local->get_shoot_position(), spot.value());
			math::angle_vectors(angle, aim);
			const auto end = local->get_shoot_position() + aim * range;
			r.init(local->get_shoot_position(), end);

			engine_trace()->trace_ray(r, mask_solid, reinterpret_cast<trace_filter*>(filter), &tr);

			if (tr.fraction >= 1.0f)
			{
#undef min
#undef max
				const c_vector3d min(-16.f, -16.f, -18.f);

				const c_vector3d max(16.f, 16.f, 18.f);
				r.Init(local->get_shoot_position(), end, min, max);
				engine_trace()->trace_ray(r, mask_solid, reinterpret_cast<trace_filter*>(filter), &tr);
			}
		});

	if (tr.entity != animation->player || !config->Ragebot.autoknife)
		return std::nullopt;

	return aim_info{ tr.endpos, 100.f, animation, stab, c_vector3d(), 0.f, 0.f, c_cs_player::hitbox::head, 0 };
}

bool c_ragebot::is_breaking_lagcomp(c_animation_system::animation* animation)
{
	static constexpr auto teleport_dist = 64.f * 64.f;

	const auto info = animation_system->get_animation_info(animation->player);

	if (!info || info->frames.size() < 2)
		return false;

	if (info->frames[0].dormant)
		return false;

	auto prev_org = info->frames[0].origin;
	auto skip_first = true;

	// walk context looking for any invalidating event
	for (auto& record : info->frames)
	{
		if (skip_first)
		{
			skip_first = false;
			continue;
		}

		if (record.dormant)
			break;

		auto delta = record.origin - prev_org;
		if (delta.length2dsqr() > teleport_dist)
		{
			// lost track, too much difference
			return true;
		}

		// did we find a context smaller than target time?
		if (record.sim_time <= animation->sim_time)
			break; // hurra, stop

		prev_org = record.origin;
	}

	return false;
}

std::optional<c_ragebot::aim_info> c_ragebot::scan_record_gun(c_cs_player* local, c_animation_system::animation* animation, c_base_combat_weapon* weapon, std::optional<Menu22::ragebot::weapon_config> weapon_cfg, std::optional<c_vector3d> pos)
{
	//const auto weapon_cfg = c_aimhelper::get_weapon_conf();
	//const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon)
		return std::nullopt;

	if (!animation || !animation->player || !weapon_cfg.has_value())
		return std::nullopt;

	const auto info = animation_system->get_animation_info(animation->player);

	if (!info)
		return std::nullopt;

	const auto cfg = weapon_cfg.value();
	c_vector3d shoot_pos = local->get_shoot_position();

	auto should_baim = true;
	const auto center = animation->player->get_hitbox_position(c_cs_player::hitbox::head, animation->bones);
	if (center.has_value())
	{
		const auto center_wall = trace_system->wall_penetration(pos.value_or(shoot_pos),
			center.value(), animation);

		if (center_wall.has_value() && center_wall.value().hitbox == c_cs_player::hitbox::head
			&& center_wall.value().damage > animation->health - 20)
			should_baim = false;
	}

	aim_info best_match = { c_vector3d(), -FLT_MAX, nullptr, false, c_vector3d(), 0.f, 0.f, c_cs_player::hitbox::head, 0 };

	const auto scan_box = [&](c_cs_player::hitbox hitbox)
	{
		auto box = animation->player->get_hitbox_position(hitbox, const_cast<matrix3x4*>(animation->bones));
		if (!box.has_value())
			return;

		//auto points = pos.has_value() ?
		//std::vector<aim_info>() : c_aimhelper::select_multipoint(animation, hitbox, hitgroup_head, cfg);  //c_aimhelper::GetMultiplePointsForHitbox(animation->player, hitbox, animation);

		auto points = std::vector<aim_info>();
		points = c_aimhelper::select_multipoint(animation, hitbox, hitgroup_head, cfg);

		for (auto& point : points)
		{
			const auto wall = trace_system->wall_penetration(pos.value_or(shoot_pos),
				point.position, animation);

			if (!wall.has_value())
				continue;

			if (hitbox == c_cs_player::hitbox::head && hitbox != wall.value().hitbox)
				continue;

			point.hitgroup = wall.value().hitgroup;

			if (hitbox == c_cs_player::hitbox::upper_chest
				&& (wall.value().hitbox == c_cs_player::hitbox::head || wall.value().hitbox == c_cs_player::hitbox::neck))
				continue;

			point.damage = wall.value().damage;

			if (point.damage > best_match.damage)
				best_match = point;
		}
	};

	const auto data = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!data)
		return std::nullopt;

	if (animation->health <= cfg.body_aim_health)
		should_baim = true;

	if (cfg.body_aim_in_air)
		if (animation->velocity.length2d() > 0.1 && !(animation->flags & c_base_player::flags::on_ground)/*!animation->player->is_on_ground()*/)
			should_baim = true;

	if (animation->velocity.length2d() > 200 && animation->flags & c_base_player::flags::on_ground)
		should_baim = true;

	if (cfg.body_aim_lethal && animation->health < data->iDamage && animation->health <= 50)
		should_baim = true;

	if (GetAsyncKeyState(cfg.body_aim_key) && cfg.body_aim_key_enable)
		should_baim = true;

	if (cfg.body_aim_if_not_on_shot && !animation->is_backtrackable)
		should_baim = true;

	std::vector<c_cs_player::hitbox> hitboxes_scan;

	if (!weapon_cfg->hitscan_chest && !weapon_cfg->hitscan_stomach && !weapon_cfg->hitscan_pelvis && !weapon_cfg->hitscan_legs && !weapon_cfg->hitscan_feet)
		should_baim = false;

	auto strip_hitboxes =
		resolver.missed_due_to_bad_resolve[animation->index] >= 2;

	if (unit::force_safepoint)
		should_baim = true;

	if (animation->is_backtrackable)
	{
		if (resolver.missed_due_to_bad_resolve[animation->index] <= 2)
		{
			should_baim = false;
			strip_hitboxes = false;
		}
	}

	if (cfg.hitbox_override != 0 && GetAsyncKeyState(cfg.hitbox_override_key))
	{
		switch (cfg.hitbox_override) {
			case 1: {
				hitboxes_scan.push_back(c_cs_player::hitbox::head);
				//hitboxes_scan.push_back(c_cs_player::hitbox::neck);
			} break;
			case 2: {
				hitboxes_scan.push_back(c_cs_player::hitbox::upper_chest);
				hitboxes_scan.push_back(c_cs_player::hitbox::chest);
			} break;
			case 3: {
				hitboxes_scan.push_back(c_cs_player::hitbox::body);
				hitboxes_scan.push_back(c_cs_player::hitbox::thorax);
			} break;
			case 4: {
				hitboxes_scan.push_back(c_cs_player::hitbox::left_calf);
				hitboxes_scan.push_back(c_cs_player::hitbox::right_calf);
				hitboxes_scan.push_back(c_cs_player::hitbox::left_thigh);
				hitboxes_scan.push_back(c_cs_player::hitbox::right_thigh);
			} break;
			case 5: {
				hitboxes_scan.push_back(c_cs_player::hitbox::left_foot);
				hitboxes_scan.push_back(c_cs_player::hitbox::right_foot);
			} break;
		}
	}
	else if (should_baim)
	{
		if (weapon_cfg->hitscan_pelvis)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::pelvis);
		}
		if (weapon_cfg->hitscan_chest)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::chest);
			hitboxes_scan.push_back(c_cs_player::hitbox::upper_chest);
		}
		if (weapon_cfg->hitscan_stomach)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::body);
			//hitboxes_scan.push_back(c_cs_player::hitbox::thorax);
		}
	}
	else
	{
		if (weapon_cfg->hitscan_head && weapon_cfg->hitbox_prefer == 0)
			hitboxes_scan.push_back(c_cs_player::hitbox::head);
			//hitboxes_scan.push_back(c_cs_player::hitbox::neck);
		if (weapon_cfg->hitscan_chest)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::upper_chest);
			hitboxes_scan.push_back(c_cs_player::hitbox::chest);
		}
		if (weapon_cfg->hitscan_stomach)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::body);
			hitboxes_scan.push_back(c_cs_player::hitbox::thorax);
		}
		if (weapon_cfg->hitscan_pelvis)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::pelvis);
		}
		if (weapon_cfg->hitscan_legs)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::left_calf);
			hitboxes_scan.push_back(c_cs_player::hitbox::right_calf);
			hitboxes_scan.push_back(c_cs_player::hitbox::left_thigh);
			hitboxes_scan.push_back(c_cs_player::hitbox::right_thigh);
		}
		if (weapon_cfg->hitscan_feet)
		{
			hitboxes_scan.push_back(c_cs_player::hitbox::left_foot);
			hitboxes_scan.push_back(c_cs_player::hitbox::right_foot);
		}
	}
	for (const auto& hitbox : hitboxes_scan)
		scan_box(hitbox);

	if (GetKeyState(config->Misc.damage_override) && best_match.damage >= 5)
		return best_match;
	else if ((best_match.damage >= cfg.min_dmg) || (best_match.damage >= animation->health && animation->health > 0))
		return best_match;

	return std::nullopt;
}

c_ragebot::autostop_info& c_ragebot::get_autostop_info()
{
	static autostop_info info{ -FLT_MAX, false };
	return info;
}

std::optional<c_ragebot::aim_info> c_ragebot::simple_backtrack_record_gun(c_cs_player* local, c_animation_system::animation* animation, std::optional<c_vector3d> pos)
{
	const auto weapon_cfg = c_aimhelper::get_weapon_conf();
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon)
		return std::nullopt;

	if (!animation || !animation->player || !weapon_cfg.has_value())
		return std::nullopt;

	const auto cfg = weapon_cfg.value();
	aim_info best_match = { c_vector3d(), -FLT_MAX, nullptr, false, c_vector3d(), 0.f, 0.f, c_cs_player::hitbox::head, 0 };

	const auto scan_box = [&](c_cs_player::hitbox hitbox)
	{
		auto box = animation->player->get_hitbox_position(hitbox, const_cast<matrix3x4*>(animation->bones));
		if (!box.has_value())
			return;

		auto points = std::vector<aim_info>();
		points = c_aimhelper::select_multipoint(animation, hitbox, hitgroup_head, cfg);

		for (auto& point : points)
		{
			const auto wall = trace_system->wall_penetration(pos.value_or(local->get_shoot_position()),
				point.position, animation);

			if (!wall.has_value())
				continue;

			if (hitbox == c_cs_player::hitbox::head && hitbox != wall.value().hitbox)
				continue;

			point.hitgroup = wall.value().hitgroup;

			if (hitbox == c_cs_player::hitbox::upper_chest
				&& (wall.value().hitbox == c_cs_player::hitbox::head || wall.value().hitbox == c_cs_player::hitbox::neck))
				continue;

			point.damage = wall.value().damage;

			if (point.damage > best_match.damage)
				best_match = point;
		}
	};

	const auto data = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!data)
		return std::nullopt;

	std::vector<c_cs_player::hitbox> hitboxes_scan;

	if (weapon_cfg->hitscan_stomach)
		hitboxes_scan.push_back(c_cs_player::hitbox::body);

	if (weapon_cfg->hitscan_pelvis)
		hitboxes_scan.push_back(c_cs_player::hitbox::pelvis);

	for (const auto& hitbox : hitboxes_scan)
		scan_box(hitbox);

	if (GetKeyState(config->Misc.damage_override) && best_match.damage >= 5)
		return best_match;
	else
	if (best_match.damage >= cfg.min_dmg || best_match.damage >= animation->health)
		return best_match;

	return std::nullopt;
}

// Experimental function by Solpadoin. DON'T USE THIS FOR RAGEBOT PURPOSES! YOU MAY GOT CRASH!
std::optional<c_ragebot::aim_info> c_ragebot::scan_record_lag_peek(c_cs_player* local, c_animation_system::animation* animation, std::optional<c_vector3d> shoot_pos)
{
	const auto weapon_cfg = c_aimhelper::get_weapon_conf();
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon)
		return std::nullopt;

	if (!animation || !animation->player || !weapon_cfg.has_value())
		return std::nullopt;

	const auto info = animation_system->get_animation_info(animation->player);

	if (!info)
		return std::nullopt;

	const auto cfg = weapon_cfg.value();

	//c_vector3d shoot_pos = local->get_shoot_position();
	c_vector3d enemy_shoot_pos = animation->player->get_shoot_position();

	aim_info best_match = { c_vector3d(), -FLT_MAX, nullptr, false, c_vector3d(), 0.f, 0.f, c_cs_player::hitbox::head, 0 };

	const auto scan_box = [&]()
	{
		const auto record = animation_system->get_latest_animation(local);

		if (!record.has_value())
			return;

		const auto wall = /*trace_system->wall_penetration(shoot_pos.value_or(local->get_shoot_position()),
			enemy_shoot_pos, animation); */// scan autowall from local to enemy player

		trace_system->wall_penetration(enemy_shoot_pos,
			shoot_pos.value_or(local->get_shoot_position()), record.value(), nullptr, animation->player); // scan autowall from enemy to local player

		if (!wall.has_value())
		{
			const auto wall2 = /*trace_system->wall_penetration(enemy_shoot_pos,
				shoot_pos.value_or(local->get_shoot_position()), record.value(), animation->player); */// scan autowall from enemy to local player
				trace_system->wall_penetration(shoot_pos.value_or(local->get_shoot_position()),
					enemy_shoot_pos, animation);

			if (!wall2.has_value())
				return;

			best_match.position = shoot_pos.value_or(local->get_shoot_position());
			best_match.damage = wall2.value().damage;

			return; // to do.. improve this shit to extended final version: scan local left, right hands for damage and buttons left_move, right_move...
		}

		best_match.position = enemy_shoot_pos;
		best_match.damage = wall.value().damage;
	};

	scan_box();

	if (best_match.damage >= 1)
		return best_match;

	return std::nullopt;
}
