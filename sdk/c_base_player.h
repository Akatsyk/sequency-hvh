#pragma once

#include "c_base_combat_character.h"
#include "c_user_cmd.h"

class c_base_player : public c_base_combat_character
{
public:
	enum flags
	{
		on_ground = 1 << 0,
		ducking = 1 << 1,
		water_jump = 1 << 2,
		on_train = 1 << 3,
		in_rain = 1 << 4,
		frozen = 1 << 5,
		at_controls = 1 << 6,
		client = 1 << 7,
		fake_client = 1 << 8,
		in_water = 1 << 9,
		fly = 1 << 10,
		swim = 1 << 11,
		conveyor = 1 << 12,
		npc = 1 << 13,
		god_mode = 1 << 14,
		no_target = 1 << 15,
		aim_target = 1 << 16,
		partial_ground = 1 << 17,
		static_prop = 1 << 18,
		graphed = 1 << 19,
		grenade = 1 << 20,
		step_movement = 1 << 21,
		dont_touch = 1 << 22,
		base_velocity = 1 << 23,
		world_brush = 1 << 24,
		object = 1 << 25,
		kill_me = 1 << 26,
		on_fire = 1 << 27,
		dissolving = 1 << 28,
		tansragdoll = 1 << 29,
		unblockable_by_player = 1 << 30
	};

	bool is_on_ground()
	{
		return get_flags() & on_ground || get_flags() & partial_ground || get_flags() & conveyor;
	}

	bool is_grounded(std::optional<int32_t> flags = std::nullopt)
	{
		return flags.value_or(get_flags()) & on_ground;
	}

	//VFUNC(set_local_view_angles(c_qangle& angle), 372, void(__thiscall*)(void*, c_qangle&), angle)
	vfunc(372, set_local_view_angles(c_qangle& angle), void(__thiscall*)(void*, c_qangle&))(angle)
	netvar(get_punch_angle(), c_vector3d, "CBasePlayer", "m_aimPunchAngle")
	datamap(get_impulse(), int, "m_nImpulse")
	datamap(get_buttons(), int, "m_nButtons")
	datamap(get_buttons_last(), int, "m_afButtonLast")
	datamap(get_buttons_pressed(), int, "m_afButtonPressed")
	datamap(get_buttons_released(), int, "m_afButtonReleased")
	netvar(get_fall_velocity(), float, "CBasePlayer", "m_flFallVelocity")
	netvar(get_sequence(), int, "CBaseAnimating", "m_nSequence")
	netvar(get_flags(), int32_t, "CBasePlayer", "m_fFlags")
	netvar_offset(get_third_person_angles(), c_qangle, "CBasePlayer", "deadflag", 4)
	netvar_offset(get_cur_cmd(), c_user_cmd*, "CBasePlayer", "m_hConstraintEntity", -0xC)
	netvar(get_tick_base(), int32_t, "CBasePlayer", "m_nTickBase")
	netvar(m_nHitboxSet(), int32_t, "CBasePlayer", "m_nHitboxSet")
	netvar(get_view_model(), c_base_handle, "CBasePlayer", "m_hViewModel[0]")
	netvar(get_velocity(), c_vector3d, "CBasePlayer", "m_vecVelocity[0]")
	netvar(m_hObserverTarget(), c_base_handle, "CBasePlayer", "m_hObserverTarget")
	netvar(get_lifestate(), int32_t, "CBasePlayer", "m_lifeState")

	datamap(get_ground_entity(), c_base_handle, "m_hGroundEntity")

	netvar(get_vehicle(), c_base_handle, "CBasePlayer", "m_hVehicle")
	netvar(get_next_think_tick(), int, "CBasePlayer", "m_nNextThinkTick")
	VFUNC_SIG(unknown_think(int unk), "client.dll", "55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B C6 C1 E8 16 24 01 74 18", void(__thiscall*)(void*, int), unk)
	VFUNC_SIG(physics_run_think(int index), "client.dll", "55 8B EC 83 EC 10 53 56 57 8B F9 8B 87", bool(__thiscall*)(void*, int), index)
	VFUNC_SIG(using_standard_weapons_in_vehicle(), "client.dll", "56 57 8B F9 8B 97 ? ? ? ? 83 FA FF 74 41", bool(__thiscall*)(void*))
	//VFUNC(select_item(const char* name, int sub_type), 329, void(__thiscall*)(void*, const char*, int), name, sub_type)
	vfunc(329, select_item(const char* name, int sub_type), void(__thiscall*)(void*, const char*, int))(name, sub_type)

	bool is_alive()
	{
		return get_lifestate() == 0;
	}
};
