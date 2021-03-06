#include "c_client_state_.h"
#include "../utils/c_hook.h"
#include "../utils/c_config.h"
#include "c_prediction_.h"
#include "../sdk/c_cvar.h"
#include "../sdk/c_game_rules.h"
#include "c_events.h"
#include "../new_menu/config.h"
#include "../menu/c_menu.h"

void c_client_state_::hook()
{
	static c_hook<c_client_state> hook(reinterpret_cast<c_client_state*>(uint32_t(client_state) + 8));
	_temp_entites = hook.apply<temp_entities_t>(36, temp_entites);
	_packet_start = hook.apply<packet_start_t>(5, packet_start);
}


void __fastcall c_client_state_::packet_start(c_client_state* state, uint32_t, int incoming_sequence, int outgoing_acknowledged)
{

	const auto local = c_cs_player::get_local_player();

	if (!local || !local->is_alive() || !engine_client()->is_ingame())
		return _packet_start(state, incoming_sequence, outgoing_acknowledged);

	for (auto it = cmds.begin(); it != cmds.end(); it++)
		if (*it == outgoing_acknowledged)
		{
			cmds.erase(it);
			return _packet_start(state, incoming_sequence, outgoing_acknowledged);
		}
	

	return _packet_start(state, incoming_sequence, outgoing_acknowledged);
}


bool __fastcall c_client_state_::temp_entites(c_client_state* state, uint32_t, void* msg)
{
	bool ret = _temp_entites(state, msg);

	const auto local = c_cs_player::get_local_player();

	if (!local || !local->is_alive() || !engine_client()->is_ingame())
		return ret;

	for (c_event_info* it{ client_state->events }; it != nullptr; it = it->m_next)
	{
		if (!it->m_class_id)
			continue;

		// lets fire all events right away.
		it->m_fire_delay = 0.f;
	}

	return ret;
}