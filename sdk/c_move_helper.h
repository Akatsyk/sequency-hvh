#pragma once
#include "../utils/c_memory.h"
#include "c_base_entity.h"
#include "c_base_player.h"
typedef int entity_handle_t, soundlevel_t;
class c_move_helper
{
protected:
	~c_move_helper() = default;
public:
	static c_move_helper* get()
	{
		static const auto move_helper = **reinterpret_cast<c_move_helper***>(
			reinterpret_cast<uint32_t>(sig("client.dll", "8B 0D ? ? ? ? 8B 46 08 68")) + 2);
		return move_helper;
	}
	

private:
	virtual void unknown() = 0;
public:
	VFUNC(set_host2(c_base_player* player), 1, void(__thiscall*)(void*, c_base_player*), player)
	VFUNC(process_impacts(), 4, void(__thiscall*)(void*))

	bool m_first_run_of_functions : 1;
	bool m_game_code_moved_player : 1;
	entity_handle_t	m_player_handle;
	int m_impulse_command;
	c_vector3d m_vec_view_angles;
	c_vector3d m_vec_abs_view_angles;
	int m_buttons;
	int m_old_buttons;
	float m_forward_move;
	float m_side_move;
	float m_up_move;
	float m_max_speed;
	float m_client_max_speed;
	c_vector3d m_vec_velocity;
	c_vector3d m_vec_angles;
	c_vector3d m_vec_old_angles;
	float m_out_step_height;
	c_vector3d m_out_wish_vel;
	c_vector3d m_out_jump_vel;
	c_vector3d m_vec_constraint_center;
	float m_constraint_radius;
	float m_constraint_width;
	float m_constraint_speed_factor;
	c_vector3d m_vec_abs_origin;
	virtual void set_host(c_base_entity* host) = 0;
private:
	virtual void unknown1() = 0;
	virtual void unknown2() = 0;
public:
	virtual bool unknown_func() = 0;
};



#define move_helper c_move_helper::get()

