#include "../../SDK/SDK.h"

#include "../Features/Menu/Menu.h"
#include "../Features/ESP/ESP.h"
#include "../Features/Radar/Radar.h"
#include "../Features/MiscVisuals/MiscVisuals.h"
#include "../Features/SpectatorList/SpectatorList.h"
#include "../Features/SpyCamera/SpyCamera.h"
#include "../Features/SpyWarning/SpyWarning.h"
#include "../Features/TeamWellBeing/TeamWellBeing.h"
#include "../Features/SeedPred/SeedPred.h"

MAKE_HOOK(IEngineVGuiInternal_Paint, Memory::GetVFunc(I::EngineVGui, 14), void, __fastcall,
	void *ecx, int mode)
{
	CALL_ORIGINAL(ecx, mode);

	if (mode & PAINT_UIPANELS)
	{
		// Matrix update now handled in Present hook to prevent double-update flicker
		// H::Draw->UpdateW2SMatrix();

		I::MatSystemSurface->StartDrawing();
		{
			// Handle menu input toggle (F3/INSERT key)
			try {
				F::Menu->Run();
			}
			catch (...) {
				// If input handling crashes, just skip menu processing
			}

			// Menu and ESP rendering are now handled in basicHook Present hook (stream-proof)
			// Other visual features still render here until converted to ImGui

			// Run remaining visual features
			// F::ESP->Run(); // Now handled by ImGui in Present hook
			// F::TeamWellBeing->Run(); // Now handled by ImGui in Present hook
			// F::Radar->Run(); // Now handled by ImGui in Present hook
			// F::SpectatorList->Run(); // Now handled by ImGui in Present hook
			// F::MiscVisuals->AimbotFOVCircle(); // Now handled by ImGui in Present hook
			// F::SpyWarning->Run(); // Now handled by ImGui in Present hook
			// F::SeedPred->Paint(); // Now handled by ImGui in Present hook
			F::SpyCamera->Run();
		}
		I::MatSystemSurface->FinishDrawing();
	}
}
