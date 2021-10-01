#include "c_animation_system.h"
#include "c_trace_system.h"
#include "c_antiaim.h"
#include "../sdk/c_global_vars_base.h"
#include "../sdk/c_prediction.h"
#include "../sdk/c_client_entity_list.h"
#include "../sdk/c_cvar.h"
#include "../utils/math.h"
#include "c_resolver.h"
#include "c_esp.h"
#include "../sdk/c_input.h"
#include "../hooks/c_prediction_.h"

c_animation_system::animation::animation(c_cs_player* player)
{
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(player->get_current_weapon_handle()));

	this->player = player;
	index = player->index();
	dormant = player->is_dormant();
	abs_velocity = player->get_abs_velocity();
	velocity = player->get_velocity();
	origin = player->get_origin();
	abs_origin = player->get_abs_origin();
	obb_mins = player->get_mins();
	obb_maxs = player->get_maxs();
	layers = *player->get_animation_layers();
	poses = player->get_pose_parameter();
	if ((has_anim_state = player->get_anim_state()))
		anim_state = *player->get_anim_state();

	anim_time = player->get_old_simtime() + global_vars_base->interval_per_tick;
	sim_time = player->get_simtime();
	//sim_old_time = player->get_old_simtime();
	interp_time = 0.f;
	last_shot_time = weapon ? weapon->get_last_shot_time() : 0.f;
	duck = player->get_duck_amount();
	lby = player->get_lby();
	eye_angles = player->get_eye_angles();
	abs_ang = player->get_abs_angles();
	flags = player->get_flags();
	eflags = player->get_eflags();
	effects = player->get_effects();
	lag = time_to_ticks(player->get_simtime() - player->get_old_simtime());
	health = player->get_health();

	// animations are off when we enter pvs, we do not want to shoot yet.
	valid = lag >= 0 && lag <= 17;

	// clamp it so we don't interpolate too far : )
	lag = std::clamp(lag, 0, 17);
}

c_animation_system::animation::animation(c_cs_player* player, c_qangle last_reliable_angle) : animation(player)
{
	this->last_reliable_angle = last_reliable_angle;
}

void c_animation_system::animation::restore(c_cs_player* player) const
{
	player->get_velocity() = velocity;
	player->get_abs_velocity() = abs_velocity;
	player->get_flags() = flags;
	player->get_eflags() = eflags;
	player->get_duck_amount() = duck;
	*player->get_animation_layers() = layers;
	player->get_lby() = lby;
	player->get_origin() = origin;
	player->set_abs_origin(abs_origin);
}

void c_animation_system::animation::apply(c_cs_player* player) const
{
	player->get_pose_parameter() = poses;
	player->get_eye_angles() = eye_angles;
	player->get_velocity() = player->get_abs_velocity() = velocity;
	player->get_lby() = lby;
	player->get_duck_amount() = duck;
	player->get_flags() = flags;
	player->get_origin() = origin;
	player->set_abs_origin(origin);
	if (player->get_anim_state() && has_anim_state)
		*player->get_anim_state() = anim_state;
}

void c_animation_system::animation::build_server_bones(c_cs_player* player)
{
	// keep track of old occlusion values
	const auto backup_occlusion_flags = player->get_occlusion_flags();
	const auto backup_occlusion_framecount = player->get_occlusion_framecount();

	// skip occlusion checks in c_cs_player::setup_bones
	if (!player->is_local_player())
	{
		player->get_occlusion_flags() = 0;
		player->get_occlusion_framecount() = 0;
	}

	// clear bone accessor
	player->get_readable_bones() = player->get_writable_bones() = 0;

	// invalidate bone cache
	player->get_most_recent_model_bone_counter() = 0;
	player->get_last_bone_setup_time() = -FLT_MAX;

	// stop interpolation
	player->get_effects() |= c_base_entity::ef_nointerp;

	// build bones
	player->setup_bones(nullptr, -1, bone_used_by_anything, global_vars_base->curtime);

	// restore original occlusion
	if (!player->is_local_player())
	{
		player->get_occlusion_flags() = backup_occlusion_flags;
		player->get_occlusion_framecount() = backup_occlusion_framecount;
	}

	// start interpolation again
	player->get_effects() &= ~c_base_entity::ef_nointerp;

	// save bones.
	const auto bone_matrix = player->get_bone_array_for_write();
	memcpy(bones, bone_matrix, player->get_bone_cache_count() * sizeof(float) * 12);
}

void c_animation_system::animation_info::update_animations(animation* record, animation* from)
{
	// really basic detection.
	const auto is_cheater = record->lag > 1;

	if (!from)
	{
		// set velocity and layers.
		record->velocity = player->get_velocity();

		// fix feet spin.
		record->anim_state.feet_yaw_rate = 0.f;

		// resolve player.
		c_resolver::resolve(record);

		// apply record.
		record->apply(player);

		// run update.
		instance()->update_player(player);

		return;
	}

	// backup player data //

	const auto backup_eye_angles = player->get_eye_angles();
	const auto backup_player_origin = player->get_origin();
	const auto backup_player_flags = player->get_flags();
	const auto backup_player_velocity = player->get_velocity();
	const auto backup_player_abs_angles = player->get_abs_angles();
	const auto backup_player_duck_ammount = player->get_duck_amount();
	const auto backup_player_animstate = record->anim_state;
	const auto backup_player_animstate_origin = player->get_anim_state();

	// restore previous state.
	*player->get_animation_layers() = from->layers;
	player->set_abs_origin(record->origin);
	player->set_abs_angles(from->abs_ang);

	// setup extrapolation parameters.
	auto old_origin = from->origin;
	auto old_flags =  from->flags;

	// did the player shoot?
	const auto shot = record->is_backtrackable =
		record->last_shot_time > from->sim_time && record->last_shot_time <= record->sim_time;

	for (auto i = 0; i < record->lag; i++)
	{
		// move time forward.
		const auto time = from->sim_time + ticks_to_time(i + 1);
		const auto lerp = 1.f - (record->sim_time - time) / (record->sim_time - from->sim_time);

		if (record->lag > 1)
		{
			// lerp eye angles.
			auto eye_angles = math::interpolate(from->eye_angles, record->eye_angles, lerp);
			math::normalize(eye_angles);
			player->get_eye_angles() = eye_angles;

			// lerp duck amount.
			player->get_duck_amount() = math::interpolate(from->duck, record->duck, lerp);


			// lerp velocity.
			player->get_velocity() = player->get_abs_velocity() =
				math::interpolate(from->velocity, record->velocity, lerp);
		}

		// resolve player.
		if (record->lag - 1 == i)
		{
			if (record->flags & c_base_player::flags::on_ground)
			{
				record->anim_state.on_ground = true;  // fix on_ground broken anim, caused by fakelag
				record->anim_state.in_hit_ground_animation = false; // fix on_ground broken anim, caused by fakelag
			}
	
			player->set_abs_origin(record->origin);
			player->set_abs_angles(from->abs_ang);


			player->get_eye_angles() = record->eye_angles;


			if ( is_cheater )
			   c_resolver::resolve(record);
		}
		else
		{
			//if (i == 0)
			//{
				//player->set_abs_origin(from->origin); // testing
			//}
			//else
			//{
				auto origin_interp = math::interpolate(from->origin, record->origin, lerp);
				auto abs_interp =    math::interpolate(from->abs_ang, record->abs_ang, lerp);

				player->set_abs_origin(origin_interp);
				player->set_abs_angles(abs_interp);
			//}
		}

		// correct shot desync.
		if (shot)
		{
			player->get_eye_angles() = record->last_reliable_angle;

			if (record->last_shot_time <= time)
				player->get_eye_angles() = record->eye_angles;
		}

		// fix feet spin.
		player->get_anim_state()->feet_yaw_rate = 0.f;

		// backup simtime.
		const auto backup_simtime = player->get_simtime();

		// set new simtime.
		player->get_simtime() = time;

		// run update.
		instance()->update_player(player);

		// restore old simtime.
		player->get_simtime() = backup_simtime;
	}

	// restore player data //

	/*
	player->get_eye_angles() = backup_eye_angles;
	player->get_origin() = backup_player_origin;
	player->get_flags() = backup_player_flags;
	player->get_velocity() = backup_player_velocity;
	player->get_abs_angles() = backup_player_abs_angles;
	player->get_duck_amount() = backup_player_duck_ammount;
	record->anim_state = backup_player_animstate;
	player->get_anim_state() = backup_player_animstate_origin;

	player->set_abs_origin(backup_player_origin);
	player->set_abs_angles(backup_player_abs_angles);
	*/
}

void c_animation_system::post_player_update()
{
	if (!engine_client()->is_ingame() && engine_client()->is_connected())
		return;

	const auto local = c_cs_player::get_local_player();

	// erase outdated entries
	for (auto it = animation_infos.begin(); it != animation_infos.end();) {
		auto player = reinterpret_cast<c_cs_player*>(client_entity_list()->get_client_entity_from_handle(it->first));

		if (!player || player != it->second.player || !player->is_alive() || !player->is_enemy()
			|| !local)
		{
			if (player)
				player->get_client_side_animation() = true;
			it = animation_infos.erase(it);
		}
		else
			it = next(it);
	}

	if (!local)
	{
		client_entity_list()->for_each_player([](c_cs_player* player) -> void
		{
			player->get_client_side_animation() = true;
		});
		return;
	}

	// create new entries and reset old ones
	client_entity_list()->for_each_player([&](c_cs_player* player) -> void {
		if (!player->is_enemy() && !player->is_local_player())
			player->get_client_side_animation() = true;

		if (!player->is_alive() || player->is_dormant() || player->is_local_player() || !player->is_enemy())
			return;

		if (animation_infos.find(player->get_handle()) == animation_infos.end())
			animation_infos.insert_or_assign(player->get_handle(), animation_info(player, {}));
	});

	// run post update
	for (auto& info : animation_infos)
	{
		auto& animation = info.second;
		const auto player = animation.player;

		// erase frames out-of-range
		for (auto it = animation.frames.rbegin(); it != animation.frames.rend();) {
			if (global_vars_base->curtime - it->sim_time > 1.2f)
				it = decltype(it) { info.second.frames.erase(next(it).base()) };
			else
				it = next(it);
		}

		// we need to reset animstate, because game will only reset when player spawned in pvs.
		if (animation.last_spawn_time != player->get_spawn_time())
		{
			const auto state = player->get_anim_state();
			if (state)
			{
				state->reset();
				state->last_client_side_animation_update_time = global_vars_base->curtime - global_vars_base->interval_per_tick;
				player->get_velocity() = player->get_abs_velocity() = c_vector3d(0.f, 0.f, 0.f);
				animation.last_spawn_time = player->get_spawn_time();
			}
			continue;
		}

		// have we already seen this update?
		bool update = player->get_simtime() > player->get_old_simtime();

		if (!update)
			continue;

		// grab weapon
		const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
			client_entity_list()->get_client_entity_from_handle(player->get_current_weapon_handle()));

		// make a full backup of the player
		auto backup = c_animation_system::animation(player);
		backup.apply(player);

		// grab previous
		c_animation_system::animation* previous = nullptr;

		if (!animation.frames.empty() && !animation.frames.front().dormant
			&& time_to_ticks(player->get_simtime() - animation.frames.front().sim_time) <= 17)
			previous = &animation.frames.front();

		// update shot info
		const auto shot = weapon && previous &&
			weapon->get_last_shot_time() != previous->last_shot_time;

		if (!shot)
			info.second.last_reliable_angle = player->get_eye_angles();

		// store server record
		auto& record = animation.frames.emplace_front(player, info.second.last_reliable_angle);

		// run full update
		animation.update_animations(&record, previous);

		// restore server layers
		*player->get_animation_layers() = backup.layers;

		// use uninterpolated data to generate our bone matrix
		record.build_server_bones(player);

		// restore correctly synced values
		backup.restore(player);

		// set record to latest animation
		animation.latest_animation = record;
	}
}

void c_animation_system::update_player(c_cs_player* player)
{
	static auto& enable_bone_cache_invalidation = **reinterpret_cast<bool**>(
		reinterpret_cast<uint32_t>(sig("client.dll", "C6 05 ? ? ? ? ? 89 47 70")) + 2);

	// make a backup of globals
	const auto backup_frametime = global_vars_base->frametime;
	const auto backup_curtime =   global_vars_base->curtime;

	const auto backup_tickcount =  global_vars_base->tickcount;
	const auto backup_framecount = global_vars_base->framecount;
	const auto backup_intrpamnt =  global_vars_base->interpolation_amount;
	const auto backup_realtime =   global_vars_base->realtime;
	const auto backup_absfrtime =  global_vars_base->absoluteframetime;

	float v4 = player->get_simtime() / global_vars_base->interval_per_tick;
	float v5 = (v4 + 0.5);

	// get player anim state
	const auto state = player->get_anim_state();

	// fixes for networked players
	if (!player->is_local_player())
	{
		global_vars_base->frametime = global_vars_base->interval_per_tick;
		global_vars_base->absoluteframetime = global_vars_base->interval_per_tick;

		global_vars_base->framecount = v5;
		global_vars_base->tickcount = v5;

		global_vars_base->interpolation_amount = 0.0f;

		global_vars_base->curtime = player->get_simtime();
		global_vars_base->realtime = player->get_simtime();

		player->get_eflags() &= ~c_base_entity::efl_dirty_absvelocity;
	}

	// allow reanimating in the same frame
	if (state->last_client_side_animation_update_framecount == global_vars_base->framecount)
		state->last_client_side_animation_update_framecount -= 1;

	// make sure we keep track of the original invalidation state
	const auto old_invalidation = enable_bone_cache_invalidation;

	// notify the other hooks to instruct animations and pvs fix

	helper::g_pViolanes->g_bOverrideModifier = true;
	instance()->enable_bones = player->get_client_side_animation() = true;
	player->update_clientside_anim();
	instance()->enable_bones = false;

	if (!player->is_local_player())
		player->get_client_side_animation() = false;
	helper::g_pViolanes->g_bOverrideModifier = false;

	// invalidate physics.
	if (!player->is_local_player())
		player->invalidate_physics_recursive(c_base_entity::angles_changed
			| c_base_entity::animation_changed
			| c_base_entity::sequence_changed);

	// we don't want to enable cache invalidation by accident
	enable_bone_cache_invalidation = old_invalidation;

	// restore globals
	global_vars_base->frametime = backup_frametime;
	global_vars_base->curtime =   backup_curtime;
	global_vars_base->absoluteframetime = backup_absfrtime;
	global_vars_base->framecount = backup_framecount;
	global_vars_base->tickcount =  backup_tickcount;
	global_vars_base->interpolation_amount = backup_intrpamnt;
	global_vars_base->realtime = backup_realtime;
}

c_animation_system::animation_info* c_animation_system::get_animation_info(c_cs_player* player)
{
	auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end())
		return nullptr;

	return &info->second;
}

bool c_animation_system::animation::is_valid(float range, float max_unlag)
{
	if (!net_channel || !valid)
		return false;

	max_unlag = .2f;

	static auto sv_maxunlag = cvar()->find_var("sv_maxunlag");
	if (sv_maxunlag)
		max_unlag = sv_maxunlag->get_float();

	const auto correct = std::clamp(net_channel->get_latency(flow_incoming)
		+ net_channel->get_latency(flow_outgoing)
		+ calculate_lerp(), 0.f, max_unlag);

	return fabsf(correct - (global_vars_base->curtime - sim_time)) <= range;
}

std::optional<c_animation_system::animation*> c_animation_system::get_latest_animation(c_cs_player* player)
{
	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end() || info->second.frames.empty())
		return std::nullopt;

	for (auto it = info->second.frames.begin(); it != info->second.frames.end(); it = next(it))
		if (it->is_valid())
			return &*it;

	return std::nullopt;
}

std::optional<c_animation_system::animation*> c_animation_system::get_oldest_animation(c_cs_player* player)
{
	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end() || info->second.frames.empty())
		return std::nullopt;

	for (auto it = info->second.frames.rbegin(); it != info->second.frames.rend(); it = next(it))
		if (it->is_valid())
			return &*it;

	return std::nullopt;
}

std::optional<c_animation_system::animation*> c_animation_system::get_intermediate_anim(c_cs_player* player)
{
	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end() || info->second.frames.empty())
		return std::nullopt;

	for (auto it = info->second.frames.rbegin(); it != info->second.frames.rend(); it = next(it))
		if (it->is_valid(0.1))
			return &*it;

	return std::nullopt;
}

std::optional<c_animation_system::animation*> c_animation_system::get_custom_backtrack(c_cs_player* player, float time/* = 0.05*/)
{
	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end() || info->second.frames.empty())
		return std::nullopt;

	for (auto it = info->second.frames.rbegin(); it != info->second.frames.rend(); it = next(it))
		if (it->is_valid(time))
			return &*it;

	return std::nullopt;
}

std::optional<c_animation_system::animation*> c_animation_system::get_uncrouched_animation(c_cs_player* player)
{
	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end() || info->second.frames.empty())
		return std::nullopt;

	for (auto it = info->second.frames.begin(); it != info->second.frames.end(); it = next(it))
		if (it->is_valid() && fabsf(it->duck) < .0001f)
			return &*it;

	return std::nullopt;
}

std::optional<std::pair<c_animation_system::animation*, c_animation_system::animation*>> c_animation_system::get_intermediate_animations(
	c_cs_player* player, const float range)
{
	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end() || info->second.frames.empty())
		return std::nullopt;

	for (auto it = info->second.frames.begin(); it != info->second.frames.end(); it = next(it))
		if (it->is_valid(range * .2f) && it + 1 != info->second.frames.end() && !(it + 1)->is_valid(range * .2f))
			return std::make_pair(&*(it + 1), &*it);

	return std::nullopt;
}

std::vector<c_animation_system::animation*> c_animation_system::get_valid_animations(c_cs_player* player, const float range)
{
	std::vector<animation*> result;

	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end() || info->second.frames.empty())
		return result;

	result.reserve(static_cast<int>(std::ceil(range * .2f / global_vars_base->interval_per_tick)));

	for (auto it = info->second.frames.begin(); it != info->second.frames.end(); it = next(it))
		if (it->is_valid(range * .2f))
			result.push_back(&*it);

	return result;
}

std::optional<c_animation_system::animation*> c_animation_system::get_lastest_animation_unsafe(c_cs_player* player)
{
	const auto latest = get_latest_animation(player);

	if (latest.has_value())
		return latest;

	const auto info = animation_infos.find(player->get_handle());

	if (info == animation_infos.end())
		return std::nullopt;

	return &info->second.latest_animation;
}