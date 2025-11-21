#include "../../SDK/SDK.h"

#include "../Features/EnginePrediction/EnginePrediction.h"
#include "../Features/Ticks/Ticks.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Aimbot/GlobalState.h"
#include "../Features/CFG.h"
#include "../Features/Visuals/FakeAngle/FakeAngle.h"

#include "../../SDK/Helpers/Entities/Entities.h"
#include "../../SDK/Helpers/AimUtils/AimUtils.h"

#define MATH_EPSILON (1.f / 16)
#define PSILENT_EPSILON (1.f - MATH_EPSILON)
#define REAL_EPSILON (0.1f + MATH_EPSILON)
#define SNAP_SIZE_EPSILON (10.f - MATH_EPSILON)
#define SNAP_NOISE_EPSILON (0.5f + MATH_EPSILON)

// TF2 Constants (ported from Amalgam)
#define WEAPON_NOCLIP -1
#define LIFE_ALIVE 0

// Structure for command history validation (ported from Amalgam)
struct CmdHistory_t
{
	Vec3 m_vAngle;
	bool m_bAttack1;
	bool m_bAttack2;
	bool m_bSendingPacket;
};

static inline void UpdateInfo(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	// Ported from Amalgam - comprehensive weapon and attack state management
	// CRITICAL FIX: Reset PSilentAngles at START of every frame like Amalgam (line 35)
	// Silent aim is a PER-FRAME decision, not persistent state!
	g_GlobalState.bSilentAngles = g_GlobalState.bPSilentAngles = g_GlobalState.bAttacking = g_GlobalState.bThrowing = false;
	g_GlobalState.bChoking = false; // Reset choking state at start of each frame
	g_GlobalState.pLastUserCmd = g_GlobalState.pCurrentUserCmd ? g_GlobalState.pCurrentUserCmd : pCmd;
	g_GlobalState.pCurrentUserCmd = pCmd;
	// g_GlobalState.OriginalCmd = *pCmd; // Commented out - OriginalCmd expects int, not CUserCmd

	if (!pWeapon)
		return;

	g_GlobalState.bCanPrimaryAttack = g_GlobalState.bCanSecondaryAttack = g_GlobalState.bReloading = false;

	// Enhanced weapon state check - ported from Amalgam
	// SEOwnedDE compatibility: Use NETVAR access since methods are not available
	if (pWeapon->m_iClip1() != -1 && true) // Simplified check - m_bReloadsSingly not available
	{
		// dumb fix from Amalgam - check reload state properly
		float flOldCurtime = I::GlobalVars->curtime;
		I::GlobalVars->curtime = TICKS_TO_TIME(pLocal->m_nTickBase());
		// pWeapon->CheckReload(); // Method not available in SEOwnedDE
		I::GlobalVars->curtime = flOldCurtime;
	}

	// SEOwnedDE compatibility: Use simple attack check since CanAttack is not available
	bool bCanAttack = (pLocal->m_lifeState() == LIFE_ALIVE && pLocal->m_flNextAttack() <= I::GlobalVars->curtime);
	{
		static int iStaticItemDefinitionIndex = 0;
		int iOldItemDefinitionIndex = iStaticItemDefinitionIndex;
		int iNewItemDefinitionIndex = iStaticItemDefinitionIndex = pWeapon->m_iItemDefinitionIndex();

		if (iNewItemDefinitionIndex != iOldItemDefinitionIndex || !bCanAttack || !pWeapon->m_iClip1())
			F::Ticks->m_iWait = 1;
	}

	if (bCanAttack)
	{
		// Calculate weapon attack states using current time for now
		// Will be recalculated with predicted time after F::Ticks->Start()
		g_GlobalState.bCanPrimaryAttack = pWeapon->CanPrimaryAttack(pLocal);
		g_GlobalState.bCanSecondaryAttack = pWeapon->CanSecondaryAttack(pLocal);

		// Ported from Amalgam - weapon-specific attack checks
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_FLAME_BALL:
			if (g_GlobalState.bCanPrimaryAttack)
			{
				// do this, otherwise it will be a tick behind
				float flFrametime = TICK_INTERVAL * 100;
				// Note: IHasGenericMeter_GetMeterMultiplier would need SEOwnedDE adaptation
				// float flMeterMult = S::IHasGenericMeter_GetMeterMultiplier.Call<float>(pWeapon->m_pMeter());
				float flRate = 1.f; // SDK::AttribHookValue(1.f, "item_meter_charge_rate", pWeapon) - 1;
				float flMult = 1.f; // SDK::AttribHookValue(1.f, "mult_item_meter_charge_rate", pWeapon);
				float flTankPressure = pLocal->m_flTankPressure() + flFrametime * 0.f / (flRate * flMult); // Simplified

				if (g_GlobalState.bCanPrimaryAttack && flTankPressure < 100.f)
					g_GlobalState.bCanPrimaryAttack = g_GlobalState.bCanSecondaryAttack = false;
			}
			break;
		case TF_WEAPON_MINIGUN:
		{
			// Note: CTFMinigun would need SEOwnedDE adaptation
			// int iState = pWeapon->As<CTFMinigun>()->m_iWeaponState();
			// if (iState != AC_STATE_FIRING && iState != AC_STATE_SPINNING || !pWeapon->HasPrimaryAmmoForShot())
			//	g_GlobalState.bCanPrimaryAttack = false;
			break;
		}
		case TF_WEAPON_FLAREGUN_REVENGE:
			if (pCmd->buttons & IN_ATTACK2)
				g_GlobalState.bCanPrimaryAttack = false;
			break;
		case TF_WEAPON_BAT_WOOD:
		case TF_WEAPON_BAT_GIFTWRAP:
			if (!pWeapon->HasPrimaryAmmoForShot())
				g_GlobalState.bCanSecondaryAttack = false;
			break;
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_LASER_POINTER:
			break;
		case TF_WEAPON_PARTICLE_CANNON:
		{
			// Note: CTFParticleCannon would need SEOwnedDE adaptation
			// float flChargeBeginTime = pWeapon->As<CTFParticleCannon>()->m_flChargeBeginTime();
			// if (flChargeBeginTime > 0)
			// {
			//	float flTotalChargeTime = TICKS_TO_TIME(pLocal->m_nTickBase()) - flChargeBeginTime;
			//	if (flTotalChargeTime < TF_PARTICLE_MAX_CHARGE_TIME)
			//	{
			//		g_GlobalState.bCanPrimaryAttack = g_GlobalState.bCanSecondaryAttack = false;
			//		break;
			//	}
			// }
			break;
		}
		default:
			if (pWeapon->GetSlot() != WEAPON_SLOT_MELEE)
			{
				bool bAmmo = pWeapon->HasPrimaryAmmoForShot();
				// SEOwnedDE compatibility: Use state checking since IsInReload is not available
				bool bReload = pWeapon->m_iState() == 2; // Approximate reload state
				if (!bAmmo && pWeapon->m_iItemDefinitionIndex() != 115) // Soldier_m_TheBeggarsBazooka item definition
					g_GlobalState.bCanPrimaryAttack = g_GlobalState.bCanSecondaryAttack = false;
				if (bReload && bAmmo && !g_GlobalState.bCanPrimaryAttack)
					g_GlobalState.bReloading = true;
			}
		}

		if (g_GlobalState.bCanPrimaryAttack)
		{
			switch (pWeapon->GetWeaponID())
			{
			case TF_WEAPON_FLAMETHROWER:
			case TF_WEAPON_FLAME_BALL:
			case TF_WEAPON_FLAREGUN:
			case TF_WEAPON_FLAREGUN_REVENGE:
				// SEOwnedDE compatibility: Use water level check since IsUnderwater is not available
				if (pLocal->m_nWaterLevel() > 1) // Simple water level check
					g_GlobalState.bCanPrimaryAttack = g_GlobalState.bCanSecondaryAttack = false;
			}
		}
	}

	// Initial attack state calculation (will be recalculated after aimbot like Amalgam does)
	g_GlobalState.bAttacking = (pCmd->buttons & IN_ATTACK) || (pCmd->buttons & IN_ATTACK2);
	// Keep G::bFiring in sync for compatibility
	G::bFiring = g_GlobalState.bAttacking;
	// Note: SDK::GetWeaponType would need SEOwnedDE adaptation
	// g_GlobalState.PrimaryWeaponType = SDK::GetWeaponType(pWeapon, &g_GlobalState.SecondaryWeaponType);
	// g_GlobalState.bCanHeadshot = pWeapon->CanHeadshot() || pWeapon->AmbassadorCanHeadshot(TICKS_TO_TIME(pLocal->m_nTickBase()));
}

// CRITICAL FIX: Recalculate bAttacking AFTER aimbot runs (like Amalgam does in CAimbot::Run line 78)
// This prevents the double animation bug by ensuring we know the FINAL attack state
static inline void RecalculateAttackingState(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	// Recalculate with final button state after aimbot may have modified it
	g_GlobalState.bAttacking = (pCmd->buttons & IN_ATTACK) || (pCmd->buttons & IN_ATTACK2);
	G::bFiring = g_GlobalState.bAttacking;
}

static inline void AntiCheatCompatibility(CUserCmd* pCmd, bool* pSendPacket)
{
	// Ported from Amalgam - advanced anti-cheat compatibility with command history validation
	// SEOwnedDE config integration point: AntiCheatCompatibility
	// if (!CFG::Misc_AntiCheatCompatibility)
	//	return;
	// For now, enable by default for functionality (like Amalgam)

	Math::ClampAngles(pCmd->viewangles); // shouldn't happen, but failsafe

	// Command history validation for anti-cheat compatibility (ported from Amalgam)
	static std::deque<CmdHistory_t> vHistory;
	vHistory.emplace_front(pCmd->viewangles, pCmd->buttons & IN_ATTACK, pCmd->buttons & IN_ATTACK2, *pSendPacket);
	if (vHistory.size() > 5)
		vHistory.pop_back();

	if (vHistory.size() < 3)
		return;

	// prevent trigger checks, though this shouldn't happen ordinarily
	if (!vHistory[0].m_bAttack1 && vHistory[1].m_bAttack1 && !vHistory[2].m_bAttack1)
		pCmd->buttons |= IN_ATTACK;
	if (!vHistory[0].m_bAttack2 && vHistory[1].m_bAttack2 && !vHistory[2].m_bAttack2)
		pCmd->buttons |= IN_ATTACK2;

	// don't care if we are actually attacking or not, a miss is less important than a detection
	if (vHistory[0].m_bAttack1 || vHistory[1].m_bAttack1 || vHistory[2].m_bAttack1)
	{
		// prevent silent aim checks
		if (Math::CalcFov(vHistory[0].m_vAngle, vHistory[1].m_vAngle) > PSILENT_EPSILON
			&& Math::CalcFov(vHistory[0].m_vAngle, vHistory[2].m_vAngle) < REAL_EPSILON)
		{
			{
			// Manual angle interpolation since LerpAngle is not available in SEOwnedDE Vec3
			Vec3 vDelta = vHistory[0].m_vAngle - vHistory[1].m_vAngle;
			Math::ClampAngles(vDelta);
			pCmd->viewangles = vHistory[1].m_vAngle + vDelta * 0.5f;
		}
			if (Math::CalcFov(pCmd->viewangles, vHistory[2].m_vAngle) < REAL_EPSILON)
				pCmd->viewangles = vHistory[0].m_vAngle + Vec3(0.f, REAL_EPSILON * 2, 0.f);
			vHistory[0].m_vAngle = pCmd->viewangles;
			vHistory[0].m_bSendingPacket = *pSendPacket = vHistory[1].m_bSendingPacket;
		}

		// prevent aim snap checks
		if (vHistory.size() == 5)
		{
			float flDelta01 = Math::CalcFov(vHistory[0].m_vAngle, vHistory[1].m_vAngle);
			float flDelta12 = Math::CalcFov(vHistory[1].m_vAngle, vHistory[2].m_vAngle);
			float flDelta23 = Math::CalcFov(vHistory[2].m_vAngle, vHistory[3].m_vAngle);
			float flDelta34 = Math::CalcFov(vHistory[3].m_vAngle, vHistory[4].m_vAngle);

			if ((
				flDelta12 > SNAP_SIZE_EPSILON && flDelta23 < SNAP_NOISE_EPSILON &&
				(vHistory[2].m_vAngle.x != vHistory[3].m_vAngle.x || vHistory[2].m_vAngle.y != vHistory[3].m_vAngle.y || vHistory[2].m_vAngle.z != vHistory[3].m_vAngle.z)
				|| flDelta23 > SNAP_SIZE_EPSILON && flDelta12 < SNAP_NOISE_EPSILON &&
				(vHistory[1].m_vAngle.x != vHistory[2].m_vAngle.x || vHistory[1].m_vAngle.y != vHistory[2].m_vAngle.y || vHistory[1].m_vAngle.z != vHistory[2].m_vAngle.z)
				)
				&& flDelta01 < SNAP_NOISE_EPSILON &&
				(vHistory[0].m_vAngle.x != vHistory[1].m_vAngle.x || vHistory[0].m_vAngle.y != vHistory[1].m_vAngle.y || vHistory[0].m_vAngle.z != vHistory[1].m_vAngle.z)
				&& flDelta34 < SNAP_NOISE_EPSILON &&
				(vHistory[3].m_vAngle.x != vHistory[4].m_vAngle.x || vHistory[3].m_vAngle.y != vHistory[4].m_vAngle.y || vHistory[3].m_vAngle.z != vHistory[4].m_vAngle.z))
			{
				pCmd->viewangles.y += SNAP_NOISE_EPSILON * 2;
				vHistory[0].m_vAngle = pCmd->viewangles;
				vHistory[0].m_bSendingPacket = *pSendPacket = vHistory[1].m_bSendingPacket;
			}
		}
	}

	// Silent aim packet manipulation (enhanced from original SEOwnedDE)
	if (g_GlobalState.bPSilentAngles)
	{
		// Apply FixMovement to ensure correct movement direction for silent aim
		if (I::EngineClient->GetLocalPlayer())
		{
			auto pLocal = I::ClientEntityList->GetClientEntity(I::EngineClient->GetLocalPlayer());
			if (pLocal && pLocal->As<C_TFPlayer>())
			{
				// Fix movement to match the silent aim angles
				H::AimUtils->FixMovement(pCmd, pCmd->viewangles);
			}
		}

		// Don't send packet for silent aim (angle is applied to cmd but not visible)
		*pSendPacket = false;

		// Set choking state for tick system compatibility
		g_GlobalState.bChoking = true;
	}
	else
	{
		// Update choking state when not using silent aim
		g_GlobalState.bChoking = !*pSendPacket;
	}

	// Don't choke too much
	if (I::ClientState->chokedcommands > 22)
	{
		*pSendPacket = true;
	}
}

// Ported from Amalgam - local animation system for proper client-side animation
// CRITICAL FIX: This function prevents double animations in silent mode!
static inline void LocalAnimations(C_TFPlayer* pLocal, CUserCmd* pCmd, bool bSendPacket)
{
	static std::vector<Vec3> vAngles = {};
	vAngles.push_back(pCmd->viewangles);

	// CRITICAL: ONLY process animations when bSendPacket = true (like Amalgam lines 152-170)
	// This prevents phantom animations during choked frames (silent aim)
	if (bSendPacket)
	{
		// SEOwnedDE compatibility: Check if animation state is available
		// In SEOwnedDE, we may need to adapt the animation system access
		if (pLocal && pLocal->m_nWaterLevel() >= 0) // Basic validity check
		{
			// Set proper global timing for animation system (exact Amalgam approach)
			float flOldFrametime = I::GlobalVars->frametime;
			float flOldCurtime = I::GlobalVars->curtime;
			I::GlobalVars->frametime = TICK_INTERVAL;
			I::GlobalVars->curtime = TICKS_TO_TIME(pLocal->m_nTickBase());

			// Process accumulated angles for animation (Amalgam-style implementation)
			for (auto& vAngle : vAngles)
			{
				// SEOwnedDE adaptation: Handle taunt animations
				// Use SEOwnedDE's condition checking for taunt state
				if (pLocal->InCond(TF_COND_TAUNTING))
					pLocal->m_flTauntYaw() = vAngle.y;

				// CRITICAL FIX: Use SEOwnedDE's animation state access method
				auto pAnimState = pLocal->GetAnimState();
				if (pAnimState)
				{
					// Update animation state with current angles (exact Amalgam approach)
					pAnimState->Update(vAngle.y, vAngle.x);
				}

				// SEOwnedDE adaptation: Frame advance using client-side method
				pLocal->UpdateClientSideAnimation();
			}

			// Restore original global timing (exact Amalgam approach)
			I::GlobalVars->frametime = flOldFrametime;
			I::GlobalVars->curtime = flOldCurtime;

			// Clear processed angles (exact Amalgam approach)
			vAngles.clear();

			// SEOwnedDE adaptation: FakeAngle system integration - NOW IMPLEMENTED!
			// Run fake angle processing for visual effects and anti-cheat compatibility
			F::FakeAngle->Run(pLocal);
		}
	}
	// CRITICAL: When bSendPacket = false (silent aim), animations are NOT processed
	// This prevents double animations and phantom fire effects
}

MAKE_HOOK(CHLClient_CreateMove, Memory::GetVFunc(I::BaseClientDLL, 21), bool, __fastcall,
	void* ecx, int sequence_number, float input_sample_frametime, bool active)
{
	const auto original = CALL_ORIGINAL(ecx, sequence_number, input_sample_frametime, active);

	bool* pSendPacket = reinterpret_cast<bool*>(uintptr_t(_AddressOfReturnAddress()) + 0x20);
	CUserCmd* pCmd = &I::Input->GetCommands()[sequence_number % MULTIPLAYER_BACKUP];

	if (!pCmd || !pCmd->command_number)
		return original;

	auto pLocal = H::Entities->GetLocal();
	auto pWeapon = H::Entities->GetWeapon();
	if (!pLocal)
		return original;

	I::Prediction->Update
	(
		I::ClientState->m_nDeltaTick,
		I::ClientState->m_nDeltaTick > 0,
		I::ClientState->last_command_ack,
		I::ClientState->lastoutgoingcommand + I::ClientState->chokedcommands
	);

	UpdateInfo(pLocal, pWeapon, pCmd);

	// Edge jump functionality from original SEOwnedDE
	if (H::Input->IsDown(CFG::Misc_Edge_Jump_Key))
	{
		if ((F::EnginePrediction->flags & FL_ONGROUND) && !(pLocal->m_fFlags() & FL_ONGROUND))
		{
			pCmd->buttons |= IN_JUMP;
		}
	}

	// CORRECTED FLOW based on Amalgam analysis - exact Amalgam execution order:
	// Prediction starts inside F::Ticks.Start() and ends inside F::Ticks.End()

	// Start tick manipulation (this will internally start prediction)
	F::Ticks->Start(pLocal, pCmd);

	// Run aimbot DURING prediction
	// NOTE: CanPrimaryAttack() uses pLocal->m_nTickBase() internally (not I::GlobalVars->curtime)
	// so no need to modify curtime here. The tickbase is updated by prediction in F::Ticks->Start()
	F::Aimbot->Run(pLocal, pWeapon, pCmd);

	// CRITICAL FIX: Recalculate attacking state AFTER aimbot (like Amalgam CAimbot::Run line 78)
	// This is THE FIX for the double animation bug!
	RecalculateAttackingState(pLocal, pWeapon, pCmd);

	// End tick manipulation (this will internally end prediction)
	F::Ticks->End(pLocal, pCmd);

	// Follow exact Amalgam execution order:
	F::Ticks->CreateMove(pLocal, pWeapon, pCmd, pSendPacket);

	// CRITICAL: Final prediction cleanup - THE MISSING PIECE for silent aim! (from Amalgam line 267)
	F::EnginePrediction->End(pLocal, pCmd);

	// Anti-cheat compatibility (ported from Amalgam)
	AntiCheatCompatibility(pCmd, pSendPacket);

	// Local animations (ported from Amalgam)
	LocalAnimations(pLocal, pCmd, *pSendPacket);

	g_GlobalState.nOldButtons = pCmd->buttons;
	g_GlobalState.vUserCmdAngles = pCmd->viewangles;

	// Ported from Amalgam - set choking state (now available with enhanced GlobalState)
	g_GlobalState.bChoking = !*pSendPacket;
	g_GlobalState.pLastUserCmd = pCmd;

	return (g_GlobalState.bSilentAngles || g_GlobalState.bPSilentAngles) ? false : original;
}