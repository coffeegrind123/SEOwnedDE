#include "FakeAngle.h"
#include "../../../../SDK/Helpers/Entities/Entities.h"
#include "../../../../SDK/TF2/tf_shareddefs.h"
#include "../../../../SDK/TF2/const.h"

void CFakeAngle::Run(C_TFPlayer* pLocal)
{
	if (!ShouldRun())
	{
		bBonesSetup = false;
		return;
	}

	// Get animation state using SEOwnedDE's method
	auto pAnimState = pLocal->GetAnimState();
	if (!pAnimState)
		return;

	// Save current animation state
	float flOldFrameTime = I::GlobalVars->frametime;
	int nOldSequence = pLocal->m_nSequence();
	float flOldCycle = pLocal->m_flCycle();
	auto pOldPoseParams = pLocal->m_flPoseParameter();

	// Save animation state memory (simplified approach for SEOwnedDE)
	char pOldAnimState[sizeof(CMultiPlayerAnimState)];
	memcpy(pOldAnimState, pAnimState, sizeof(CMultiPlayerAnimState));

	// Set fake frame time for bone setup
	I::GlobalVars->frametime = 0.0f;

	// Clamp fake angles to valid ranges
	Vec2 vAngle = {
		std::clamp(vFakeAngles.x, -89.0f, 89.0f),
		vFakeAngles.y
	};

	// Handle taunt animations with fake angles
	if (pLocal->InCond(TF_COND_TAUNTING) && pLocal->m_bAllowMoveDuringTaunt())
	{
		pLocal->m_flTauntYaw() = vAngle.y;
	}

	// Update animation state with fake angles (Amalgam approach)
	// Note: Amalgam sets both m_flCurrentFeetYaw and m_flEyeYaw
	// For SEOwnedDE, we'll use the Update method directly
	pAnimState->Update(vAngle.y, vAngle.x);

	// Invalidate bone cache and setup bones for fake angles
	pLocal->InvalidateBoneCache();
	bBonesSetup = pLocal->SetupBones(aBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, I::GlobalVars->curtime);

	// Restore original animation state
	I::GlobalVars->frametime = flOldFrameTime;
	pLocal->m_nSequence() = nOldSequence;
	pLocal->m_flCycle() = flOldCycle;
	pLocal->m_flPoseParameter() = pOldPoseParams;
	memcpy(pAnimState, pOldAnimState, sizeof(CMultiPlayerAnimState));
}

bool CFakeAngle::ShouldRun()
{
	// Get local player
	auto pLocal = I::ClientEntityList->GetClientEntity(I::EngineClient->GetLocalPlayer());
	if (!pLocal)
		return false;

	auto pLocalPlayer = pLocal->As<C_TFPlayer>();
	if (!pLocalPlayer || pLocalPlayer->m_lifeState() != LIFE_ALIVE || pLocalPlayer->InCond(TF_COND_HALLOWEEN_GHOST_MODE))
		return false;

	// Check if anti-aim or fakelag is active (simplified version)
	// In a full implementation, this would check actual AntiAim and Ticks systems
	if (!bAntiAimOn && (!bFakelagOn || iShiftedTicks == iMaxShift))
	{
		return false;
	}

	// Additional checks could be added here for target validation, etc.
	return true;
}

void CFakeAngle::UpdateFakeAngles()
{
	// This would be updated by the AntiAim system
	// For now, using a simple implementation
	static int iCounter = 0;
	iCounter++;

	// Simple spinning fake angles for demonstration
	vFakeAngles.y = fmodf(iCounter * 2.0f, 360.0f);
	vFakeAngles.x = sinf(iCounter * 0.1f) * 30.0f;

	// In a real implementation, these would come from:
	// - Anti-aim system (F::AntiAim.vFakeAngles)
	// - Fakelag system (F::Ticks values)
}