#include "../../SDK/SDK.h"

#include "../Features/Materials/Materials.h"
#include "../Features/Outlines/Outlines.h"
#include "../Features/WorldModulation/WorldModulation.h"
#include "../Features/Paint/Paint.h"
#include "../Features/SeedPred/SeedPred.h"
#include "../../SDK/Helpers/Draw/DrawImGui.h"

MAKE_HOOK(IBaseClientDLL_LevelShutdown, Memory::GetVFunc(I::BaseClientDLL, 7), void, __fastcall,
	void* ecx)
{
	// Clear entity caches BEFORE original call to prevent accessing freed entities
	// Pass true to clear both temp AND main buffers (prevents dangling pointers during transition)
	H::Entities->ClearCache(true);

	// Reset W2S matrix to prevent using stale data during level transition
	if (H::DrawImGui) {
		H::DrawImGui->ResetW2SMatrix();
	}

	CALL_ORIGINAL(ecx);

	H::Entities->ClearModelIndexes();

	F::Materials->CleanUp();
	F::Outlines->CleanUp();
	F::Paint->CleanUp();
	F::WorldModulation->LevelShutdown();

	F::SeedPred->Reset();

	G::mapVelFixRecords.clear();

	Shifting::Reset();
}
