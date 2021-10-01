#include "c_resolver.h"
#include "../utils/math.h"
#include "c_aimhelper.h"
#include "c_trace_system.h"
#include "../sdk/c_client_entity_list.h"
#include <random>
#include "c_esp.h"
#include "../sdk/c_weapon_system.h"
#include "../idaboss.h"
#include "c_trace_system.h"
#include "../baim.h"
#include "../hooks/c_hooks.h"
c_resolver resolver;

float flAngleMod(float flAngle)
{
	return((360.0f / 65536.0f) * ((int32_t)(flAngle * (65536.0f / 360.0f)) & 65535));
}

float ApproachAngle(float flTarget, float flValue, float flSpeed)
{
	flTarget = flAngleMod(flTarget);
	flValue = flAngleMod(flValue);

	float delta = flTarget - flValue;

	// Speed is assumed to be positive
	if (flSpeed < 0)
		flSpeed = -flSpeed;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	if (delta > flSpeed)
		flValue += flSpeed;
	else if (delta < -flSpeed)
		flValue -= flSpeed;
	else
		flValue = flTarget;

	return flValue;
}

static auto GetSmoothedVelocity = [](float min_delta, c_vector3d a, c_vector3d b) {
	c_vector3d delta = a - b;
	float delta_length = delta.length();

	if (delta_length <= min_delta) {
		c_vector3d result;
		if (-min_delta <= delta_length) {
			return a;
		}
		else {
			float iradius = 1.0f / (delta_length + FLT_EPSILON);
			return b - ((delta * iradius) * min_delta);
		}
	}
	else {
		float iradius = 1.0f / (delta_length + FLT_EPSILON);
		return b + ((delta * iradius) * min_delta);
	}
};

float AngleDiff(float destAngle, float srcAngle) {
	float delta;

	delta = fmodf(destAngle - srcAngle, 360.0f);
	if (destAngle > srcAngle) {
		if (delta >= 180)
			delta -= 360;
	}
	else {
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

float calculate_server_velocity(c_animation_system::animation* record)
{
	auto state = record->anim_state;

	// push recalculated velocity
	state.velocity = record->player->get_velocity();

	// calculate abs velocity length
	const auto abs_velocity_length = powf(state.velocity.length(), 2.f);

	// calculate fraction
	const auto fraction = 1.f / (abs_velocity_length + 0.11920929f);

	// sync velocity
	if (abs_velocity_length > 344.008f)
		state.velocity *= fraction * 12.f;

	// sync up speed
	state.speed_up = state.velocity.z;

	// calculate new speed
	auto speed = state.velocity.length2d();

	// apply fraction to move_speed
	state.move_speed = state.velocity * fraction;

	// clamp animation speed
	if (speed >= 150.f)
		speed = 150.f;

	// sync speed
	state.speed = speed;

	return speed;
}

// OneTap Rebuild System //
/*
void store_deltas(c_animation_system::animation* record)
{
	c_csgo_player_anim_state animstate_backup;
	memcpy(&animstate_backup, record->player->get_anim_state(), 0x344);

	//const auto abs_origin = m_player->get_abs_origin();
	const auto poses = record->player->get_pose_parameter();
	const auto backup_angles = record->player->get_eye_angles();
	const auto backup_velocity = record->player->get_velocity();
	const auto backup_origin = record->player->get_origin();
	const auto backup_duckamt = record->player->get_duck_amount();
	const auto backup_simtime = record->player->get_simtime();
	const auto backup_flags = record->player->get_flags();

	auto m_records = cheat::features::lagcomp.records[record->index - 1].m_Tickrecords;
	c_base_animating::animation_layers* animlayers = &record->layers;

	//resolve_data->previous_rotation = backup_angles.y;

	auto lag = m_records.empty() ? 1 : ticks_to_time(record->player->get_simtime() - m_records[0].simulation_time);

	if (lag <= 1)
	{
		{
			std::memcpy(record->player->get_animation_layers(), animlayers, 0x38 * 13);

			cheat::features::lagcomp.update_animations(record->player, (m_records.empty() ? nullptr : &m_records[0]), -1);

			memcpy(resolve_data->resolver_anim_layers[1], record->player->get_animation_layers(), 0x38 * 13);
			std::memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			record->player->force_bone_rebuild();
			//record->player->setup_bones()
			record->build_server_bones(record->player);
			//memcpy(resolve_data->rightmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			//resolve_data->right_side = record->player->get_anim_state()->goal_feet_yaw;
			//cheat::features::lagcomp.store_record_data(record->player, &resolve_data->rightrec);

			record->player->get_pose_parameter() = poses;
			memcpy(record->player->get_anim_state(), &animstate_backup, 0x344);
			memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);

			record->player->get_velocity() = backup_velocity;
			record->player->get_origin() = backup_origin;
			record->player->get_duck_amount() = backup_duckamt;
			record->player->get_simtime() = backup_simtime;
			record->player->get_eye_angles() = backup_angles;
			record->player->get_flags() = backup_flags;
			record->player->get_abs_velocity() = backup_velocity;
		}

		{
			cheat::features::lagcomp.update_animations(record->player, (m_records.empty() ? nullptr : &m_records[0]), 1);

			std::memcpy(resolve_data->resolver_anim_layers[2], record->player->get_animation_layers(), 0x38 * 13);
			std::memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			record->player->force_bone_rebuild();
			//record->player->SetupBonesEx();
			record->build_server_bones(record->player);
			//memcpy(resolve_data->leftmx, m_player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * m_player->GetBoneCount());
			//cheat::features::lagcomp.store_record_data(record->player, &resolve_data->leftrec);
			//resolve_data->left_side = record->player->get_anim_state()->goal_feet_yaw;

			record->player->get_pose_parameter() = poses;
			memcpy(record->player->get_anim_state(), &animstate_backup, 0x344);
			memcpy(record->player->get_animation_layers(), animlayers, 0x38 * 13);

			record->player->get_velocity() = backup_velocity;
			record->player->get_origin() = backup_origin;
			record->player->get_duck_amount() = backup_duckamt;
			record->player->get_simtime() = backup_simtime;
			record->player->get_eye_angles() = backup_angles;
			record->player->get_flags() = backup_flags;
			record->player->get_abs_velocity() = backup_velocity;
		}
	}
	else
	{
		cheat::features::lagcomp.update_animations(record->player, (m_records.empty() ? nullptr : &m_records[0]), 1);

		auto v38 = AngleDiff(record->player->get_eye_angles().y, record->player->get_anim_state()->goal_feet_yaw);
		auto is_inverted = v38 < 0.0f;

		if (is_inverted)
		{
			std::memcpy(resolve_data->resolver_anim_layers[1], record->player->get_animation_layers(), 0x38 * 13);
			std::memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			record->player->force_bone_rebuild();
			//record->player->SetupBonesEx();
			record->build_server_bones(record->player);
			//memcpy(resolve_data->rightmx, record->player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * record->player->GetBoneCount());
			//cheat::features::lagcomp.store_record_data(record->player, &resolve_data->rightrec);
			//resolve_data->right_side = record->player->get_animation_state()->abs_yaw;
		}
		else
		{
			std::memcpy(resolve_data->resolver_anim_layers[2], record->player->get_animation_layers(), 0x38 * 13);
			std::memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);
			//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
			record->player->force_bone_rebuild();
			//record->player->SetupBonesEx();
			record->build_server_bones(record->player);
			//memcpy(resolve_data->leftmx, record->player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * record->player->GetBoneCount());
			//cheat::features::lagcomp.store_record_data(record->player, &resolve_data->leftrec);
			//resolve_data->left_side = record->player->get_animation_state()->abs_yaw;
		}

		record->player->get_pose_parameter() = poses;
		memcpy(record->player->get_anim_state(), &animstate_backup, 0x344);
		memcpy(record->player->get_animation_layers(), animlayers, 0x38 * 13);

		record->player->get_velocity() = backup_velocity;
		record->player->get_origin() = backup_origin;
		record->player->get_duck_amount() = backup_duckamt;
		record->player->get_simtime() = backup_simtime;
		record->player->get_eye_angles() = backup_angles;
		record->player->get_flags() = backup_flags;
		record->player->get_abs_velocity() = backup_velocity;

		{
			std::memcpy(record->player->get_animation_layers(), animlayers, 0x38 * 13);

			cheat::features::lagcomp.update_animations(record->player, (m_records.empty() ? nullptr : &m_records[0]), -1);

			if (is_inverted)
			{
				std::memcpy(resolve_data->resolver_anim_layers[2], record->player->get_animation_layers(), 0x38 * 13);
				std::memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				record->player->force_bone_rebuild();
				//record->player->SetupBonesEx();
				record->build_server_bones(record->player);
				//memcpy(resolve_data->leftmx, record->player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * record->player->GetBoneCount());
				//cheat::features::lagcomp.store_record_data(record->player, &resolve_data->leftrec);
				//resolve_data->left_side = record->player->get_animation_state()->abs_yaw;
			}
			else
			{
				memcpy(resolve_data->resolver_anim_layers[1], record->player->get_animation_layers(), 0x38 * 13);
				std::memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);
				//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
				record->player->force_bone_rebuild();
				//record->player->SetupBonesEx();
				record->build_server_bones(record->player);
				//memcpy(resolve_data->rightmx, record->player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * record->player->GetBoneCount());
				//resolve_data->right_side = record->player->get_animation_state()->abs_yaw;
				//cheat::features::lagcomp.store_record_data(record->player, &resolve_data->rightrec);
			}

			record->player->get_pose_parameter() = poses;
			memcpy(record->player->get_anim_state(), &animstate_backup, 0x344);
			memcpy(record->player->get_animation_layers(), animlayers, 0x38 * 13);

			record->player->get_velocity() = backup_velocity;
			record->player->get_origin() = backup_origin;
			record->player->get_duck_amount() = backup_duckamt;
			record->player->get_simtime() = backup_simtime;
			record->player->get_eye_angles() = backup_angles;
			record->player->get_flags() = backup_flags;
			record->player->get_abs_velocity() = backup_velocity;
		}
	}

	{
		cheat::features::lagcomp.update_animations(record->player, (m_records.empty() ? nullptr : &m_records[0]), 0);
		memcpy(resolve_data->resolver_anim_layers[0], record->player->get_animation_layers(), 0x38 * 13);
		std::memcpy(record->player->get_animation_layers(), resolve_data->server_anim_layers, 0x38 * 13);
		//m_player->set_abs_angles(QAngle(0, (*(float*)(uintptr_t(m_player->get_animation_state()) + 0x80)), 0));
		record->player->force_bone_rebuild();
		record->build_server_bones(record->player);
		//record->player->SetupBonesEx();
		//cheat::features::lagcomp.store_record_data(record->player, &resolve_data->norec);
		//memcpy(resolve_data->nodesmx, record->player->m_CachedBoneData().Base(), sizeof(matrix3x4_t) * record->player->GetBoneCount());
		//resolve_data->no_side = record->player->get_animation_state()->abs_yaw;

		record->player->get_pose_parameter() = poses;
		memcpy(record->player->get_anim_state(), &animstate_backup, 0x344);
		memcpy(record->player->get_animation_layers(), animlayers, 0x38 * 13);

		record->player->get_velocity() = backup_velocity;
		record->player->get_origin() = backup_origin;
		record->player->get_duck_amount() = backup_duckamt;
		record->player->get_simtime() = backup_simtime;
		record->player->get_eye_angles() = backup_angles;
		record->player->get_flags() = backup_flags;
		record->player->get_abs_velocity() = backup_velocity;
	}
}

// OneTap Resolver //
bool resolve_by_onetap_resolver(c_animation_system::animation* record)
{

	rebuild_and_store();

	auto speed = record->player->get_velocity().length2d();

	if (record->player->get_flags() & c_base_player::flags::on_ground && record->flags & c_base_player::flags::on_ground)
	{
		if (speed < 0.1f)
		{
			auto delta = AngleDiff(record->player->get_eye_angles().y, lag_data->no_side);

			if (lag_data->server_anim_layers[3].m_flWeight == 0.0f && lag_data->server_anim_layers[3].m_flCycle == 0.0f) {
				lag_data->resolving_way = std::clamp((2 * (delta <= 0.f) - 1), -1, 1);// copysign(1, delta);
				lag_data->animations_updated = true;
			}
		}
		else if (!int(record->layers[12].weight * 1000.f))//(lag_data->server_anim_layers[6].m_flWeight * 1000.f) == (history_data->at(0).anim_layers[6].m_flWeight * 1000.f))
		{
			//2 = -1; 3 = 1; 1 = fake;
			if (int(record->layers[6].weight * 1000.f) == int(record->layers[6].weight * 1000.f))
			{
				float delta1 = abs(record->layers[6].playback_rate - record->resolver_layers[0][6].playback_rate);
				float delta2 = abs(record->layers[6].playback_rate - record->resolver_layers[1][6].playback_rate);
				float delta3 = abs(record->layers[6].playback_rate - record->resolver_layers[2][6].playback_rate);

				if (delta1 < delta3 || delta2 <= delta3 || (int)(float)(delta3 * 1000.0f)) {
					if (delta1 >= delta2 && delta3 > delta2 && !(int)(float)(delta2 * 1000.0f))
					{
						//lag_data->resolving_method = 1;
						lag_data->resolving_way = 1;
						lag_data->animations_updated = true;
						lag_data->resolved = true;
						lag_data->last_anims_update_time = global_vars_base->realtime;
					}
				}
				else
				{
					//lag_data->resolving_method = -1;
					lag_data->resolving_way = -1;
					lag_data->animations_updated = true;
					lag_data->resolved = true;
					lag_data->last_anims_update_time = global_vars_base->realtime;
				}
			}
		}
	}

	auto ResolvedYaw = math::normalize_yaw(record->player->get_eye_angles().y + math::normalize_yaw(60.f * lag_data->resolving_method));//(lag_data->resolving_method == 0 ? lag_data->no_side : (lag_data->resolving_method > 0 ? lag_data->left_side : lag_data->right_side));//
}

*/

bool  UseFreestand[65];
float FreestandAngle[65];
float LastFreestandAngle[65];
float FreestandAngle_ò[65];
float OldYaw[65];

void c_resolver::AntiFreestanding()
{
	if (!config->Ragebot.enable)
		return;

	auto local = c_cs_player::get_local_player();

	if (!local)
		return;

	if (!local->is_alive())
		return;

	auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	if (!weapon)
		return;

	for (int index = 1; index <= engine_client()->get_max_clients(); ++index)
	{
		const auto pPlayerEntity = (c_cs_player*)client_entity_list()->get_client_entity(index);

		//auto pPlayerEntity = g_csgo.m_entlist()->get_client_entity< c_base_player >(index);

		if (!pPlayerEntity
			|| !pPlayerEntity->is_alive()
			|| !pPlayerEntity->is_enemy()
			|| local->index() == index
			|| pPlayerEntity->is_dormant()
			|| local->get_team() == pPlayerEntity->get_team())
		{
			continue;
		}

		UseFreestand[index] = false;

		//	if ( !bUseFreestandAngle[ index ] )
		{
			bool Autowalled = false, HitSide1 = false, HitSide2 = false;

			float angToLocal = math::calc_angle(local->get_origin(), pPlayerEntity->get_origin()).y;
			c_vector3d ViewPoint = local->get_origin() + c_vector3d(0, 0, 80);

			c_vector3d Side1 = c_vector3d((45 * sin(deg2rad(angToLocal))), (45 * cos(deg2rad(angToLocal))), 0.f);
			c_vector3d Side2 = c_vector3d((45 * sin(deg2rad(angToLocal + 180))), (45 * cos(deg2rad(angToLocal + 180))), 0.f);

			c_vector3d Side3 = c_vector3d((50 * sin(deg2rad(angToLocal))), (50 * cos(deg2rad(angToLocal))), 0.f);
			c_vector3d Side4 = c_vector3d((50 * sin(deg2rad(angToLocal + 180))), (50 * cos(deg2rad(angToLocal + 180))), 0.f);

			c_vector3d Origin = pPlayerEntity->get_origin();

			c_vector3d OriginLeftRight[] = { c_vector3d(Side1.x, Side1.y, 0.f), c_vector3d(Side2.x, Side2.y, 0.f) };

			c_vector3d OriginLeftRightLocal[] = { c_vector3d(Side3.x, Side3.y, 0.f), c_vector3d(Side4.x, Side4.y, 0.f) };

	//		if (!g_Autowall->CanHitFloatingPoint(pPlayerEntity->get_eye_pos(), local->get_eye_pos(), pPlayerEntity))
			{
				for (int side = 0; side < 2; side++)
				{
					c_vector3d OriginAutowall = c_vector3d(Origin.x + OriginLeftRight[side].x, Origin.y - OriginLeftRight[side].y, Origin.z + 90 );
					c_vector3d OriginAutowall2 = c_vector3d(ViewPoint.x + OriginLeftRightLocal[side].x, ViewPoint.y - OriginLeftRightLocal[side].y, ViewPoint.z);

					if (g_Autowall->CanHitFloatingPoint(OriginAutowall, ViewPoint, pPlayerEntity))
					{
						if (side == 0)
						{
							HitSide1 = true;
							FreestandAngle[index] = -90;
							FreestandAngle_ò[index] = -1;
						}
						else if (side == 1)
						{
							HitSide2 = true;
							FreestandAngle[index] = 90;
							FreestandAngle_ò[index] = 1;
						}

						Autowalled = true;
					}
					else
					{
						for (int side222 = 0; side222 < 2; side222++)
						{
							c_vector3d OriginAutowall222 = c_vector3d(Origin.x + OriginLeftRight[side222].x, Origin.y - OriginLeftRight[side222].y, Origin.z + 90 );

							if (g_Autowall->CanHitFloatingPoint(OriginAutowall222, OriginAutowall2, pPlayerEntity))
							{
								if (side222 == 0)
								{
									HitSide1 = true;
									FreestandAngle[index] = -90;
									FreestandAngle_ò[index] = -1;
								}
								else if (side222 == 1)
								{
									HitSide2 = true;
									FreestandAngle[index] = 90;
									FreestandAngle_ò[index] = 1;
								}

								Autowalled = true;
							}
						}
					}
				}
			}
			if (Autowalled)
			{
				if (HitSide1 && HitSide2)
					UseFreestand[index] = false;
				else
				{
					UseFreestand[index] = true;
					LastFreestandAngle[index] = FreestandAngle[index];
				}
			}
		}
	}
}

void c_resolver::resolve(c_animation_system::animation* record)
{
	float resolver_yaw = 0.f;

	// is it neither a bot nor a legit player?
	if (!record->player || record->player->get_info().fakeplayer || !config->Ragebot.resolver)
	{
		resolver.missed_due_to_bad_resolve[record->index] = 0;
		return;
	}

	const auto info = animation_system->get_animation_info(record->player);
	if (!info)
		return;

	if (!record->has_anim_state)
		return;

	static float m_flFakeGoalFeetYaw[65] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	static float fl_shot_delta_backup[65] = { -1 };

	// Rebuild setup velocity to receive flMinBodyYaw and flMaxBodyYaw
	c_vector3d velocity = record->anim_state.velocity;

	float spd = velocity.lengthsqr();
	if (spd > std::powf(1.2f * 260.0f, 2.f)) {
		c_vector3d velocity_normalized = velocity.normalize();
		velocity = velocity_normalized * (1.2f * 260.0f);
	}
	float flAbsVelocity = velocity.z;
	velocity.z = 0;

	float choked_time = fabs(record->sim_time - (record->anim_time - global_vars_base->interval_per_tick));

	float deltatime = record->anim_state.delta_time;
	bool  is_standing = true;
	float v25 = std::clamp(record->duck + record->anim_state.landing_duck_additive_something, 0.0f, 1.0f);
	float v26 = record->anim_state.duck_amount;
	float v27 = deltatime * 6.0f;
	float v28;

	// clamp
	if ((v25 - v26) <= v27) {
		if (-v27 <= (v25 - v26))
			v28 = v25;
		else
			v28 = v26 - v27;
	}
	else {
		v28 = v26 + v27;
	}
	const auto local = c_cs_player::get_local_player();
	if (!local)
		return;
	float flDuckAmount = std::clamp(v28, 0.0f, 1.0f);

	c_vector3d animationVelocity = GetSmoothedVelocity(deltatime * 2000.0f, velocity, record->anim_state.velocity);
	float speed = std::fmin(animationVelocity.length(), 260.0f);

	auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(record->player->get_current_weapon_handle()));

	if (!weapon)
		return;

	auto wpndata = weapon_system->get_weapon_data(weapon->get_item_definition());

	if (!wpndata)
		return;

	float flMaxMovementSpeed = 260.0f;
	if (weapon) {
		flMaxMovementSpeed = std::fmaxf(wpndata->flMaxPlayerSpeed, 0.001f);
	}

	float flRunningSpeed = speed / (flMaxMovementSpeed * 0.520f);
	float flDuckingSpeed_2 = speed / (flMaxMovementSpeed * 0.340f);

	flRunningSpeed = std::clamp(flRunningSpeed, 0.0f, 1.0f);

	float flYawModifier = (((record->anim_state.stop_to_full_running_fraction * -0.3f) - 0.2f) * flRunningSpeed) + 1.0f;
	if (flDuckAmount > 0.0f) {
		float flDuckingSpeed = std::clamp(flDuckingSpeed_2, 0.0f, 1.0f);
		flYawModifier += (flDuckAmount * flDuckingSpeed) * (0.5f - flYawModifier);
	}

	static const float flMinBodyYawAngle = -58; // changed
	static const float flMaxBodyYawAngle = 58;

	float flMinBodyYaw = flMinBodyYawAngle * flYawModifier;
	float flMaxBodyYaw = flMaxBodyYawAngle * flYawModifier;

	m_flFakeGoalFeetYaw[record->player->index()] = record->anim_state.goal_feet_yaw;

	float flEyeYaw = record->eye_angles.y;
	float flEyeGoalDelta = AngleDiff(flEyeYaw, m_flFakeGoalFeetYaw[record->player->index()]);

	float Left  = flEyeYaw - 60;
	float Right = flEyeYaw + 60;
	float Back =  flEyeYaw;

	float EyeDelta = fabs(math::normalize_yaw(flEyeYaw - OldYaw[record->index]));
	OldYaw[record->index] = flEyeYaw;

	/*if (record->shot) // shit is shooting
	{
		float lby_delta = flEyeYaw - record->lby;

		if (info->brute_state == resolver_brute_left)
			m_flFakeGoalFeetYaw[record->player->index()] = flEyeYaw + 180 - lby_delta;
		else
			m_flFakeGoalFeetYaw[record->player->index()] = flEyeYaw + 180 + lby_delta;
	}*/
	//else  //(/*speed > 0.2 && speed <= 4.0*/) // sidemove 'standing' desync
	
		//animlayers onetap here.
		if (true == false)
		{

		}
		else
		{
			if (resolver.missed_due_to_spread[record->player->index()] >= 4 || resolver.missed_due_to_bad_resolve[record->player->index()] >= 3) {

				if (FreestandAngle[record->player->index()] == -90)
				{

					m_flFakeGoalFeetYaw[record->player->index()] = info->brute_state == resolver_brute_left ? Left : Right;
				}
				else if (FreestandAngle[record->player->index()] == 90)
				{
					m_flFakeGoalFeetYaw[record->player->index()] = info->brute_state == resolver_brute_left ? Right : Left;
				}

			}
			else
			{
				bool forward = fabsf(math::normalize_yaw(math::normalize_yaw(record->player->get_eye_angles().y) - math::normalize_yaw(math::calc_angle(local->get_origin(), record->player->get_origin()).y - 180.f))) < 90.f;
				if (forward) {
					FreestandAngle_ò[record->player->index()] *= -1;
					//	ResolverMode[idx] += hs::B.s();
				}
					//else
					//	ResolverMode[idx] += hs::A.s();
				m_flFakeGoalFeetYaw[record->player->index()] = math::normalize_yaw(math::normalize_yaw(record->player->get_eye_angles().y) - 90.f * FreestandAngle_ò[record->player->index()]);
			}
		}
	/*
    if (speed > 4.0) // not a stupido LBY breakers, only static and jitter desync's allowed.
	{
		m_flFakeGoalFeetYaw[record->player->index()] = info->brute_state == resolver_brute_left ? -record->lby : record->lby;
	}
	else if (speed <= 0.2) // GoalFeetDelta //
	{
		if (flEyeGoalDelta <= flMaxBodyYaw) {
			if (flMinBodyYaw > flEyeGoalDelta)
				m_flFakeGoalFeetYaw[record->player->index()] = fabs(flMinBodyYaw) - flEyeYaw;
		}
		else {
			m_flFakeGoalFeetYaw[record->player->index()] = flEyeYaw + fabs(flMaxBodyYaw);
		}

		m_flFakeGoalFeetYaw[record->player->index()] = std::remainderf(m_flFakeGoalFeetYaw[record->player->index()], 360.f);
	}
	*/

	if (record->is_backtrackable)
	{
		float fl_shot_delta = AngleDiff(record->eye_angles.y, record->anim_state.goal_feet_yaw);
		fl_shot_delta_backup[record->player->index()] = fl_shot_delta;
	}

	if (fabs(fl_shot_delta_backup[record->player->index()]) > 28)
		m_flFakeGoalFeetYaw[record->player->index()] = record->eye_angles.y + (fl_shot_delta_backup[record->player->index()] * 3);
	else if (fabs(record->anim_state.current_feet_yaw - record->anim_state.goal_feet_yaw) > 28)
		m_flFakeGoalFeetYaw[record->player->index()] = record->anim_state.current_feet_yaw;
	else if (record->velocity.length2d() > 4 && fabs(record->lby - record->eye_angles.y) > 45)
		m_flFakeGoalFeetYaw[record->player->index()] = record->lby;
	else if (record->velocity.length2d() > 2 && record->velocity.length2d() < 10 && fabs(record->lby - record->eye_angles.y) < 35)
	{
		if (flEyeGoalDelta <= flMaxBodyYaw) {
			if (flMinBodyYaw > flEyeGoalDelta)
				m_flFakeGoalFeetYaw[record->player->index()] = record->eye_angles.y - 22; //  45 / 2
		}
		else {
			m_flFakeGoalFeetYaw[record->player->index()] = record->eye_angles.y - 22;     //  45 / 2
		}
	}
	else if (record->velocity.length2d() > 15)
	{
		if (flEyeGoalDelta <= flMaxBodyYaw) {
			if (flMinBodyYaw > flEyeGoalDelta)
				m_flFakeGoalFeetYaw[record->player->index()] = record->eye_angles.y;
		}
		else {
			m_flFakeGoalFeetYaw[record->player->index()] = record->eye_angles.y;
		}
	}
	else if (record->velocity.length2d() < 1)
	{
		if (flEyeGoalDelta <= flMaxBodyYaw) {
			if (flMinBodyYaw > flEyeGoalDelta)
				m_flFakeGoalFeetYaw[record->player->index()] = fabs(flMinBodyYaw) - flEyeYaw;
		}
		else {
			m_flFakeGoalFeetYaw[record->player->index()] = flEyeYaw + fabs(flMaxBodyYaw);
		}
	}


	if (speed > 0.1f || fabs(flAbsVelocity) > 100.0f) {
	
	}
	else {
		m_flFakeGoalFeetYaw[record->player->index()] = ApproachAngle(
			record->lby,
			m_flFakeGoalFeetYaw[record->player->index()],
			deltatime * 100.0f); // fix time deltatime
	}

	/*switch (info->brute_state)
	{
		case resolver_start:
			resolver_yaw = m_flFakeGoalFeetYaw[record->player->index()];
			break;
		case resolver_brute_left:
			resolver_yaw = m_flFakeGoalFeetYaw[record->player->index()];
			break;
		case resolver_brute_right:
			if (info->brute_yaw == flEyeYaw)
				resolver_yaw = Left;
			else if (info->brute_yaw == Left)
				resolver_yaw = Right;
			else if (info->brute_yaw == Right)
				resolver_yaw = Left;
			else
				resolver_yaw = flEyeYaw;
			break;
		default:
			break;
	}*/

	info->brute_yaw = math::normalize_yaw(m_flFakeGoalFeetYaw[record->player->index()]);
	record->anim_state.goal_feet_yaw = info->brute_yaw;

    if (record->eye_angles.x == -540.f || record->eye_angles.x == -1620.f)
			record->eye_angles.x = 88.8;

	const auto current_animstate = record->player->get_anim_state();
	if (current_animstate)
		current_animstate->goal_feet_yaw = record->anim_state.goal_feet_yaw;
}

const char* GetHitgroupName12(int hitgroup)
{
	switch (hitgroup)
	{
	case 1:
		return "Head";
	case 2:
		return "Chest";
	case 3:
		return "Pelvis";
	case 4:
		return "L Arm";
	case 5:
		return "R Arm";
	case 6:
		return "L Leg";
	case 7:
		return "R Leg";
	default:
		return "unknown";
	}
}
void c_resolver::register_shot(resolver_shot::shot&& s)
{
	shots.emplace_back(std::move(s));
}

int OppositeColor()
{
	auto local = c_cs_player::get_local_player();

	if (!local)
		return 0;

	for (int i = 1; i < engine_client()->get_max_clients(); i++)
	{
		auto player = (c_cs_player*)client_entity_list()->get_client_entity(i);

		if (!player || player == local)
			continue;

		if (player->get_team() != local->get_team())
		{
			return player->index();
		}
	}
	return 0;
}

void c_resolver::on_player_hurt(c_game_event* event)
{
	if (!event || !config->Ragebot.enable)
		return;

	auto local = c_cs_player::get_local_player();

	if (!local || !local->is_alive())
		return;

	const auto attacker = client_entity_list()->get_client_entity(
		engine_client()->get_player_for_user_id(event->GetInt(_("attacker"))));

	const auto target = reinterpret_cast<c_cs_player*>(client_entity_list()->get_client_entity(
		engine_client()->get_player_for_user_id(event->GetInt(_("userid")))));

	if (shots.empty())
		return;

	if (attacker && target && attacker == local && target->is_enemy())
	{
		int user_id = engine_client()->get_player_for_user_id(event->GetInt("userid"));
		int hitgroup = event->GetInt("hitgroup");

		player_info pInfo;
		engine_client()->get_player_info(user_id, &pInfo);

		//_events.push_back(_event(msg));

		FEATURES::MISC::InGameLogger::Log log;

		std::string name = pInfo.name;
		std::transform(name.begin(), name.end(), name.begin(), ::toupper);

		log.color_line.PushBack("[Sequency] ", c_color(0, 255, 0, 255));
		log.time = global_vars_base->curtime;
		log.color_line.PushBack("-", c_color(255, 255, 255, 255));
		log.color_line.PushBack(std::to_string(event->GetInt("dmg_health")), c_color(255, 255, 255, 255));
		log.color_line.PushBack(" in ", c_color(255, 255, 255, 255));
		log.color_line.PushBack(GetHitgroupName12(hitgroup), c_color(255, 255, 255, 255));
		log.color_line.PushBack(" to ", c_color(255, 255, 255, 255));
		log.color_line.PushBack(name, c_color(255, 255, 255, 255));

		FEATURES::MISC::in_game_logger.AddLog(log);
		std::string info = "=====================================";

	

		unit::g_ChatElement->ChatPrintf(1, 0, info.c_str());


		std::string type = "[Sequency]: ";
		std::string text2 = type + "Hit " + std::to_string(event->GetInt("dmg_health"))  + " in " + GetHitgroupName12(hitgroup);

		unit::g_ChatElement->ChatPrintf(32, 0, text2.c_str());

		unit::g_ChatElement->ChatPrintf(1, 0, info.c_str());

		//int index = target->index();
	//	resolver.missed_due_to_bad_resolve[index]--;
		//resolver.missed_due_to_bad_resolve[index] =
		//	std::clamp(resolver.missed_due_to_bad_resolve[index], 0, 99);
	}

	//const auto attacker2 = event->GetInt(_("attacker"));
	//const auto attacker_index = engine_client()->get_player_for_user_id(attacker2);

	if (attacker != local)
		return;

	if (target == nullptr)
		return;

	//const auto userid = event->GetInt(_("userid"));
	//const auto index = engine_client()->get_player_for_user_id(userid);

	//c_cs_player* entity = reinterpret_cast<c_cs_player*>(client_entity_list()->get_client_entity(index));

	resolver_shot::shot* last_confirmed = nullptr;

	for (auto it = shots.rbegin(); it != shots.rend(); it = next(it))
	{
		if (it->confirmed && !it->skip)
		{
			last_confirmed = &*it;
			break;
		}
	}

	if (!last_confirmed)
		return;

	if (target->index() != last_confirmed->record.index)
		return;

	last_confirmed->server_info.index = target->index();
	last_confirmed->server_info.damage = event->GetInt(_("dmg_health"));
	last_confirmed->server_info.hitgroup = event->GetInt(_("hitgroup"));
}

float  c_resolver::GetCurtime2222()
{
	const auto local = c_cs_player::get_local_player();
	return static_cast<float>(local->get_tick_base()) * global_vars_base->interval_per_tick;
}

void c_resolver::on_bullet_impact(c_game_event* event)
{
	const auto userid = event->GetInt(_("userid"));
	const auto index = engine_client()->get_player_for_user_id(userid);

	if (index != engine_client()->get_local_player())
		return;

	if (shots.empty())
		return;

	resolver_shot::shot* last_confirmed = nullptr;

	for (auto it = shots.rbegin(); it != shots.rend(); it = next(it))
	{
		if (it->confirmed && !it->skip)
		{
			last_confirmed = &*it;
			break;
		}
	}

	if (!last_confirmed)
		return;

	last_confirmed->impacted = true;
	last_confirmed->server_info.impacts.emplace_back(event->GetFloat(_("x")),
		event->GetFloat(_("y")),
		event->GetFloat(_("z")));

	//last_confirmed->time = global_vars_base->curtime;
	//last_confirmed->end = last_confirmed->server_info.impacts.back();
}

void c_resolver::on_weapon_fire(c_game_event* event)
{
	const auto userid = event->GetInt(_("userid"));
	const auto index = engine_client()->get_player_for_user_id(userid);

	if (index != engine_client()->get_local_player())
		return;

	if (shots.empty())
		return;

	resolver_shot::shot* last_unconfirmed = nullptr;

	for (auto it = shots.rbegin(); it != shots.rend(); it = next(it))
	{
		if (!it->confirmed)
		{
			last_unconfirmed = &*it;
			break;
		}

		it->skip = true;
	}

	if (!last_unconfirmed)
		return;

	last_unconfirmed->confirmed = true;
}


void c_resolver::on_round_start(c_game_event* event)
{
	resolver.last_shot_missed_index = 0;
	resolver.has_target = false;

	std::fill(resolver.missed_due_to_bad_resolve, resolver.missed_due_to_bad_resolve +
		ARRAYSIZE(resolver.missed_due_to_bad_resolve), 0);

	std::fill(resolver.missed_due_to_spread, resolver.missed_due_to_spread +
		ARRAYSIZE(resolver.missed_due_to_spread), 0);
}

void c_resolver::on_player_death(c_game_event* event)
{
	if (!event || !config->Ragebot.enable)
		return;

	auto local = c_cs_player::get_local_player();

	if (!local || !local->is_alive())
	{
		resolver.last_shot_missed_index = 0;
		return;
	}

	int victim_id = event->GetInt(_("userid"));
	int killer_id = event->GetInt(_("attacker"));

	bool is_headshot = event->GetInt(_("headshot"));

	int victim_index = engine_client()->get_player_for_user_id(victim_id);
	int killer_index = engine_client()->get_player_for_user_id(killer_id);

	if (victim_index && killer_index && local)
	{
		//resolver.missed_due_to_bad_resolve[victim_index] = 0;
		resolver.missed_due_to_spread[victim_index] = 0;
	}
}

bool c_resolver::get_target_freestand_yaw(c_cs_player* target, float* yaw)
{
	float dmg_left = 0.f;
	float dmg_right = 0.f;

	static auto get_rotated_pos = [](c_vector3d start, float rotation, float distance)
	{
		float rad = deg2rad(rotation);
		start.x += cos(rad) * distance;
		start.y += sin(rad) * distance;

		return start;
	};

	const auto local = c_cs_player::get_local_player();

	if (!local || !target || !local->is_alive())
		return false;

	c_vector3d local_eye_pos = local->get_shoot_position();
	c_vector3d eye_pos = target->get_shoot_position();
	c_qangle angle = math::calc_angle(local_eye_pos, eye_pos);

	auto backwards = angle.y;

	c_vector3d pos_left = get_rotated_pos(eye_pos, angle.y + 90.f, 40.f);
	c_vector3d pos_right = get_rotated_pos(eye_pos, angle.y - 90.f, -40.f);

	const auto wall_left = trace_system->wall_penetration(local_eye_pos, pos_left,
		nullptr, nullptr, local);

	const auto wall_right = trace_system->wall_penetration(local_eye_pos, pos_right,
		nullptr, nullptr, local);

	if (wall_left.has_value())
		dmg_left = wall_left.value().damage;

	if (wall_right.has_value())
		dmg_right = wall_right.value().damage;

	// we can hit both sides, lets force backwards
	if (fabsf(dmg_left - dmg_right) < 10.f)
	{
		*yaw = backwards;
		return false;
	}

	bool direction = dmg_left > dmg_right;
	*yaw = direction ? angle.y - 90.f : angle.y + 90.f;

	return true;
}

bool c_resolver::get_enemy_freestand_yaw(c_cs_player* target, float* yaw)
{
	float dmg_left = 0.f;
	float dmg_right = 0.f;

	static auto get_rotated_pos = [](c_vector3d start, float rotation, float distance)
	{
		float rad = deg2rad(rotation);
		start.x += cos(rad) * distance;
		start.y += sin(rad) * distance;

		return start;
	};

	const auto local = c_cs_player::get_local_player();

	if (!local || !target || !local->is_alive())
		return false;

	c_vector3d local_eye_pos = target->get_shoot_position();
	c_vector3d eye_pos = local->get_shoot_position();
	c_qangle angle = math::calc_angle(local_eye_pos, eye_pos);

	auto backwards = target->get_eye_angles().y; // angle.y;

	c_vector3d pos_left = get_rotated_pos(eye_pos, angle.y + 90.f, 40.f);
	c_vector3d pos_right = get_rotated_pos(eye_pos, angle.y - 90.f, -40.f);

	const auto wall_left = trace_system->wall_penetration(local_eye_pos, pos_left,
		nullptr, nullptr, local);

	const auto wall_right = trace_system->wall_penetration(local_eye_pos, pos_right,
		nullptr, nullptr, local);

	if (wall_left.has_value())
		dmg_left = wall_left.value().damage;

	if (wall_right.has_value())
		dmg_right = wall_right.value().damage;

	if (dmg_left == 0.f && dmg_right == 0.f)
	{
		*yaw = backwards;
		return false;
	}

	// we can hit both sides, lets force backwards
	if (fabsf(dmg_left - dmg_right) < 10.f)
	{
		*yaw = backwards;
		return false;
	}

	bool direction = dmg_left > dmg_right;
	*yaw = direction ? angle.y - 90.f : angle.y + 90.f;

	return true;
}

float c_resolver::get_max_desync_delta(c_cs_player* player)
{
	if (!player || !player->is_alive() || player->is_dormant())
		return 0.f;

	auto animstate = uintptr_t(player->get_anim_state());
	if (!animstate)
		return 0.f;

	float duckammount = *(float*)(animstate + 0xA4);
	float speedfraction = std::max<float>(0, std::min(*reinterpret_cast<float*>(animstate + 0xF8), 1.f));
	float speedfactor = speedfraction >= 0.f ? std::max<float>(0, std::min(1.f, *reinterpret_cast<float*> (animstate + 0xFC))) : speedfactor = 0.f;

	float unk1 = ((*reinterpret_cast<float*> (animstate + 0x11C) * -0.30000001) - 0.2) * speedfraction;
	float unk2 = unk1 + 1.f;

	if (duckammount > 0)
		unk2 += +((duckammount * speedfactor) * (0.5f - unk2));

	auto retvalue = *(float*)(animstate + 0x334) * unk2;

	if (retvalue > 1.f) // account for rounding errors
		retvalue -= 1.f;

	retvalue = std::clamp(retvalue, 0.f, 60.f); // 28.f, 60.f
	return retvalue;
}

void c_resolver::on_render_start()
{
	if (shots.empty())
		return;

	for (auto it = shots.begin(); it != shots.end();)
	{
		if (it->time + 1.f < global_vars_base->curtime) // 1.0
			it = shots.erase(it);
		else
			it = next(it);
	}

	for (auto it = shots.begin(); it != shots.end();)
	{
		if (it->confirmed && it->impacted)
		{
			if (!it->delayed) it->delayed = true;
			else
			{
				resolve_shot(*it);
				//c_esp::draw_local_impact(it->start, it->server_info.impacts.back());
				it = shots.erase(it);
			}
		}
		else
			it = next(it);
	}
}

void c_resolver::resolve_shot(resolver_shot::shot& shot)
{
	if (!config->Ragebot.enable || shot.manual)
		return;

	//if (!config.rage.enabled || shot.manual)
		//return;

	const auto player = reinterpret_cast<c_cs_player*>(client_entity_list()->get_client_entity(shot.record.index));

	if (player != shot.record.player)
		return;

	//if (config.esp.enemy.show_on_shot_hitboxes)
		//shot.record.player->draw_hitboxes(shot.record.bones);

	const auto hdr = model_info_client()->get_studio_model(shot.record.player->get_model());

	if (!hdr)
		return;

	const auto info = animation_system->get_animation_info(player);

	if (!info)
		return;

	const auto angle = math::calc_angle(shot.start, shot.server_info.impacts.back());
	c_vector3d forward;
	math::angle_vectors(angle, forward);
	const auto end = shot.server_info.impacts.back() + forward * 2000.f;

	auto does_match = c_aimhelper::can_hit_hitbox(shot.start, end, &shot.record, hdr, shot.hitbox);

	// check deviation from server.
	auto backup = c_animation_system::animation(shot.record.player);
	shot.record.apply(player);
	const auto trace = trace_system->wall_penetration(shot.start, end, &shot.record);

	if (shot.server_info.damage > 0)
	{
		player->draw_hitboxes(shot.record.bones, 5.0, 0, 255, 0, 255);
	}
	else if (!does_match)
	{
		static const auto hit_msg = __("Missed shot due to SPREAD. The target: %s");
		_rt(hit, hit_msg);
		char msg[255];
		sprintf_s(msg, hit, player->get_info().name);

		logging->info(msg);

		resolver.missed_due_to_spread[player->index()]++;
		info->missed_due_to_spread = resolver.missed_due_to_spread[player->index()];

		FEATURES::MISC::InGameLogger::Log log;
		log.color_line.PushBack("[InterPredict] ", c_color(152, 82, 251, 255));
		log.color_line.PushBack("SHOT MISSED DUE TO SPREAD", c_color(255, 255, 255, 255));
		log.time = global_vars_base->curtime;
		FEATURES::MISC::in_game_logger.AddLog(log);

		player->draw_hitboxes(shot.record.bones, 5.0, 255, 255, 0, 255);
	}
	else if (!trace.has_value())
	{
		does_match = false;
		FEATURES::MISC::InGameLogger::Log log;
		log.color_line.PushBack("[InterPredict] ", c_color(152, 82, 251, 255));
		log.color_line.PushBack("SHOT MISSED DUE TO OCCLUSION", c_color(255, 255, 255, 255));
		log.time = global_vars_base->curtime;
		FEATURES::MISC::in_game_logger.AddLog(log);

		player->draw_hitboxes(shot.record.bones, 5.0, 255, 0, 0, 255);
	}
	else if (does_match && shot.server_info.damage <= 0 && shot.damage > 0)
	{
		if (trace.has_value())
		{
			static const auto hit_msg = __("Missed shot due to RESOLVER.");
			_rt(hit, hit_msg);
			char msg[255];
			sprintf_s(msg, hit, player->get_info().name);
			logging->info(msg);
		}
		else
		{
			logging->info("Missed shot due to occlusion");
			does_match = false;
		}

		resolver.missed_due_to_bad_resolve[player->index()]++;
	
		FEATURES::MISC::InGameLogger::Log log;
		log.color_line.PushBack("[InterPredict] ", c_color(152, 82, 251, 255));
		log.time = global_vars_base->curtime;
		log.color_line.PushBack("SHOT MISSED DUE TO RESOLVER", c_color(255, 255, 255, 255));
		FEATURES::MISC::in_game_logger.AddLog(log);

		player->draw_hitboxes(shot.record.bones, 5.0, 0, 0, 255, 255);
	}

	//if (does_match || shot.server_info.damage > 0)
		//info->missed_due_to_spread = 0;

	if (!shot.record.player->is_alive() || shot.record.player->get_info().fakeplayer
		|| !shot.record.has_anim_state || !shot.record.player->get_anim_state() || !info)
		return;

	// note old brute_yaw.
	const auto old_brute_yaw = info->brute_yaw;

	// start brute.
	if (does_match && shot.server_info.damage <= 0 && shot.damage > 0)
	{
		switch (info->brute_state)
		{
			case resolver_start:
				info->brute_state = resolver_brute_left;
				logging->debug("resolver: brute left side");
				break;
			case resolver_brute_left:
				info->brute_state = resolver_brute_right;
				logging->debug("resolver: brute right side");
				break;
			case resolver_brute_right:
				info->brute_state = resolver_start;
				logging->debug("resolver: brute middle side");
				break;
			default:
				info->brute_state = resolver_start;
				break;
		}

		resolve(&shot.record);
	}

	// apply changes.
	if (!info->frames.empty())
	{
		c_animation_system::animation* previous = nullptr;

		// jump back to the beginning.
		*player->get_anim_state() = info->frames.back().anim_state;

		for (auto it = info->frames.rbegin(); it != info->frames.rend(); ++it)
		{
			auto& frame = *it;
			const auto frame_player = reinterpret_cast<c_cs_player*>(
				client_entity_list()->get_client_entity(frame.index));

			if (frame_player == frame.player
				&& frame.player == player)
			{
				// re-run complete animation code and repredict all animations in between!
				frame.anim_state = *player->get_anim_state();
				frame.apply(player);
				player->get_flags() = frame.flags;
				*player->get_animation_layers() = frame.layers;
				player->get_simtime() = frame.sim_time;

				info->update_animations(&frame, previous);
				frame.abs_ang.y = player->get_anim_state()->goal_feet_yaw;
				frame.flags = player->get_flags();
				*player->get_animation_layers() = frame.layers;
				frame.build_server_bones(player);
				previous = &frame;
			}
		}
	}
}
