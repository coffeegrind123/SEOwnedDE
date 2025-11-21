#pragma once

#include "../../../SDK/SDK.h"
#include "../../../../include/imgui/imgui.h"
#include "../../../../include/imgui/backends/imgui_impl_win32.h"
#include "../../../../include/imgui/backends/imgui_impl_dx9.h"
#include <string>

struct IDirect3DDevice9;

class CMenu
{
private:
    bool m_bOpen = false;
    bool m_bInitialized = false;
    bool m_bImGuiContextCreated = false;

    // ImGui state
    ImVec2* m_vWindowSize;
    ImVec2* m_vWindowPos;

public:
    // DirectX9 device reference
    IDirect3DDevice9* m_pDevice;

    inline bool IsOpen() { return m_bOpen; }
    inline bool IsMenuWindowHovered() { return m_bInitialized && ImGui::IsWindowHovered(); }
    inline bool IsInitialized() { return m_bInitialized; }

    bool m_bWantTextInput = false;
    bool m_bInKeybind = false;

private:
    // Helper functions for ImGui controls
    bool InputKey(const char* szLabel, int& nKeyOut);
    void KeybindPopup(const char* szLabel, int& nKeyOut);
    std::string GetKeyName(int nKey);

    // Tab rendering functions
    void RenderAimTab();
    void RenderVisualsTab();
    void RenderMiscTab();
    void RenderPlayersTab();
    void RenderConfigsTab();

    // Sub-tab rendering functions
    void RenderAimbotTab();
    void RenderTriggerbotTab();
    void RenderESPTab();
    void RenderRadarTab();
    void RenderMaterialsTab();
    void RenderOutlinesTab();
    void RenderOtherTab();
    void RenderOther2Tab();
    void RenderColorsTab();

public:
    void Run();
    void Initialize(IDirect3DDevice9* pDevice = nullptr);
    void Shutdown();
    void RenderImguiFrame();
    CMenu();
    ~CMenu();
};

MAKE_SINGLETON_SCOPED(CMenu, Menu, F);