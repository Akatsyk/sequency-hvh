#include "baim.h"
#include <Windows.h>
#include <Psapi.h>

#define INRANGE2(x,a,b)  (x >= a && x <= b) 
#define getBits2( x )    (INRANGE2((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE2(x,'0','9') ? x - '0' : 0))
#define getByte2( x )    (getBits2(x[0]) << 4 | getBits2(x[1]))


namespace unit
{
	bool bsendpacket;





	bool force_safepoint;




	uint64_t FindSignature22(const char* szModule, const char* szSignature)
	{
		MODULEINFO modInfo;
		GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(szModule), &modInfo, sizeof(MODULEINFO));
		DWORD startAddress = (DWORD)modInfo.lpBaseOfDll;
		DWORD endAddress = startAddress + modInfo.SizeOfImage;
		const char* pat = szSignature;
		DWORD firstMatch = 0;
		for (DWORD pCur = startAddress; pCur < endAddress; pCur++) {
			if (!*pat) return firstMatch;
			if (*(PBYTE)pat == '\?' || *(BYTE*)pCur == getByte2(pat)) {
				if (!firstMatch) firstMatch = pCur;
				if (!pat[2]) return firstMatch;
				if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?') pat += 3;
				else pat += 2;    //one ?
			}
			else {
				pat = szSignature;
				firstMatch = 0;
			}
		}
		return NULL;
	}


	

	CHudChat* g_ChatElement;


}