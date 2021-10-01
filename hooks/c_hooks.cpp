#include "c_hooks.h"
#include "c_wnd_proc.h"
#include "idirect3ddevice9.h"
#include "c_client_mode_shared.h"
#include "c_surface_.h"
#include "c_events.h"
#include "c_model_render_.h"
#include "c_client.h"
#include "c_game_event_manager_.h"
#include "c_engine_client.h"
#include "c_base_animating_.h"
#include "c_cl_csm_shadows.h"
#include "c_net_channel_.h"
#include "c_cs_player_.h"
#include "dbghelp.h"
#include <thread>
#include "c_cl_camera_height_restriction_debug.h"
#include "c_view_render_.h"
#include "c_prediction_.h"
#include "c_client_state_.h"
#include "c_render_view_.h"
#include "c_engine_sound_.h"
#include "c_sv_cheats.h"
#include "c_panel_.h"
#include "engine_bsp.h"
#include "../hacks/c_esp.h"
#include "../hacks/skinchanger/kit_parser.h"
#include "../main.h"
#include "../sdk/c_base_tonecontrol.h"
#include "../baim.h"

using namespace std::chrono_literals;

c_events g_Event;

namespace fonts 
{
	DWORD ESP;
	DWORD health_font;
	DWORD weaponnamefont;
	DWORD weapon_font;
	DWORD flagfont;
	DWORD arialfont;
}
template<class T>
static T* FindHudElement_n(const char* name)
{
	static auto pThis = *reinterpret_cast<DWORD**> (sig("client.dll", "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08") + 1);

	static auto find_hud_element = reinterpret_cast<DWORD(__thiscall*)(void*, const char*)>(sig("client.dll", "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));
	return (T*)find_hud_element(pThis, name);
}

void c_hooks::run()
{

	/* eventlogs*/
		g_Event.Init();
	c_client_mode_shared::hook();
	c_wnd_proc::hook();
	//c_net_channel_::hook();
	c_cs_player_::hook();
	c_surface_::hook();
//	c_events::hook();
	c_render_view_::hook();
	c_model_render_::hook();
	c_client::hook();
	c_vgui_panel::hook();
	c_game_event_manager_::hook();
	//c_base_animating_::hook();
	c_engine_client_::hook();
	c_cl_csm_shadows::hook();
	c_cl_camera_height_restriction_debug::hook();
	c_view_render_::hook();
	c_prediction_::hook();
	c_client_state_::hook();
	c_engine_sound_::hook();
	c_sv_cheats::hook();
	c_panel_::hook();
	//engine_bsp::hook();
	env_tonemap_controllor::hook();
	idirect3ddevice9::instance();
	

	NetvarHook();

	unit::g_ChatElement = FindHudElement_n<unit::CHudChat>("CHudChat"); // skeet
}