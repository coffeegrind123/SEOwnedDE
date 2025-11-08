#include "imguiHook.h"
#include "../../../SDK/SDK.h"
#include "Menu.h"
#include "../../../Utils/Utils.h"

WNDPROC oWndProc;

LRESULT STDMETHODCALLTYPE hkWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Handle ImGui input first
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    // If our menu is open, block mouse events from reaching the game
    if (F::Menu && F::Menu->IsOpen()) {
        switch (uMsg) {
            // Block all mouse movement events
            case WM_MOUSEMOVE:
            case WM_NCMOUSEMOVE:
            case WM_MOUSEHOVER:
            case WM_NCMOUSEHOVER:
                // Block mouse movement to prevent game menu highlighting
                return true;

            // Block mouse button events (though input system should handle these)
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
                return true;

            // Block cursor changes
            case WM_SETCURSOR:
                return true;

            // Block mouse wheel
            case WM_MOUSEWHEEL:
                return true;

            // Block mouse leave/enter events
            case WM_MOUSELEAVE:
            case WM_NCMOUSELEAVE:
                return true;
        }
    }

    // Control game input based on menu state (like GOESP)
    if (I::InputSystem && F::Menu) {
        I::InputSystem->EnableInput(!F::Menu->IsOpen());
    }

    // Let other events go through to the game
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void imguiHook::InitializeImgui(IDirect3DDevice9* pDevice) {
    if (ImGui::GetCurrentContext() != nullptr) {
        return;
    }

    HWND window = SDKUtils::GetTeamFortressWindow();
    if (!window || !pDevice) {
        return;
    }

    oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, LONG_PTR(hkWndProc)));
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(pDevice);
}