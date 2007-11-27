#include <windows.h>
#pragma argsused
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason,
                            LPVOID lpvReserved)
	{
	switch (fdwReason)
		{
		case DLL_PROCESS_ATTACH : break;
		case DLL_PROCESS_DETACH : break;
		}
	return TRUE;
	}
