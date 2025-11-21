#include "WINAPI_WndProc.h"

#include "../Features/Menu/Menu.h"

LRESULT __stdcall Hooks::WINAPI_WndProc::Func(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// WndProc is now handled by imguiHook - this hook is disabled
	return CallWindowProc(Original, hWnd, uMsg, wParam, lParam);
}

void Hooks::WINAPI_WndProc::Init()
{
	// Disabled - WndProc now handled by imguiHook in basicHook system
	// hwWindow = SDKUtils::GetTeamFortressWindow();
	// Original = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Func)));
}

void Hooks::WINAPI_WndProc::Release()
{
	// Disabled - WndProc now handled by imguiHook
	// if (Original) {
	// 	SetWindowLongPtr(hwWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Original));
	// }
}
