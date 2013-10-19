// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "Ls3Thumb_i.h"
#include "dllmain.h"
#include "xdlldata.h"

CLs3ThumbModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
#ifdef _MERGE_PROXYSTUB
	if (!PrxDllMain(hInstance, dwReason, lpReserved))
		return FALSE;
#endif
	hInstance;

	// Register/unregister class for the hidden window on dynamic
	// load/unload
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		WNDCLASSEX wc;
		wc.hInstance = hInstance;
		wc.lpszClassName = L"Ls3ThumbShlExtHiddenWindow";
		wc.lpfnWndProc = DefWindowProc;
		wc.style = 0;
		wc.cbSize = sizeof (WNDCLASSEX);
		wc.hIcon = NULL;
		wc.hIconSm = NULL;
		wc.hCursor = NULL;
		wc.lpszMenuName = NULL;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = NULL;

		ATOM wndClassAtom = RegisterClassEx(&wc);
		if (wndClassAtom == 0) {
			DWORD lastError = GetLastError();
			return false;
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		if (UnregisterClass(L"Ls3ThumbShlExtHiddenWindow", hInstance) == false)
		{
			DWORD lastError = GetLastError();
		}
	}

	return _AtlModule.DllMain(dwReason, lpReserved);
}
