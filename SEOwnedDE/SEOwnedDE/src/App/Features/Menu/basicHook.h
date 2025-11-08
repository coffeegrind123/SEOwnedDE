#pragma once
#include "../../../../include/MinHook/MinHook.h"
#include <d3d9.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include "findpattern.h"
#include "debugMessage.h"
#include "imguiHook.h"

// Enforce 64-bit compilation
#ifndef _WIN64
#error "This project requires 64-bit compilation for TF2 Steam overlay compatibility"
#endif

namespace hooks {
    /**
     * @brief Initialize TF2 Steam overlay hooks (x64)
     * Locates and hooks Steam overlay functions for Team Fortress 2
     */
    void Initialize();

    /**
     * @brief Enhanced MinHook wrapper with 64-bit validation and error handling
     * @tparam T Function pointer type
     * @param pTarget Target function address to hook
     * @param pDetour Detour function to redirect to
     * @param ppOriginal Pointer to store original function
     */
    template<typename T>
    inline void Hook(void* pTarget, void* pDetour, T** ppOriginal)
    {
        if (!pTarget || !pDetour) {
            std::cout << "TF2 Hook: Invalid parameters (target: " << pTarget << ", detour: " << pDetour << ")" << std::endl;
            return;
        }

        // Validate target address is in executable memory
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(pTarget, &mbi, sizeof(mbi))) {
            std::cout << "TF2 Hook: Failed to query target memory" << std::endl;
            return;
        }

        if (!(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            std::cout << "TF2 Hook: Target is not executable memory (protect: 0x" << std::hex << mbi.Protect << ")" << std::endl;
            return;
        }

        // Create hook with enhanced error reporting
        MH_STATUS creationStatus = MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
        if (creationStatus != MH_OK) {
            std::cout << "TF2 Hook: MH_CreateHook() failed with status: " << MH_StatusToString(creationStatus) << std::endl;
            return;
        }

        // Enable hook with enhanced error reporting
        MH_STATUS enableStatus = MH_EnableHook(pTarget);
        if (enableStatus != MH_OK) {
            std::cout << "TF2 Hook: MH_EnableHook() failed with status: " << MH_StatusToString(enableStatus) << std::endl;
            MH_RemoveHook(pTarget); // Cleanup on failure
            return;
        }

        std::cout << "TF2 Hook: Successfully hooked function at 0x" << std::hex << reinterpret_cast<uintptr_t>(pTarget) << std::endl;
    }

    /**
     * @brief Safely uninitialize all TF2 Steam overlay hooks
     * Performs proper cleanup of MinHook and ImGui resources
     */
    void Uninitialize();
}

// Forward declarations for TF2-specific hook functions
extern "C" {
    /**
     * @brief Extract Steam overlay function address from pattern using LEA instruction analysis
     * @param pattern 64-bit pattern to search for
     * @param patternName Name for logging purposes
     * @return Function address or 0 if not found
     */
    uintptr_t ExtractSteamFunction(const char* pattern, const char* patternName);
}

// TF2 Steam overlay configuration
namespace TF2Config {
    constexpr const char* STEAM_OVERLAY_DLL = "gameoverlayrenderer64.dll";
    constexpr const char* PRESENT_PATTERN = "48 8B ? 88 00 00 00 E8";
    constexpr const char* RESET_PATTERN = "48 8B ? 80 00 00 00 E8";
    constexpr DWORD OVERLAY_TOGGLE_KEY = VK_F1;
}

// Global state management for TF2 overlay
extern bool g_overlayVisible;
extern bool g_initialized;