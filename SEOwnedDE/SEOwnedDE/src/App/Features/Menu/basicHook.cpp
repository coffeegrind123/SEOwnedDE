#include "basicHook.h"
#include "Menu.h"
#include "../ESP/ESP.h"
#include "../Radar/Radar.h"
#include "../SpectatorList/SpectatorList.h"
#include "../MiscVisuals/MiscVisuals.h"
#include "../TeamWellBeing/TeamWellBeing.h"
#include "../SpyCamera/SpyCamera.h"
#include "../SpyWarning/SpyWarning.h"
#include "../SeedPred/SeedPred.h"
#include <atomic>

#define REGISTER_HOOK(pTarget, pDetour, ppOriginal) Hook(pTarget, pDetour, ppOriginal); \
originalFunctions.push_back(pTarget);

// 64-bit function signatures for TF2 Steam overlay
using tPresent = HRESULT(STDMETHODCALLTYPE*) (IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
using tReset = HRESULT (STDMETHODCALLTYPE*) (IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

// Original function pointers for 64-bit TF2
tPresent oPresent = nullptr;
tReset oReset = nullptr;

// TF2Config constants are now defined in basicHook.h

// Global state for overlay visibility
static bool g_overlayVisible = true;
static bool g_initialized = false;
static std::atomic<bool> g_unloading{false}; // Thread-safe unloading flag

/**
 * @brief Extract Steam overlay function address from pattern using LEA instruction analysis
 * @param pattern Pattern to search for in Steam overlay
 * @param patternName Name for logging
 * @return Steam function address, or 0 if not found
 */
uintptr_t ExtractSteamFunction(const char* pattern, const char* patternName) {
    uintptr_t patternAddr = FindPattern(TF2Config::STEAM_OVERLAY_DLL, pattern);
    if (!patternAddr) {
        LOGHEX("Pattern not found for", patternName);
        return 0;
    }

    LOGHEX("Found pattern for", patternName);
    LOGHEX("Pattern address", patternAddr);

    // Extract function address using LEA instruction analysis
    uintptr_t functionAddr = ExtractFunctionFromLEA(patternAddr, -7);
    if (!functionAddr) {
        // Try alternative offsets if standard -7 doesn't work
        for (int offset = -15; offset <= -3; offset++) {
            functionAddr = ExtractFunctionFromLEA(patternAddr, offset);
            if (functionAddr) {
                LOGHEX("Found function with offset", offset);
                break;
            }
        }
    }

    if (functionAddr && IsValidExecutableAddress(functionAddr)) {
        LOGHEX("Extracted Steam function", functionAddr);
        return functionAddr;
    }

    LOGHEX("Failed to extract valid function for", patternName);
    return 0;
}

/**
 * @brief TF2 Present hook - renders overlay interface
 */
HRESULT STDMETHODCALLTYPE hkPresent(IDirect3DDevice9* thisptr, const RECT* src, const RECT* dest, HWND wnd_override, const RGNDATA* dirty_region) {
    // Skip rendering if unloading is in progress to prevent crashes
    if (g_unloading.load(std::memory_order_acquire)) {
        return oPresent(thisptr, src, dest, wnd_override, dirty_region);
    }

    // Additional safety check - ensure the hook is still valid
    if (!oPresent) {
        return E_FAIL;
    }
    // Initialize ImGui on first call
    if (!g_initialized && thisptr) {
        HRESULT deviceState = thisptr->TestCooperativeLevel();
        if (deviceState == D3D_OK) {
            imguiHook::InitializeImgui(thisptr);

            // Initialize Menu with device reference
            if (F::Menu && !F::Menu->IsInitialized()) {
                F::Menu->Initialize(thisptr);
            }

            g_initialized = true;
            LOGHEX("ImGui initialized for TF2", reinterpret_cast<uintptr_t>(thisptr));
        } else {
            LOGHEX("D3D device not ready, state", deviceState);
            return oPresent(thisptr, src, dest, wnd_override, dirty_region);
        }
    }

    // Render Menu and ESP in Steam overlay (stream-proof)
    if (g_initialized) {
        // Check device state before rendering
        HRESULT deviceState = thisptr->TestCooperativeLevel();
        if (deviceState != D3D_OK) {
            // Device lost or not ready, skip rendering
            if (deviceState == D3DERR_DEVICELOST) {
                // Device was lost, try to reset on next frame
                static bool deviceLostReported = false;
                if (!deviceLostReported) {
                    LOGHEX("D3D device lost, will attempt recovery", deviceState);
                    deviceLostReported = true;
                }
            }
            return oPresent(thisptr, src, dest, wnd_override, dirty_region);
        } else {
            static bool deviceLostReported = false;
            if (deviceLostReported) {
                LOGHEX("D3D device recovered", deviceState);
                deviceLostReported = false;
            }
        }

        try {
            // Additional safety check - ensure global interfaces are still valid
            if (!I::EngineClient || !I::GlobalVars || !H::Entities) {
                return oPresent(thisptr, src, dest, wnd_override, dirty_region);
            }

            // Data race fix in Entities.cpp should prevent flicker
            // No need for duplicate call filtering - let all Present calls through

            // Setup ImGui frame (following GOESP pattern)
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            // Update W2S matrix and screen size (both have frame guards to prevent multiple updates per frame)
            H::DrawImGui->UpdateW2SMatrix();
            H::DrawImGui->UpdateScreenSize();

            // Render ESP in background draw list (stream-proof)
            if (F::ESP) {
                try {
                    F::ESP->RunImGui();
                } catch (...) {
                    LOGHEX("ESP rendering error during unload", 0);
                }
            }

            // Render Radar in background draw list (stream-proof)
            if (F::Radar) {
                try {
                    F::Radar->RunImGui();
                } catch (...) {
                    LOGHEX("Radar rendering error during unload", 0);
                }
            }

            // Render Team WellBeing in background draw list (stream-proof)
            if (F::TeamWellBeing) {
                try {
                    F::TeamWellBeing->RunImGui();
                } catch (...) {
                    LOGHEX("TeamWellBeing rendering error during unload", 0);
                }
            }

            // Render Spectator List in background draw list (stream-proof)
            if (F::SpectatorList) {
                try {
                    F::SpectatorList->RunImGui();
                } catch (...) {
                    LOGHEX("SpectatorList rendering error during unload", 0);
                }
            }

            // Render MiscVisuals features in background draw list (stream-proof)
            if (F::MiscVisuals) {
                try {
                    F::MiscVisuals->AimbotFOVCircleImGui();
                    F::MiscVisuals->ShiftBarImGui();
                } catch (...) {
                    LOGHEX("MiscVisuals rendering error during unload", 0);
                }
            }

            // Render SpyWarning in background draw list (stream-proof)
            if (F::SpyWarning) {
                try {
                    F::SpyWarning->RunImGui();
                } catch (...) {
                    LOGHEX("SpyWarning rendering error during unload", 0);
                }
            }

            // Render SeedPred in background draw list (stream-proof)
            if (F::SeedPred) {
                try {
                    F::SeedPred->PaintImGui();
                } catch (...) {
                    LOGHEX("SeedPred rendering error during unload", 0);
                }
            }

            // Render Menu (stream-proof)
            if (F::Menu && F::Menu->IsOpen()) {
                try {
                    F::Menu->RenderImguiFrame();
                } catch (...) {
                    LOGHEX("Menu rendering error during unload", 0);
                }
            }

            // Complete ImGui frame and render
            ImGui::Render();

            if (thisptr->BeginScene() == D3D_OK) {
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
                thisptr->EndScene();
            }
        }
        catch (...) {
            LOGHEX("Exception in overlay rendering", 0);
        }
    }

    return oPresent(thisptr, src, dest, wnd_override, dirty_region);
}

/**
 * @brief TF2 Reset hook - handles device reset
 */
HRESULT STDMETHODCALLTYPE hkReset(IDirect3DDevice9* thisptr, D3DPRESENT_PARAMETERS* params) {
    LOGHEX("TF2 Device Reset requested", reinterpret_cast<uintptr_t>(thisptr));
    
    if (g_initialized) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
    }
    
    HRESULT result = oReset(thisptr, params);
    
    if (SUCCEEDED(result) && g_initialized) {
        ImGui_ImplDX9_CreateDeviceObjects();
        LOGHEX("TF2 Device Reset successful", result);
    } else if (!SUCCEEDED(result)) {
        LOGHEX("TF2 Device Reset failed", result);
    }
    
    return result;
}

std::vector<void*> originalFunctions;

void hooks::Initialize()
{
    try {
        LOGHEX("Initializing TF2 Steam Overlay Hook (x64)", 0);
        
        // Verify we're targeting the correct Steam overlay module
        HMODULE overlayModule = GetModuleHandleA(TF2Config::STEAM_OVERLAY_DLL);
        if (!overlayModule) {
            throw std::exception("TF2 Steam overlay module not found! Ensure TF2 is running with Steam overlay enabled.");
        }
        
        LOGHEX("TF2 Steam overlay module found", reinterpret_cast<uintptr_t>(overlayModule));

        // MinHook already initialized by SEOwnedDE HookManager - skip initialization

        // Extract Steam overlay function addresses using modern pattern analysis
        uintptr_t presentFunction = ExtractSteamFunction(TF2Config::PRESENT_PATTERN, "Present");
        uintptr_t resetFunction = ExtractSteamFunction(TF2Config::RESET_PATTERN, "Reset");

        if (!presentFunction) {
            throw std::exception("Failed to locate TF2 Steam Present function!");
        }

        LOGHEX("TF2 Present function", presentFunction);
        
        // Hook Present function (required)
        REGISTER_HOOK(reinterpret_cast<void*>(presentFunction), &hkPresent, &oPresent);
        
        // Hook Reset function (optional, but recommended)
        if (resetFunction) {
            LOGHEX("TF2 Reset function", resetFunction);
            REGISTER_HOOK(reinterpret_cast<void*>(resetFunction), &hkReset, &oReset);
        } else {
            LOGHEX("TF2 Reset function not found (non-critical)", 0);
        }

        LOGHEX("TF2 Steam Overlay hooks installed successfully", originalFunctions.size());

    } catch (const std::exception &ex) {
        MessageBoxA(nullptr, ex.what(), "TF2 Steam Overlay Hook Error", MB_ICONERROR);
    }
}

void hooks::Uninitialize()
{
    LOGHEX("Uninitializing TF2 Steam Overlay Hook", originalFunctions.size());

    // Set unloading flag to prevent Present hook from running during cleanup
    g_unloading.store(true, std::memory_order_release);

    // Wait longer to ensure all in-progress Present calls complete
    // Present can be called rapidly, so we need more time
    Sleep(100);

    // First, disable all hooks to prevent new calls while cleaning up
    for (auto& org : originalFunctions) {
        MH_DisableHook(org);
    }

    // Additional wait after disabling hooks
    Sleep(50);

    // Now cleanup ImGui if it was initialized
    if (g_initialized) {
        try {
            // Set ImGui context to null first to prevent any new frame operations
            ImGui::SetCurrentContext(nullptr);

            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();

            // Only destroy context if we still have one
            if (ImGui::GetCurrentContext()) {
                ImGui::DestroyContext();
            }

            g_initialized = false;
        }
        catch (...) {
            // If ImGui cleanup fails, just log and continue
            LOGHEX("ImGui cleanup failed during unload", 0);
        }
    }

    // Now remove all hooks completely
    for (auto& org : originalFunctions) {
        MH_RemoveHook(org);
    }

    // Clear the function list
    originalFunctions.clear();

    // Reset Present hook pointer to prevent any late calls
    oPresent = nullptr;

    LOGHEX("TF2 Steam Overlay Hook cleanup complete", 0);
}

