#pragma once
#include "stdint.h"
#include <stdio.h>
#include <stdarg.h>
#include "vfunc.h"
namespace unit
{


	class CHudChat
	{
	public:
		enum ChatFilters
		{
			CHAT_FILTER_NONE = 0,
			CHAT_FILTER_JOINLEAVE = 0x000001,
			CHAT_FILTER_NAMECHANGE = 0x000002,
			CHAT_FILTER_PUBLICCHAT = 0x000004,
			CHAT_FILTER_SERVERMSG = 0x000008,
			CHAT_FILTER_TEAMCHANGE = 0x000010,
			//=============================================================================
			// HPE_BEGIN:
			// [tj]Added a filter for achievement announce
			//=============================================================================

			CHAT_FILTER_ACHIEVEMENT = 0x000020,

			//=============================================================================
			// HPE_END
			//=============================================================================
		};

		void ChatPrintf(int iPlayerIndex, int iFilter, const char* fmt, ...)
		{
			char msg[1024];

			va_list args;
			va_start(args, fmt);
			vsprintf(msg, fmt, args);
			call_vfunc<void(__cdecl*)(void*, int, int, const char*, ...)>(this, 27)(this, iPlayerIndex, iFilter, fmt);
			va_end(args);
		}
	};




	extern bool bsendpacket;

	extern bool force_safepoint;
	extern CHudChat* g_ChatElement;
	extern uint64_t FindSignature22(const char* szModule, const char* szSignature);
}