#include "../../SDK/SDK.h"

#include "../Features/CFG.h"
#include "../Features/Aimbot/GlobalState.h"

MAKE_HOOK(CPrediction_RunCommand, Memory::GetVFunc(I::Prediction, 17), void, __fastcall,
	CPrediction* ecx, C_BasePlayer* player, CUserCmd* pCmd, IMoveHelper* moveHelper)
{
	if (Shifting::bRecharging)
	{
		if (const auto pLocal = H::Entities->GetLocal())
		{
			if (player == pLocal)
				return;
		}
	}

	CALL_ORIGINAL(ecx, player, pCmd, moveHelper);

	if (const auto pLocal = H::Entities->GetLocal())
	{
		//credits: KGB
		if (CFG::Misc_Accuracy_Improvements && !pLocal->InCond(TF_COND_HALLOWEEN_KART) && !Shifting::bRecharging)
		{
			if (!pCmd->hasbeenpredicted && player == pLocal)
			{
				if (const auto pAnimState = pLocal->GetAnimState())
				{
					const float flOldFrameTime = I::GlobalVars->frametime;
					I::GlobalVars->frametime = I::Prediction->m_bEnginePaused ? 0.0f : TICK_INTERVAL;

					// CRITICAL FIX: Handle silent aim angles properly for animations
					float flAnimYaw = pCmd->viewangles.y;
					float flAnimPitch = pCmd->viewangles.x;

					// If using silent aim (PSilentAngles), use the real angles for animation
					// This prevents double animations and mismatched visual state
					if (g_GlobalState.bPSilentAngles)
					{
						// Use stored real angles for animation when silent aim is active
						// This ensures animations match what the server sees, not the client aim angles
						flAnimYaw = g_GlobalState.vUserCmdAngles.y;
						flAnimPitch = g_GlobalState.vUserCmdAngles.x;
					}

					pAnimState->Update(G::bStartedFakeTaunt ? G::flFakeTauntStartYaw : flAnimYaw, flAnimPitch);
					pLocal->FrameAdvance(I::GlobalVars->frametime);
					I::GlobalVars->frametime = flOldFrameTime;
				}
			}
		}
	}
}
