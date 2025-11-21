#pragma once
#include <Windows.h>
#include <d3d9.h>
#include "../../../../include/imgui/imgui.h"
#include "../../../../include/imgui/backends/imgui_impl_win32.h"
#include "../../../../include/imgui/backends/imgui_impl_dx9.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace imguiHook {
    void InitializeImgui(IDirect3DDevice9* pDevice);
}