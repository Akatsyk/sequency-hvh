#include "c_client.h"
#include "../utils/c_hook.h"
#include "../hacks/c_animation_system.h"
#include "../sdk/c_client_entity_list.h"
#include "../sdk/c_game_rules.h"
#include "../sdk/c_weapon_system.h"
#include "../hacks/c_esp.h"
#include "c_net_channel_.h"
#include "../hacks/c_movement.h"
#include "../hacks/c_resolver.h"
#include "../hacks/debug.h"
#include "../hacks/c_miscellaneous.h"
#include "../hacks/c_ragebot.h"
#include "c_client.h"
#include "../hacks/c_prediction_system.h"
#include "../sdk/c_input.h"
#include "../hacks/c_antiaim.h"
#include "../sdk/c_client_state.h"
#include "../sdk/c_prediction.h"
#include "../sdk/c_base_view_model.h"
#include "../hacks/c_legitbot.h"
#include "c_client_state_.h"
#include "c_events.h"
#include "c_cl_camera_height_restriction_debug.h"
#include "../hacks/skinchanger/skin_changer.h"
#include "../main.h"
#include "c_prediction_.h"
#include "../idaboss.h"
#include "../baim.h"

#include "../LuaAPI.h"

void c_client::hook()
{
	static c_hook<c_base_client> hook(base_client());
	_shutdown = hook.apply<shutdown_t>(4, shutdown);
	_level_init_pre_entity = hook.apply<level_init_pre_entity_t>(5, level_init_pre_entity);
	_create_move = hook.apply<create_move_t>(22, ::create_move);
	_frame_stage_notify = hook.apply<frame_stage_notify_t>(37, frame_stage_notify);

	cvar()->find_var("viewmodel_fov")->null_callback();
	cvar()->find_var("viewmodel_offset_x")->null_callback();
	cvar()->find_var("viewmodel_offset_y")->null_callback();
	cvar()->find_var("viewmodel_offset_z")->null_callback();
	cvar()->find_var("r_aspectratio")->null_callback();
	cvar()->find_var("sv_showimpacts")->null_callback();
	cvar()->find_command(_("clear"))->dispatch();
}

float fake_angle;
std::array<float,24u> fake_pose;

void __fastcall c_client::shutdown(c_base_client* client, uint32_t)
{

}

void c_client::level_init_pre_entity(c_base_client* client, uint32_t, const char* map_name)
{
	_level_init_pre_entity(client, map_name);
}


template<typename FuncType>
__forceinline static FuncType CallVFuckingFunc(void* ppClass, int index)
{
	int* pVTable = *(int**)ppClass;
	int dwAddress = pVTable[index];
	return (FuncType)(dwAddress);
}


c_user_cmd *c_input::GetUserCmd(int nSlot, int sequence_number)
{
	typedef c_user_cmd*(__thiscall *GetUserCmd_t)(void*, int, int);
	return CallVFuckingFunc<GetUserCmd_t>(this, 8)(this, nSlot, sequence_number);
}

class c_create_move_fakelag {
	public:
		bool _send_packet_;
};

class c_fakelag_cmds
{
public:
	c_create_move_fakelag* sequence_tick{};
};


bool IsAbleToShift(int tickbase_shift)
{
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(c_cs_player::get_local_player()->get_current_weapon_handle()));

	if (!weapon)
		return false;

	float curtime = ticks_to_time(c_cs_player::get_local_player()->get_tick_base() - tickbase_shift);

	if (curtime < c_cs_player::get_local_player()->get_next_attack())
		return false;

	if (curtime < weapon->get_next_primary_attack())
		return false;

	return true;
}

bool shifted = false;
bool is_tick_shot = false;

bool aye = false;
void dobuletap(c_user_cmd* cmd, bool send)
{
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(c_cs_player::get_local_player()->get_current_weapon_handle()));
	if (!weapon)
		return;
	
	const auto info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!info)
		return;

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

	if (GetKeyState(config->Ragebot._auto.dt_key) && cfg.exploits == 1)
	{ 
		//auto weapon = g_csgo.m_local->get_active_weapon();
		if (!c_cs_player::get_local_player()->is_alive())
			return;

		//if (!(cmd->buttons & cmd->buttons & c_user_cmd::attack))
			//return;

		if (!(cmd->buttons & c_user_cmd::attack))
			return;

	//	unit::bsendpacket = true;

		//if (antiaim->shot_cmd != cmd->command_number)
		//	return;

		const int value_to_shift = 8;

		if (IsAbleToShift(value_to_shift))
		{
			static bool shot_last_tick = false;

			if (!shot_last_tick)
			{
				//cmd->buttons |= c_user_cmd::attack;
				if (exploits.can_shift_tickbase)
				{
					//send = true;
					unit::bsendpacket = true;
					exploits.tickbaseshift = value_to_shift;
					helper::g_pViolanes->g_nShotCmd = cmd->command_number;
					helper::g_pViolanes->g_nPlayerTickbase = c_cs_player::get_local_player()->get_tick_base();
					helper::g_pViolanes->g_nLastShift = value_to_shift;
					shifted = true;
					shot_last_tick = true;

					FEATURES::MISC::InGameLogger::Log log;
					log.color_line.PushBack("[EXPLOIT] ", c_color(255, 0, 0, 255));
					log.color_line.PushBack("DOUBLE TAP FIRED!", c_color(65, 67, 0, 255));
					log.time = global_vars_base->curtime;
					FEATURES::MISC::in_game_logger.AddLog(log);
				}
			}
			else
			{
				//send = true;
				cmd->buttons &= ~c_user_cmd::attack;
				shot_last_tick = false;
			}
		}	
	}
	else if (cfg.exploits == 2) // hide shot
	{
		if (!c_cs_player::get_local_player()->is_alive())
			return;

		if (antiaim->shot_cmd != cmd->command_number) // Is shooting this tick ?? Is aim angle applied??
			return;

	//	unit::bsendpacket = true;
		const int value_to_shift = 7; // we can hide shot even with 7 ticks... Also we can fakelag 7 ticks with that. (sv_maxproccessticks = 16, but most servers proccess ticks 14)

		if (IsAbleToShift(value_to_shift))
		{
			static bool shot_last_tick = false;

			if (!shot_last_tick)
			{
				if (exploits.can_shift_tickbase)
				{
					//send = true;
					exploits.tickbaseshift = value_to_shift;
					helper::g_pViolanes->g_nShotCmd = cmd->command_number;
					helper::g_pViolanes->g_nPlayerTickbase = c_cs_player::get_local_player()->get_tick_base();
					helper::g_pViolanes->g_nLastShift = value_to_shift;
					shifted = true;
					shot_last_tick = true;

					FEATURES::MISC::InGameLogger::Log log;
					log.color_line.PushBack("[EXPLOIT] ", c_color(255, 0, 0, 255));
					log.color_line.PushBack("HIDESHOT FIRED!", c_color(65, 67, 0, 255));
					log.time = global_vars_base->curtime;
					FEATURES::MISC::in_game_logger.AddLog(log);
				}
			}
			else
			{
				shot_last_tick = false;
			}
		}
	}
}

bool test = true;
void tickbase_reloadtime(c_user_cmd* cmd)
{
	//if (GetKeyState(config->Ragebot._auto.dt_key))
	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(c_cs_player::get_local_player()->get_current_weapon_handle()));
	if (!weapon)
		return;

	const auto info = weapon_system->get_weapon_data(weapon->get_item_definition());
	if (!info)
		return;

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

	if (cfg.exploits == 1)
	{
		const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(c_cs_player::get_local_player()->get_current_weapon_handle()));
		//auto weapon = g_csgo.m_local->get_active_weapon();
		if (!c_cs_player::get_local_player()->is_alive())
			return;

		//static int last_shift = 0;
		//static bool recharging = false;

		if (!shifted && !exploits.recharge && test == true)
		{
			exploits.can_shift_tickbase = true;
			cmd->tick_count = INT_MAX;
			test = false;
			unit::bsendpacket = true;
		}

		if (exploits.recharge)
			shifted = false;

		if (shifted && !exploits.recharge) {
			exploits.last_shift = cmd->tick_count;
			exploits.recharge = true;
			shifted = false;
			exploits.can_shift_tickbase = false;
		}

		if (exploits.recharge && abs(cmd->tick_count - exploits.last_shift) > time_to_ticks(0.2f)) {
			cmd->tick_count = INT_MAX;
			cmd->buttons &= ~c_user_cmd::attack;
			exploits.can_shift_tickbase = true;
			exploits.recharge = false;
		}
		//else
			//if ( !IsAbleToShift(14) )
			//	cmd->tick_count = INT_MAX;
	}
	else if (cfg.exploits == 2)
	{

		const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(c_cs_player::get_local_player()->get_current_weapon_handle()));
		//auto weapon = g_csgo.m_local->get_active_weapon();
		if (!c_cs_player::get_local_player()->is_alive())
			return;

		//static int last_shift = 0;
		//static bool recharging = false;

		if (!shifted && !exploits.recharge && test == true)
		{
			exploits.can_shift_tickbase = true;
			cmd->tick_count = INT_MAX;
			test = false;
		}

		if (exploits.recharge)
			shifted = false;

		if (shifted && !exploits.recharge) {
			exploits.last_shift = cmd->tick_count;
			exploits.recharge = true;
			shifted = false;
			exploits.can_shift_tickbase = false;
		}

		if (exploits.recharge && abs(cmd->tick_count - exploits.last_shift) > time_to_ticks(0.2f)) {
			cmd->tick_count = INT_MAX;
			//cmd->buttons &= ~c_user_cmd::attack;
			exploits.can_shift_tickbase = true;
			exploits.recharge = false;
		}
	}
}

bool is_revolver_shooting = false;
//c_fakelag_cmds* fakelag_cmd = new c_fakelag_cmds();
c_user_cmd* anal_cmd;

void __stdcall c_client::create_move(int sequence_number, float input_sample_frametime, bool active, bool& sendpacket)
{
	static const auto get_check_sum = reinterpret_cast<uint32_t(__thiscall*)(c_user_cmd*)>(sig("client.dll", "53 8B D9 83 C8"));
	static int ticks_allowed = 0;

	if (!get_check_sum) {
		cvar()->console_color_printf(false, c_color(255, 100, 100), "FAILED TO FIND `get_check_sum`\n");
	}

	const auto local = c_cs_player::get_local_player();
	//const auto local = reinterpret_cast<c_cs_player*>(client_entity_list()->get_client_entity(engine_client()->get_local_player()));
	if (!local)
		return _create_move(base_client(), sequence_number, input_sample_frametime, active, sendpacket);

	_create_move(base_client(), sequence_number, input_sample_frametime, active, sendpacket);

	auto cmd = input->GetUserCmd(0, sequence_number);
	auto verified_cmd = &input->verified_commands[sequence_number % 150];
	if (!cmd || !cmd->command_number || cmd == 0) return _create_move(base_client(), sequence_number, input_sample_frametime, active, sendpacket);

	c_cl_camera_height_restriction_debug::in_cm = true;

	uintptr_t* frame_pointer;
	__asm mov frame_pointer, ebp;
	//unit::bsendpacket = reinterpret_cast<bool*>(*frame_pointer - 0x1C);
	unit::bsendpacket = true;
	sendpacket = true;

	if (!cmd->command_number || !local || !local->is_alive())
	{
		c_cl_camera_height_restriction_debug::in_cm = false;
		return;
	}

	anal_cmd = cmd;

	static auto last_netchannel_ptr = net_channel;
	static auto did_once = false;

	if (net_channel && !did_once)
	{
		// first hook.
		c_net_channel_::hook();
		did_once = true;
	}

	if (last_netchannel_ptr != net_channel)
	{
		// rehook upon pointer changes.
		c_net_channel_::apply_to_net_chan(net_channel);
		last_netchannel_ptr = net_channel;
	}

	const auto weapon = reinterpret_cast<c_base_combat_weapon*>(client_entity_list()->get_client_entity_from_handle(local->get_current_weapon_handle()));

	c_base_combat_weapon::weapon_data* wpn_info = nullptr;

	if (!weapon || !(wpn_info = weapon_system->get_weapon_data(weapon->get_item_definition())))
	{
		c_cl_camera_height_restriction_debug::in_cm = false;
		return;
	}

	helper::loading(cmd);
	animation_system->in_jump = cmd->buttons & c_user_cmd::jump;

	//if (menu->open)
		//cmd->buttons &= ~c_user_cmd::attack;

	cmd->buttons |= c_user_cmd::bull_rush;
	cmd->buttons &= ~c_user_cmd::speed;

	helper::g_pViolanes->g_bOverrideModifier = true;

	if (helper::g_pViolanes->g_flVelocityModifier < 1.f)
		*(bool*)((uintptr_t)prediction() + 0x24) = true;

	prediction()->update(client_state->delta_tick, client_state->delta_tick > 0, client_state->last_command_ack, /// predict prestart
		client_state->last_command + client_state->choked_commands);

	c_movement::run(local, cmd);

	prediction_system->start(local, cmd); {
		auto target_angle = cmd->viewangles;
		//local->can_shoot(cmd, global_vars_base->curtime);
		antiaim->fakelag(local, cmd, unit::bsendpacket);

		c_miscellaneous::misc_cm(cmd);

		//c_esp::draw_thirdperson_indicators();
		auto ticks_to_cock = (int)(0.25f / (std::round(global_vars_base->interval_per_tick * 1000000.f) / 1000000.f));
		static int revolver_cocked_ticks = 0;

		if (config->Ragebot.enable)
		{
			c_ragebot::aim(local, cmd, unit::bsendpacket, weapon);
		}

		if (antiaim->get_last_shot() || local->is_shooting(cmd, global_vars_base->curtime))
		{
			antiaim->set_last_shot(false);
			c_aimhelper::fix_movement(cmd, target_angle);
			antiaim->shot_cmd = cmd->command_number;
		}

		antiaim->run(local, cmd, unit::bsendpacket, is_revolver_shooting);

		helper::g_pViolanes->g_bOverrideModifier = false;

		dobuletap(cmd, unit::bsendpacket);
		tickbase_reloadtime(cmd);
	}
	prediction_system->end(local, cmd);

	if (!unit::bsendpacket)
		ticks_allowed++;
	else
		ticks_allowed -= client_state->choked_commands;

	ticks_allowed = std::clamp(ticks_allowed, 0, 16);

	// verify new command.
	math::ensure_bounds(cmd->viewangles, *reinterpret_cast<c_vector3d*>(&cmd->forwardmove));

	//LUASYS->ExecuteCreateMove(cmd);

	c_miscellaneous::set_buttons_for_direction(cmd);

	verified_cmd->cmd = *cmd;
	verified_cmd->crc = get_check_sum(&verified_cmd->cmd);

	c_client_state_::cmds.push_back(cmd->command_number);

	c_cl_camera_height_restriction_debug::in_cm = false;
	animation_system->local_animation.eye_angles = cmd->viewangles;

	*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(_AddressOfReturnAddress()) + 0x14) = sendpacket = unit::bsendpacket;
}

bool fresh_tick()
{
	static int old_tick_count;

	if (old_tick_count != global_vars_base->tickcount)
	{
		old_tick_count = global_vars_base->tickcount;
		return true;
	}

	return false;
}

struct infos
{
	std::array<float, 24> m_poses;
	c_base_animating::animation_layers m_overlays;
};

void update_state(c_csgo_player_anim_state* state, c_vector3d ang) {
	using fn = void(__vectorcall*)(void*, void*, float, float, float, void*);
	static auto ret = reinterpret_cast<fn>(sig("client.dll", "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24"));

	if (!ret)
		return;

	ret(state, NULL, NULL, ang.y, ang.x, NULL);
}

float m_server_abs_rotation = 0.f;

static bool one_dog = true;
void c_client::frame_stage_notify(c_base_client* client, uint32_t, const clientframestage stage)
{
	//c_esp::fsn_nightmode();

	static c_base_animating::animation_layers server_layers{};

	if (!engine_client()->is_ingame() || !engine_client()->is_connected())
		return _frame_stage_notify(client, stage);

	const auto local = c_cs_player::get_local_player();
	if (!local || local == nullptr) return _frame_stage_notify(client, stage);

	static auto cycle = 0.f;
	static auto anim_time = 0.f;

	c_esp::fsn_nightmode();

	//const auto view_model = local ? reinterpret_cast<c_base_view_model*>(client_entity_list()->get_client_entity_from_handle(local->get_view_model())) : nullptr;
	
	helper::g_pViolanes->ResetData();
	/*
	if (engine_client()->is_ingame() && engine_client()->is_connected() && local && local->is_alive())
	{
		static int m_iLastCmdAck = 0;
		static float m_flNextCmdTime = 0.f;
		if (client_state && (m_iLastCmdAck != client_state->last_command_ack || m_flNextCmdTime != client_state->m_flNextCmdTime))
		{
			if (helper::g_pViolanes->g_flVelocityModifier != local->get_velocity_modifier())
			{
				*(bool*)((uintptr_t)prediction() + 0x24) = true;
				helper::g_pViolanes->g_flVelocityModifier = local->get_velocity_modifier();
			}
			m_iLastCmdAck = client_state->last_command_ack;
			m_flNextCmdTime = client_state->m_flNextCmdTime;
		}

	}
	*/
	if (stage == clientframestage::frame_net_update_postdataupdate_start && local && local->is_alive())	{
		if (config->Skinchanger.Enabled)
			SkinChanger2();
	}

	if (stage == frame_render_start) {
		c_resolver::on_render_start();

		if (one_dog)
		{

#define UNLEN 256
			char buffer[UNLEN + 1];
			DWORD size = UNLEN + 1;
			size = sizeof(buffer);
			GetUserName((TCHAR*)buffer, &size);
			char title[UNLEN];
			char ch1[25] = "";
			char* ch = strcat(ch1, buffer);

			std::string info = "=====================================";

			unit::g_ChatElement->ChatPrintf(1, 0, info.c_str());

			std::string type = "[Sequency]: ";
			std::string text2 = type + "Welcome back, " + ch;

			unit::g_ChatElement->ChatPrintf(2, 0, text2.c_str());
			unit::g_ChatElement->ChatPrintf(1, 0, info.c_str());

			one_dog = false;
		}

		for (int i = 1; i < engine_client()->get_max_clients(); i++)
		{
			auto ent = reinterpret_cast<c_cs_player*>(client_entity_list()->get_client_entity(i));

			if (!ent || ent->is_local_player())
				continue;

			if (ent && ent->is_enemy() && ent->get_health() > 0)
			{
				if (ent->is_dormant())
					continue;

				*(int*)((uintptr_t)ent + 0xA30) = global_vars_base->framecount; //we'll skip occlusion checks now
				*(int*)((uintptr_t)ent + 0xA28) = 0;//clear occlusion flags

				if (resolver.missed_due_to_bad_resolve[i] >= 4)
					resolver.missed_due_to_bad_resolve[i] = 0;
			}
		}

		if (local && local->is_alive() && !game_rules->is_freeze_period()) {
			const auto state = local->get_anim_state();
			if (state)
			{
				static auto last_tick = global_vars_base->tickcount;

				if (global_vars_base->tickcount != last_tick)
				{
					last_tick = global_vars_base->tickcount;

					if (server_layers.empty())
						server_layers = *local->get_animation_layers();

					const auto backup_layers = server_layers;

					animation_system->enable_bones = true;
					local->update_clientside_anim();
					animation_system->enable_bones = false;

					if (!client_state->choked_commands)
					{
						animation_system->local_animation.abs_ang.y = state->goal_feet_yaw;
						animation_system->local_animation.poses = local->get_pose_parameter();
					}

					*local->get_animation_layers() = backup_layers;

					local->get_anim_state()->feet_yaw_rate = 0.f;
					local->get_anim_state()->unknown_fraction = 0.f;
				}

				local->set_abs_angles(c_qangle(0, animation_system->local_animation.abs_ang.y, 0));
				local->get_pose_parameter() = animation_system->local_animation.poses;
				//local->InvalidateBoneCache();
			}
		}
	}
	
	_frame_stage_notify(client, stage);

	if (stage == frame_render_start)
	{
		if (local && local->is_alive()) {
			antiaim->increment_visual_progress();
			c_miscellaneous::set_viewmodel_parameters();
			c_miscellaneous::engine_radar();
			c_miscellaneous::view_model();
		}
		c_miscellaneous::remove_flash();
		c_miscellaneous::remove_smoke();
	}

	if (stage == frame_net_update_postdataupdate_start) {

		//	c_resolver::AntiFreestanding();
	
		c_miscellaneous::modify_player_model();
		/*if (view_model && !input->camera_in_third_person)
		{
			view_model->get_anim_time() = anim_time;
			view_model->get_cycle() = cycle;
		}*/

		if (local && local->is_alive())
			server_layers = *local->get_animation_layers();
	}

	//if (view_model) {
		//cycle = view_model->get_cycle();
		//anim_time = view_model->get_anim_time();
	//}

	if (stage == frame_net_update_end)
		animation_system->post_player_update();

	/*if (stage == frame_render_end) {
		c_resolver::AntiFreestanding();
	}*/
}

__declspec(naked) void create_move(int sequence_number, float input_sample_frametime, bool active)
{
	__asm
	{
		push ebx
		push esp
		push[esp + 0x14]
		push[esp + 0x14]
		push[esp + 0x14]
		call c_client::create_move
		pop ebx
		ret 0x0c
	}
}