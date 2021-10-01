#include "c_aimhelper.h"
#include "../sdk/c_cs_player.h"
#include "../sdk/c_client_entity_list.h"
#include "c_animation_system.h"
#include "../utils/math.h"
#include "../sdk/c_weapon_system.h"
#include "c_trace_system.h"
#include "../menu/c_menu.h"
#include "c_prediction_system.h"
#include "../sdk/c_engine_trace.h"
#include "../idaboss.h"

static std::vector<std::tuple<float, float, float>> precomputed_seeds = {};

std::optional<Menu22::ragebot::weapon_config> c_aimhelper::get_weapon_conf()
{
	if (!engine_client()->is_ingame() || !engine_client()->is_connected() || !c_cs_player::get_local_player() || !c_cs_player::get_local_player()->is_alive())
		return std::nullopt;

	const auto local = c_cs_player::get_local_player();

	if (!local)
		return std::nullopt;

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon)
		return std::nullopt;

	const auto info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!info)
		return std::nullopt;

	if (info->get_weapon_id() == weapon_g3sg1 || info->get_weapon_id() == weapon_scar20) {
		if (!menu->open)config->Ragebot.weaponconfig = 1;
		return config->Ragebot._auto;
	}
	else if (info->get_weapon_id() == weapon_ssg08) {
		if (!menu->open)config->Ragebot.weaponconfig = 2;
		return config->Ragebot.scout;
	}
	else if (info->get_weapon_id() == weapon_awp) {
		if (!menu->open)config->Ragebot.weaponconfig = 3;
		return config->Ragebot.awp;
	}
	else if (info->get_weapon_id() == weapon_deagle || info->get_weapon_id() == weapon_revolver) {
		if (!menu->open)config->Ragebot.weaponconfig = 4;
		return config->Ragebot.heavy;
	}
	else if (info->WeaponType == weapontype_pistol) {
		if (!menu->open)config->Ragebot.weaponconfig = 0;
		return config->Ragebot.pistol;
	}
	else if (info->get_weapon_id() == weapon_taser) {
		if (!menu->open)config->Ragebot.weaponconfig = 6;
		return config->Ragebot.taser;
	}
	else {
		if (!menu->open)config->Ragebot.weaponconfig = 5;
		return config->Ragebot.other;
	}
}

std::optional<std::tuple<c_vector3d, float, c_animation_system::animation*, float>> c_aimhelper::get_legit_target(float target_fov, float range, std::vector<c_cs_player::hitbox> override_hitbox, bool visible)
{
	auto local = c_cs_player::get_local_player();

	if (!local)
		return std::nullopt;

	auto current_pos = local->get_shoot_position();
	auto current_angle = engine_client()->get_view_angles();
	const auto recoil_scale = cvar()->find_var("weapon_recoil_scale")->get_float();
	auto current_angle_with_recoil = current_angle + (local->get_punch_angle() * recoil_scale);
	math::normalize(current_angle_with_recoil);
	std::tuple<c_vector3d, c_animation_system::animation*, float> best_match = { c_vector3d(), nullptr, FLT_MAX };
	std::tuple<c_vector3d, c_animation_system::animation*, float, float> best_match1 = { c_vector3d(), nullptr, FLT_MAX, FLT_MAX };

	client_entity_list()->for_each_player([&](c_cs_player* player) -> void
		{
			if (!player->is_alive() || player->is_dormant() /*|| (visible && player->is_dormant())*/ || !player->is_enemy())
				return;//look at the visible shit

			//if (!visible && player->get_simtime() + 7.f < global_vars_base->curtime)
				//return;//look at the visible shit

			auto view = player->get_shoot_position();
			const auto estimate_fov = math::get_fov(current_angle_with_recoil, current_pos, view);

			if (estimate_fov > 2.f * target_fov)//cool check but rather useless
				return;

			const auto current = animation_system->get_lastest_animation_unsafe(player);
			if (!current)return;//dunno why that would happen
			/*if (!current.has_value() ||
				(visible && !player->is_visible(const_cast<matrix3x4*>(current.value()->bones))))
				return;*/

			const auto animations = animation_system->get_valid_animations(player, 1.f); //range > 0.f ?  : std::vector<c_animation_system::animation*> { current.value() };

			/*find closest hitbox/record*/
			for (auto& animation : animations) {
				for (const auto& hitbox : override_hitbox) {
					auto box = player->get_hitbox_position(hitbox, const_cast<matrix3x4*>(animation->bones));
					if (!box.has_value())
						continue;

					const auto fov = math::get_fov(current_angle_with_recoil, current_pos, box.value());
					if (fov <= target_fov)
						if (fov < std::get<2>(best_match1))
							best_match1 = std::make_tuple(box.value(), animation, fov, 0);
				}
			}
			/*damage check*/
			if (std::get<2>(best_match1) != FLT_MAX) {//we have something useful
				if (std::get<1>(best_match1)->index == player->index()) {//current scan has the closest
					const auto wall = trace_system->wall_penetration(local->get_shoot_position(),
						std::get<0>(best_match1), std::get<1>(best_match1));

					if (wall.has_value()) {
						if (wall.value().damage > std::get<3>(best_match1)) {
							std::get<3>(best_match1) = wall.value().damage;
							best_match = std::make_tuple(std::get<0>(best_match1), std::get<1>(best_match1), std::get<2>(best_match1));
						}
					}
				}
			}
		});

	if (std::get<2>(best_match) == FLT_MAX || !std::get<1>(best_match) || !std::get<1>(best_match)->player)
		return std::nullopt;
	std::optional<std::tuple<c_vector3d, float, c_animation_system::animation*, float>> target = std::nullopt;
	auto t = std::make_tuple(std::get<0>(best_match), std::get<1>(best_match)->sim_time, std::get<1>(best_match), std::get<2>(best_match));
	target = t;
	return std::make_tuple(std::get<0>(best_match), std::get<1>(best_match)->sim_time, std::get<1>(best_match), std::get<2>(best_match));
}

std::optional<std::tuple<c_vector3d, float, c_animation_system::animation*, float>> c_aimhelper::get_legit_target(float target_fov, float range, std::vector<c_cs_player::hitbox> override_hitbox)
{
	auto local = c_cs_player::get_local_player();

	if (!local)
		return std::nullopt;

	auto current_pos = local->get_shoot_position();
	auto current_angle = engine_client()->get_view_angles();
	const auto recoil_scale = cvar()->find_var("weapon_recoil_scale")->get_float();
	auto current_angle_with_recoil = current_angle + (local->get_punch_angle() * recoil_scale);
	math::normalize(current_angle_with_recoil);
	std::tuple<c_vector3d, c_animation_system::animation*, float> best_match = { c_vector3d(), nullptr, FLT_MAX };
	std::tuple<c_vector3d, c_animation_system::animation*, float, float> best_match1 = { c_vector3d(), nullptr, FLT_MAX, FLT_MAX };

	client_entity_list()->for_each_player([&](c_cs_player* player) -> void
		{
			if (!player->is_alive() || player->is_dormant() /*|| (visible && player->is_dormant())*/ || !player->is_enemy())
				return;//look at the visible shit

			//if (!visible && player->get_simtime() + 7.f < global_vars_base->curtime)
				//return;//look at the visible shit

			auto view = player->get_shoot_position();
			const auto estimate_fov = math::get_fov(current_angle_with_recoil, current_pos, view);

			if (estimate_fov > 2.f * target_fov)//cool check but rather useless
				return;

			const auto current = animation_system->get_lastest_animation_unsafe(player);
			if (!current)return;//dunno why that would happen
			/*if (!current.has_value() ||
				(visible && !player->is_visible(const_cast<matrix3x4*>(current.value()->bones))))
				return;*/

			const auto animations = animation_system->get_valid_animations(player, 1.f); //range > 0.f ?  : std::vector<c_animation_system::animation*> { current.value() };

			/*find closest hitbox/record*/
			for (auto& animation : animations) {
				for (const auto& hitbox : override_hitbox) {
					auto box = player->get_hitbox_position(hitbox, const_cast<matrix3x4*>(animation->bones));
					if (!box.has_value())
						continue;

					const auto fov = math::get_fov(current_angle_with_recoil, current_pos, box.value());
					if (fov <= target_fov)
						if (fov < std::get<2>(best_match1))
							best_match1 = std::make_tuple(box.value(), animation, fov, 0);
				}
			}
			/*damage check*/
			if (std::get<2>(best_match1) != FLT_MAX) {//we have something useful
				if (std::get<1>(best_match1)->index == player->index()) {//current scan has the closest
					const auto wall = trace_system->wall_penetration(local->get_shoot_position(),
						std::get<0>(best_match1), std::get<1>(best_match1));

					if (wall.has_value()) {
						if (wall.value().damage > std::get<3>(best_match1)) {
							std::get<3>(best_match1) = wall.value().damage;
							best_match = std::make_tuple(std::get<0>(best_match1), std::get<1>(best_match1), std::get<2>(best_match1));
						}
					}
				}
			}
		});

	if (std::get<2>(best_match) == FLT_MAX || !std::get<1>(best_match) || !std::get<1>(best_match)->player)
		return std::nullopt;
	std::optional<std::tuple<c_vector3d, float, c_animation_system::animation*, float>> target = std::nullopt;
	auto t = std::make_tuple(std::get<0>(best_match), std::get<1>(best_match)->sim_time, std::get<1>(best_match), std::get<2>(best_match));
	target = t;
	return std::make_tuple(std::get<0>(best_match), std::get<1>(best_match)->sim_time, std::get<1>(best_match), std::get<2>(best_match));
}

std::optional<std::tuple<c_vector3d, float, c_animation_system::animation*>> c_aimhelper::get_legit_target(float target_fov, float range, std::optional<c_cs_player::hitbox> override_hitbox, bool visible)
{
	auto local = c_cs_player::get_local_player();

	if (!local)
		return std::nullopt;

	auto current_pos = local->get_shoot_position();
	auto current_angle = engine_client()->get_view_angles();

	std::tuple<c_vector3d, c_animation_system::animation*, float> best_match = { c_vector3d(), nullptr, FLT_MAX };

	client_entity_list()->for_each_player([&](c_cs_player* player) -> void
		{
			if (!player->is_alive() || (visible && player->is_dormant()) || !player->is_enemy())
				return;

			if (!visible && player->get_simtime() + 7.f < global_vars_base->curtime)
				return;

			const auto current = animation_system->get_lastest_animation_unsafe(player);

			if (!current.has_value() ||
				(visible && !player->is_visible(const_cast<matrix3x4*>(current.value()->bones))))
				return;

			// check fov estimate for performance reasons
			auto view = player->get_shoot_position();
			const auto estimate_fov = math::get_fov(current_angle, current_pos, view);

			if (estimate_fov > 2.f * target_fov)
				return;

			const auto animations = range > 0.f ? animation_system->get_valid_animations(player, range) : std::vector<c_animation_system::animation*>{ current.value() };

			for (auto& animation : animations)
				if (!override_hitbox.has_value())
					for (const auto& hitbox : c_cs_player::hitboxes)
					{
						auto box = player->get_hitbox_position(hitbox, const_cast<matrix3x4*>(animation->bones));
						if (!box.has_value())
							continue;

						const auto fov = math::get_fov(current_angle, current_pos, box.value());

						if (fov < std::get<2>(best_match) && fov <= target_fov)
							best_match = std::make_tuple(box.value(), animation, fov);
					}
				else
				{
					auto box = player->get_hitbox_position(override_hitbox.value(), const_cast<matrix3x4*>(animation->bones));
					if (!box.has_value())
						continue;

					const auto fov = math::get_fov(current_angle, current_pos, box.value());

					if (fov < std::get<2>(best_match) && fov <= target_fov)
						best_match = std::make_tuple(box.value(), animation, fov);
				}
		});

	if (std::get<2>(best_match) == FLT_MAX || !std::get<1>(best_match) || !std::get<1>(best_match)->player)
		return std::nullopt;

	return std::make_tuple(std::get<0>(best_match), std::get<1>(best_match)->sim_time, std::get<1>(best_match));
}

void inline SinCos223(float radians, float* sine, float* cosine)
{
	*sine = sin(radians);
	*cosine = cos(radians);
}

void AngleVecto2rs(const c_vector3d& angles, c_vector3d* forward)
{
	float	sp, sy, cp, cy;

	SinCos223(deg2rad(angles[1]), &sy, &cy);
	SinCos223(deg2rad(angles[0]), &sp, &cp);

	forward->x = cp * cy;
	forward->y = cp * sy;
	forward->z = -sp;
}

std::vector<c_aimhelper::aim_info> c_aimhelper::select_multipoint(c_animation_system::animation* animation, c_cs_player::hitbox box, int32_t group,
	std::optional < Menu22::ragebot::weapon_config> cfg)
{
	std::vector<aim_info> points = {};

	if (!animation || !animation->player || !cfg.has_value())
		return points;

	if (!is_multipoint_hitbox(box))
		return points;

	auto baim_scale = std::max(cfg->pelvis_scale, cfg->body_scale);

	static auto hitbox_scale = 0.f;

	switch (box) {
		case c_cs_player::hitbox::head: {baim_scale = cfg->head_scale; } break;
		case c_cs_player::hitbox::pelvis: {baim_scale = cfg->stomach_scale; } break;
		case c_cs_player::hitbox::body: {baim_scale = cfg->stomach_scale; } break;
		case c_cs_player::hitbox::thorax: {baim_scale = cfg->stomach_scale; } break;
		case c_cs_player::hitbox::chest: {baim_scale = cfg->chest_scale; } break;
		case c_cs_player::hitbox::upper_chest: {baim_scale = cfg->chest_scale; } break;
		case c_cs_player::hitbox::left_calf: {baim_scale = cfg->legs_scale; } break;
		case c_cs_player::hitbox::right_calf: {baim_scale = cfg->legs_scale; } break;
		case c_cs_player::hitbox::left_foot: {baim_scale = cfg->feet_scale; } break;
		case c_cs_player::hitbox::right_foot: {baim_scale = cfg->feet_scale; } break;
	}

	float scale = baim_scale;
	scale = scale / 100.f;

	if (scale < .1f || scale > 1.f)
		return points;

	const auto model = animation->player->get_model();
	if (!model)
		return points;

	const auto studio_model = model_info_client()->get_studio_model(model);
	if (!studio_model)
		return points;

	const auto local = c_cs_player::get_local_player();
	if (!local)
		return points;

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	if (!weapon)
		return points;

	const auto anim = animation_system->get_animation_info(animation->player);
	if (!anim)
		return points;

	const auto hitbox = studio_model->get_hitbox(static_cast<uint32_t>(box), 0);
	if (!hitbox)
		return points;

	const auto is_zeus = weapon->get_item_definition() == weapon_taser;
	if (is_zeus)
		return points;

	auto& mat = animation->bones[hitbox->bone];
	const auto mod = hitbox->radius != -1.f ? hitbox->radius : 0.f;

	c_vector3d min, max;
	math::vector_transform(hitbox->bbmax + mod, mat, max);
	math::vector_transform(hitbox->bbmin - mod, mat, min);

	const auto center = (min + max) * 0.5f;
	const auto cur_angles = math::calc_angle(center, local->get_shoot_position());

	c_vector3d forward;
	math::angle_vectors(cur_angles, forward);

	auto rs = hitbox->radius * scale;
	rs *= 0.95f - (std::min(anim->missed_due_to_spread, 3) * 0.25);

	if (rs < .05f)
		return points;

	const auto right = forward.cross(c_vector3d(0.f, 0.f, 1.f)) * rs;
	const auto left =  c_vector3d(-right.x, -right.y, right.z);
	const auto top =   c_vector3d(0.f, 0.f, 1.f) * rs;

	points.emplace_back(center, 0.f, animation, false, center, hitbox->radius, rs, box, group);

	points.emplace_back(center + top, 0.f, animation, false, center, hitbox->radius, rs, box, group);
	points.emplace_back(center - top, 0.f, animation, false, center, hitbox->radius, rs, box, group);
	points.emplace_back(center + right, 0.f, animation, false, center, hitbox->radius, rs, box, group);
	points.emplace_back(center + left, 0.f, animation, false, center, hitbox->radius, rs, box, group);

	//debug_overlay()->AddSphereOverlay(center + top, 1, 5, 5, 255, 255, 0, 255, 0.1);
	//debug_overlay()->AddSphereOverlay(center - top, 1, 5, 5, 255, 255, 0, 255, 0.1);
	//debug_overlay()->AddSphereOverlay(center, 1, 5, 5, 0, 255, 0, 255, 0.1);
	//debug_overlay()->AddSphereOverlay(center + right, 1, 5, 5, 255, 0, 0, 255, 0.1);
	//debug_overlay()->AddSphereOverlay(center + left, 1, 5, 5, 255, 0, 0, 255, 0.1);

	return points;
}

bool c_aimhelper::optimize_multipoint(c_cs_player* local, aim_info& info, float min_dmg)
{
	if (!local || info.rs < 0.1f)
		return false;

	auto original_position = info.position;
	auto optimal_position =  info.center;
	auto optimal_damage =    info.damage;

	if (info.position == info.center)
		return false;

	const auto wall = trace_system->wall_penetration(local->get_shoot_position(),
		optimal_position, info.animation);

	float m_dmg_to_hit = min_dmg > info.animation->health ? info.animation->health : min_dmg;

	if (wall.has_value())
	{
		if (wall.value().damage < m_dmg_to_hit)
			return false;

		info.position = optimal_position;
		info.damage   = wall.value().damage;
		info.rs = 0;
	}

	return true;
}

bool c_aimhelper::can_hit_hitbox(const c_vector3d start, const c_vector3d end, c_animation_system::animation* animation, studiohdr* hdr, c_cs_player::hitbox box)
{
	const auto studio_box = hdr->get_hitbox(static_cast<uint32_t>(box), 0);

	if (!studio_box)
		return false;

	c_vector3d min, max;

	const auto is_capsule = studio_box->radius != -1.f;

	if (is_capsule)
	{
		math::vector_transform(studio_box->bbmin, animation->bones[studio_box->bone], min);
		math::vector_transform(studio_box->bbmax, animation->bones[studio_box->bone], max);
		const auto dist = math::segment_to_segment(start, end, min, max);

		if (dist < studio_box->radius)
		{
		//	debug_overlay()->AddLineOverlay(start, end, 255, 0, 0, true, 1.0f);
			//debug_overlay()->AddLineOverlay(min, max, 255, 255, 90, true, 1.0f);

			return true;
		}
	}

	if (!is_capsule)
	{
		math::vector_transform(math::vector_rotate(studio_box->bbmin, studio_box->rotation), animation->bones[studio_box->bone], min);
		math::vector_transform(math::vector_rotate(studio_box->bbmax, studio_box->rotation), animation->bones[studio_box->bone], max);

		math::vector_i_transform(start, animation->bones[studio_box->bone], min);
		math::vector_i_rotate(end, animation->bones[studio_box->bone], max);

		if (math::intersect_line_with_bb(min, max, studio_box->bbmin, studio_box->bbmax))
		{
			//debug_overlay()->AddLineOverlay(start, end, 255, 0, 0, true, 1.0f);
			//debug_overlay()->AddLineOverlay(min, max, 255, 255, 90, true, 1.0f);

			return true;
		}
	}

	return false;
}

// pasted from aimware. thanks polak.
void c_aimhelper::fix_movement(c_user_cmd* cmd, c_qangle& wishangle)
{
	c_vector3d view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	math::angle_vectors(wishangle, view_fwd, view_right, view_up);
	math::angle_vectors(cmd->viewangles, cmd_fwd, cmd_right, cmd_up);

	const auto v8 = sqrtf((view_fwd.x * view_fwd.x) + (view_fwd.y * view_fwd.y));
	const auto v10 = sqrtf((view_right.x * view_right.x) + (view_right.y * view_right.y));
	const auto v12 = sqrtf(view_up.z * view_up.z);

	const c_vector3d norm_view_fwd((1.f / v8) * view_fwd.x, (1.f / v8) * view_fwd.y, 0.f);
	const c_vector3d norm_view_right((1.f / v10) * view_right.x, (1.f / v10) * view_right.y, 0.f);
	const c_vector3d norm_view_up(0.f, 0.f, (1.f / v12) * view_up.z);

	const auto v14 = sqrtf((cmd_fwd.x * cmd_fwd.x) + (cmd_fwd.y * cmd_fwd.y));
	const auto v16 = sqrtf((cmd_right.x * cmd_right.x) + (cmd_right.y * cmd_right.y));
	const auto v18 = sqrtf(cmd_up.z * cmd_up.z);

	const c_vector3d norm_cmd_fwd((1.f / v14) * cmd_fwd.x, (1.f / v14) * cmd_fwd.y, 0.f);
	const c_vector3d norm_cmd_right((1.f / v16) * cmd_right.x, (1.f / v16) * cmd_right.y, 0.f);
	const c_vector3d norm_cmd_up(0.f, 0.f, (1.f / v18) * cmd_up.z);

	const auto v22 = norm_view_fwd.x * cmd->forwardmove;
	const auto v26 = norm_view_fwd.y * cmd->forwardmove;
	const auto v28 = norm_view_fwd.z * cmd->forwardmove;
	const auto v24 = norm_view_right.x * cmd->sidemove;
	const auto v23 = norm_view_right.y * cmd->sidemove;
	const auto v25 = norm_view_right.z * cmd->sidemove;
	const auto v30 = norm_view_up.x * cmd->upmove;
	const auto v27 = norm_view_up.z * cmd->upmove;
	const auto v29 = norm_view_up.y * cmd->upmove;

	cmd->forwardmove = ((((norm_cmd_fwd.x * v24) + (norm_cmd_fwd.y * v23)) + (norm_cmd_fwd.z * v25))
		+ (((norm_cmd_fwd.x * v22) + (norm_cmd_fwd.y * v26)) + (norm_cmd_fwd.z * v28)))
		+ (((norm_cmd_fwd.y * v30) + (norm_cmd_fwd.x * v29)) + (norm_cmd_fwd.z * v27));
	cmd->sidemove = ((((norm_cmd_right.x * v24) + (norm_cmd_right.y * v23)) + (norm_cmd_right.z * v25))
		+ (((norm_cmd_right.x * v22) + (norm_cmd_right.y * v26)) + (norm_cmd_right.z * v28)))
		+ (((norm_cmd_right.x * v29) + (norm_cmd_right.y * v30)) + (norm_cmd_right.z * v27));
	cmd->upmove = ((((norm_cmd_up.x * v23) + (norm_cmd_up.y * v24)) + (norm_cmd_up.z * v25))
		+ (((norm_cmd_up.x * v26) + (norm_cmd_up.y * v22)) + (norm_cmd_up.z * v28)))
		+ (((norm_cmd_up.x * v30) + (norm_cmd_up.y * v29)) + (norm_cmd_up.z * v27));

	const auto ratio = 2.f - fmaxf(fabsf(cmd->sidemove), fabsf(cmd->forwardmove)) / math::forward_bounds;
	cmd->forwardmove *= ratio;
	cmd->sidemove *= ratio;

	wishangle = cmd->viewangles;
}

__forceinline void c_aimhelper::build_seed_table()
{
	if (!precomputed_seeds.empty())
		return;

	for (auto i = 0; i < total_seeds; i++) {
		math::random_seed(i + 1);

		const auto pi_seed = math::random_float(0.f, pi_2);

		precomputed_seeds.emplace_back(math::random_float(0.f, 1.f),
			sin(pi_seed), cos(pi_seed));
	}
}

bool c_aimhelper::can_hit_hitbox(int index, const c_vector3d start, const c_vector3d end, matrix3x4* bones, c_cs_player::hitbox box)
{
	const auto entity = (c_cs_player*)client_entity_list()->get_client_entity(index);

	if (!entity)
		return false;

	const auto model = entity->get_model();
	if (!model)
		return false;

	auto* p_studio_hdr = model_info_client()->get_studio_model(model);
	if (!p_studio_hdr)
		return false;

	const auto studio_box = p_studio_hdr->get_hitbox(static_cast<uint32_t>(box), 0);

	if (!studio_box)
		return false;

	c_vector3d min, max;

	const auto is_capsule = studio_box->radius != -1.f;

	if (is_capsule)
	{
		math::vector_transform(studio_box->bbmin, bones[studio_box->bone], min);
		math::vector_transform(studio_box->bbmax, bones[studio_box->bone], max);
		const auto dist = math::segment_to_segment(start, end, min, max);

		if (dist < studio_box->radius)
			return true;
	}

	if (!is_capsule)
	{
		math::vector_transform(math::vector_rotate(studio_box->bbmin, studio_box->rotation), bones[studio_box->bone], min);
		math::vector_transform(math::vector_rotate(studio_box->bbmax, studio_box->rotation), bones[studio_box->bone], max);

		math::vector_i_transform(start, bones[studio_box->bone], min);
		math::vector_i_rotate(end, bones[studio_box->bone], max);

		if (math::intersect_line_with_bb(min, max, studio_box->bbmin, studio_box->bbmax))
			return true;
	}

	return false;
}

using msg_t32 = void(__cdecl*)(const char*, ...);
msg_t32 message2 = reinterpret_cast<msg_t32>(GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg"));

bool c_aimhelper::hit_chance(c_vector3d angles, c_base_player* ent, float hit_chance, c_cs_player::hitbox box, c_animation_system::animation* animation, const c_vector3d position, const c_vector3d start, c_base_combat_weapon* weapon, c_cs_player* local)
{
	if (!local)
		return false;

	//const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));
	if (!weapon)
		return false;

	const auto wpn_info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!wpn_info)
		return false;

	const auto studio_model = model_info_client()->get_studio_model(ent->get_model());

	if (!studio_model)
		return false;

	static auto weapon_recoil_scale = cvar()->find_var("weapon_recoil_scale")->get_float();
	angles -= local->get_punch_angle() * weapon_recoil_scale;

	c_vector3d forward, right, up;
	c_vector3d src = start;   // local->get_shoot_position();
	math::angle_vectors(angles.clamp_angles(), forward, right, up);
	math::fast_vec_normalize(forward);
	math::fast_vec_normalize(right);
	math::fast_vec_normalize(up);
	
	int traces_hit = 0;
	int traces_should_hit = static_cast<int>(255.f * (hit_chance / 100.f));

	//weapon->update_accuracy();
	float weapon_spread = weapon->get_spread();
	float weapon_inaccuracy = weapon->get_inaccuracy();

	if (!(local->is_on_ground()) && weapon->get_item_definition() == weapon_ssg08) {
		if (weapon->get_inaccuracy() < 0.009f) {
			hit_chance = 99;
			return true;
		}
	}

	for (int i = 0; i < 255; i++) {
		math::random_seed((i & 255) + 1); // shit parasha ot nas xui Kavkaz

		//float a = math::random_float(0.f, 1.f);
		float b = math::random_float(0.f, 2.f * pi);
		float c = math::random_float(0.f, 1.f);
		float d = math::random_float(0.f, 2.f * pi);

		float inaccuracy = c * weapon_inaccuracy;
		float spread = c * weapon_spread;

		if (weapon->get_item_definition() == weapon_revolver) {
			c = 1.f - c * c;
			c = 1.f - c * c;
		}

		c_vector3d spread_view((cos(b) * spread) + (cos(d) * inaccuracy), (sin(b) * spread) + (sin(d) * inaccuracy), 0), direction;

		direction.x = forward.x + (spread_view.x * right.x) + (spread_view.y * up.x);
		direction.y = forward.y + (spread_view.x * right.y) + (spread_view.y * up.y);
		direction.z = forward.z + (spread_view.x * right.z) + (spread_view.y * up.z);
		math::fast_vec_normalize(direction);

		c_vector3d view_angles_spread;
		math::vector_angles(direction, view_angles_spread);
		math::normalize(view_angles_spread);

		c_vector3d end;
		math::angle_vector___(angles - (view_angles_spread - angles), end);
		math::fast_vec_normalize(end);

		c_vector3d trace_end;
		trace_end = src + (end * wpn_info->flRange);

		trace tr;
		ray_t ray;
		ray.init(src, trace_end);
		engine_trace()->clip_ray_to_entity(ray, mask_shot_hull | contents_hitbox, ent, &tr);

		//debug_overlay()->AddLineOverlay(src, trace_end, 180, 240, 0, true, 0.1f);
		//if (can_hit_hitbox(src, trace_end, animation, studio_model, box))
		//	++traces_hit;

		if (tr.entity == ent)
			++traces_hit;

		if (static_cast<float>((static_cast<float>(traces_hit) / 255) * 100.f) >= hit_chance)
			return true;

		if ((255 - i + traces_hit) < traces_should_hit)
			return false;
	}

	return false;
}