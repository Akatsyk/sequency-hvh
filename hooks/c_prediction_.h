#pragma once

#include "../sdk/c_prediction.h"



namespace helper
{
	class dt
	{
	public:
		float g_flNextSecondaryAttack = 0;
		int g_iTickcount = 0;
		float g_iTickrate = 0;
		float g_nLastShift = 0;
		int g_nPlayerTickbase = 0;
		float g_flVelocityModifier = 0;
		int g_nShotCmd = 0;
		bool g_bOverrideModifier = 0;
		int m_iMaxProcessTicks = 0;

		int m_nShift = 0;
		int m_nChokeLimit = 0;

		bool m_bProcessPacket = false;
		bool m_bNextShift = false;
		bool m_bSwap = false;
		bool m_bEnabled = false;

		void ResetData()
		{
			g_nPlayerTickbase = 0;
			m_iMaxProcessTicks = 0;
			m_bSwap = false;
		}

	};

	extern void loading(c_user_cmd* m_pCmd);
	extern dt* g_pViolanes;
}


class c_prediction_
{
	typedef bool(__thiscall* in_prediction_t)(c_prediction*);
	//typedef void(__thiscall* run_command_t)(c_prediction*, c_cs_player*, c_user_cmd*, c_move_helper*);
	typedef void(__thiscall* rucmd_t)(void*, c_cs_player*, c_user_cmd*, void*);
	
	
public:
	

	static void hook();

private:
	inline static in_prediction_t _in_prediction;
	//inline static run_command_t _run_command;
	inline static rucmd_t _run_command;

	static bool __fastcall in_prediction(c_prediction* prediction, uint32_t);
	static void __fastcall RunCommand(void* ecx, void* edx, c_cs_player* player, c_user_cmd* ucmd, void* m_pMoveHelper);
	//static void __fastcall RunCommand(void* ecx, void* edx, c_cs_player* player, c_user_cmd* ucmd, c_move_helper* moveHelper);
	
};
