#include "c_prediction_.h"
#include "../utils/c_hook.h"
#include "c_client.h"
#include "../hacks/c_prediction_system.h"
#include "../hacks/c_antiaim.h"
#include "../sdk/c_input.h"
#include "../sdk/c_base_view_model.h"
#include "../utils/math.h"
#include <intrin.h>
namespace helper
{
	dt* g_pViolanes = new dt();
	void loading(c_user_cmd* m_pCmd)
	{
		if (!engine_client()->is_connected() && !engine_client()->is_ingame())
			return;
		const auto local_player = c_cs_player::get_local_player();
		if (!local_player) return;

		c_base_combat_weapon* weapon = reinterpret_cast<c_base_combat_weapon*>(local_player->get_current_weapons());
		if (!weapon) return;
		g_pViolanes->g_flNextSecondaryAttack = weapon->get_next_primary_attack();
		g_pViolanes->g_iTickcount = m_pCmd->tick_count;
		g_pViolanes->g_iTickrate = 1.0f / global_vars_base->interval_per_tick;
		g_pViolanes->g_nLastShift = 0;
		g_pViolanes->g_nPlayerTickbase = local_player->get_tick_base();
		g_pViolanes->g_flVelocityModifier = local_player->get_velocity_modifier();
	}
}

void c_prediction_::hook()
{
	static c_hook<c_prediction> hook(prediction());
	_in_prediction = hook.apply<in_prediction_t>(14, in_prediction);
	_run_command = hook.apply<rucmd_t>(19, RunCommand);
}

bool __fastcall c_prediction_::in_prediction(c_prediction* prediction, uint32_t)
{
	static const auto return_to_maintain_sequence_transitions =
		sig("client.dll", "84 C0 74 17 8B 87");

	static auto ptr_setupbones = sig("client.dll", "8B 40 ? FF D0 84 C0 74 ? F3 0F 10 05 ? ? ? ? EB ?");




	if (!engine_client()->is_connected() && !engine_client()->is_ingame())
		return _in_prediction(prediction);

	if (config->Ragebot.enable) {
		if (*reinterpret_cast<uintptr_t*> (_ReturnAddress()) == *reinterpret_cast<uintptr_t*> (ptr_setupbones) + 5)
			return false; // we don't want to predict in setupbones
	}


	return _in_prediction(prediction);
}


void FixAttackPacket(c_user_cmd* m_pCmd, bool m_bPredict)
{
	static bool m_bLastAttack = false;
	static bool m_bInvalidCycle = false;
	static float m_flLastCycle = 0.f;
	if (!c_cs_player::get_local_player()) return;
	auto animoverlay = c_cs_player::get_local_player()->get_animation_layers()->at(1);


	if (m_bPredict)
	{
		m_bLastAttack = m_pCmd->weapon_select || (m_pCmd->buttons & c_user_cmd::attack2);
		m_flLastCycle = animoverlay.cycle;
	}
	else if (m_bLastAttack && !m_bInvalidCycle)
		m_bInvalidCycle = animoverlay.cycle == 0.f && m_flLastCycle > 0.f;
}

void __fastcall c_prediction_::RunCommand(void* ecx, void* edx, c_cs_player* player, c_user_cmd* ucmd, void* m_pMoveHelper)
{
	const auto local_player = c_cs_player::get_local_player();
	if (!player || !local_player || player != local_player)
		return _run_command(ecx, player, ucmd, m_pMoveHelper);

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(
		client_entity_list()->get_client_entity_from_handle(local_player->get_current_weapon_handle()));

	static int old_tickbase = 0;

	if (player->is_local_player())
	{
		/*if (helper::g_pViolanes->g_iTickcount + helper::g_pViolanes->g_iTickrate + 8 <= ucmd->tick_count)
		{
			ucmd->predicted = true;
			return;
		}
		*/
		if (1.f / global_vars_base->interval_per_tick + helper::g_pViolanes->g_iTickcount + 8 <= ucmd->tick_count)
		{
			ucmd->predicted = true;
			return;
		}

		int m_nTickbase = player->get_tick_base();
		float m_flCurtime = global_vars_base->curtime;
		float m_flVelModBackup = player->get_velocity_modifier();

		if (ucmd->command_number == helper::g_pViolanes->g_nShotCmd)
		{
			local_player->get_tick_base() = helper::g_pViolanes->g_nPlayerTickbase - helper::g_pViolanes->g_nLastShift + 1;
			//global_vars_base->curtime = player->get_tick_base() * global_vars_base->interval_per_tick;
			global_vars_base->curtime = ticks_to_time(player->get_tick_base());
		}


		/*if (weapon) {
			static int old_activity = weapon->get_activity();
			const auto tickbase = player->get_tick_base() - 1;
			auto activity = weapon->get_activity();

			if (weapon->get_item_definition() == 64 && !ucmd->predicted) {
				if (old_activity != activity && weapon->get_activity() == 208)
					old_tickbase = tickbase + 2;

				if (weapon->get_activity() == 208 && old_tickbase == tickbase)
					weapon->get_postpone_fire_ready_time() = time_to_ticks(tickbase) + 0.2f;
			}
			old_activity = activity;
		}*/

		//	FixAttackPacket(ucmd, true);


		if (helper::g_pViolanes->g_bOverrideModifier && ucmd->command_number == client_state->last_command_ack + 1)
			player->get_velocity_modifier() = helper::g_pViolanes->g_flVelocityModifier;

		_run_command(ecx, player, ucmd, m_pMoveHelper);

		if (ucmd->command_number == helper::g_pViolanes->g_nShotCmd)
		{
			local_player->get_tick_base() = m_nTickbase;
			global_vars_base->curtime = m_flCurtime;
		}

		if (!helper::g_pViolanes->g_bOverrideModifier)
			player->get_velocity_modifier() = m_flVelModBackup;


		//	player->m_vphysicsCollisionState() = false;
		//	FixAttackPacket(ucmd, false);

		antiaim->predict(player, ucmd);
	}
}

