#pragma once

#include "macros.h"
#include "c_move_helper.h"
#include "c_game_movement.h"
#include "c_global_vars_base.h"

class c_prediction

{
public:
	virtual ~c_prediction(void) {};

	virtual void init(void) = 0;
	virtual void shutdown(void) = 0;

	// Run prediction
	virtual void update
	(
		int start_frame,            // world update ( un-modded ) most recently received
		bool valid_frame,			// is frame data valid
		int incoming_acknowledged,	// last command acknowledged to have been run by server (un-modded)
		int outgoing_command		// last command (most recent) sent to server (un-modded)
	) = 0;

	// we are about to get a network update from the server.  we know the update #, so we can pull any
	// data purely predicted on the client side and transfer it to the new from data state.
	virtual void pre_entity_packet_received(int commands_acknowledged, int current_world_update_packet) = 0;
	virtual void post_entity_packet_received(void) = 0;
	virtual void post_network_data_received(int commands_acknowledged) = 0;

	virtual void on_received_uncompressed_packet(void) = 0;

	// the engine needs to be able to access a few predicted values

public:

	vfunc(371, set_local_view_angles(c_qangle& angle), void(__thiscall*)(c_prediction*, c_qangle&))(angle)
	vfunc(14, in_prediction(), bool(__thiscall*)(c_prediction*))()
		VFUNC(check_moving_on_ground(c_base_player* player, float frame_time), 18, void(__thiscall*)(void*, c_base_player*, double), player, frame_time)

	vfunc(20, setup_move(c_base_entity* player, c_user_cmd* cmd, c_move_helper* helper, c_move_data* data),
		void(__thiscall*)(c_prediction*, c_base_entity*, c_user_cmd*, c_move_helper*, c_move_data*))(player, cmd, helper, data)
	vfunc(21, finish_move(c_base_entity* player, c_user_cmd* cmd, c_move_data* data),
		void(__thiscall*)(c_prediction*, c_base_entity*, c_user_cmd*, c_move_data*))(player, cmd, data)

	inline c_global_vars_base* get_unpredicted_globals() {
		if (in_prediction())
			return reinterpret_cast<c_global_vars_base*>(uint32_t(this) + 0x4c);

		return global_vars_base;
	}

	char pad0[8];
	bool m_in_prediction;
	char pad1[1];
	bool m_engine_paused;
	char pad2[13];
	bool m_first_time_predicted;
	char pad3[7];
	int	 m_server_commands_acknowledged;
	bool m_had_prediction_errors;

};

interface_var(c_prediction, prediction, "client.dll", "VClientPrediction")



class i_mdl_cache {
private:

};

interface_var(i_mdl_cache, model_cache, "datacache.dll", "MDLCache")
