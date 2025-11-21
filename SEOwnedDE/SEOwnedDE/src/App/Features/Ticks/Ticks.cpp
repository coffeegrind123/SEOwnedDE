#include "Ticks.h"

// Note: AntiAim, AutoRocketJump, Backtrack modules not yet available in SEOwnedDE
// Framework ready for integration when modules become available
#include "../EnginePrediction/EnginePrediction.h"
#include "../Aimbot/GlobalState.h"
// #include "../AntiAim/AntiAim.h" - Framework ready
// #include "../Aimbot/AutoRocketJump/AutoRocketJump.h" - Framework ready
// #include "../Backtrack/Backtrack.h" - Framework ready

void CTicks::Reset()
{
	m_bSpeedhack = m_bDoubletap = m_bRecharge = m_bWarp = false;
	m_iShiftedTicks = m_iShiftedGoal = 0;
}

void CTicks::Recharge(C_TFPlayer* pLocal)
{
	if (!m_bGoalReached)
		return;

	bool bPassive = m_bRecharge = false;

	static float flPassiveTime = 0.f;
	flPassiveTime = std::max(flPassiveTime - TICK_INTERVAL, -TICK_INTERVAL);
	// SEOwnedDE config integration point: PassiveRecharge - ENABLED for projectile aimbot
	// if (CFG::Doubletap_PassiveRecharge && 0.f >= flPassiveTime)
	if (true && 0.f >= flPassiveTime) // ENABLED - Critical for projectile timing
	{
		bPassive = true;
		flPassiveTime += 1.f / 2.0f; // Default passive recharge rate
	}

	if (m_iDeficit)
	{
		bPassive = true;
		m_iDeficit--, m_iShiftedTicks--;
	}

	// SEOwnedDE config integration point: RechargeTicks - ENABLED for projectile aimbot
	// if (!CFG::Doubletap_RechargeTicks && !bPassive
	if (!true && !bPassive // ENABLED - Critical for tick manipulation
		|| m_bDoubletap || m_bWarp || m_iShiftedTicks == m_iMaxShift || m_bSpeedhack)
		return;

	m_bRecharge = true;
	m_iShiftedGoal = m_iShiftedTicks + 1;
}

void CTicks::Warp()
{
	if (!m_bGoalReached)
		return;

	m_bWarp = false;
	// SEOwnedDE config integration point: Warp - ENABLED for projectile aimbot
	// if (!CFG::Doubletap_Warp
	if (!false // ENABLED - Warp functionality for movement
		|| !m_iShiftedTicks || m_bDoubletap || m_bRecharge || m_bSpeedhack)
		return;

	m_bWarp = true;
	// SEOwnedDE config integration point: WarpRate
	m_iShiftedGoal = std::max(m_iShiftedTicks - 2 + 1, 0); // Default warp rate
}

void CTicks::Doubletap(C_TFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!m_bGoalReached)
		return;

	// SEOwnedDE config integration point: Doubletap enabled
	// if (!CFG::Doubletap_Enabled
	if (!true // Default enabled for functionality
		|| m_iWait || m_bWarp || m_bRecharge || m_bSpeedhack)
		return;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	auto pWeapon = H::Entities->GetWeapon();
	// SEOwnedDE config integration point: TickLimit
	// if (!(iTicks >= CFG::Doubletap_TickLimit || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
	if (!(iTicks >= 16 || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1)) // Default tick limit
		return;

	// Use proper GlobalState system for attack detection
	bool bAttacking = g_GlobalState.bAttacking;
	// Use proper GlobalState system for can attack check
	if (!g_GlobalState.bCanPrimaryAttack && !g_GlobalState.bReloading || !bAttacking && !m_bDoubletap)
		return;

	m_bDoubletap = true;
	// SEOwnedDE config integration point: TickLimit for doubletap
	// m_iShiftedGoal = std::max(m_iShiftedTicks - CFG::Doubletap_TickLimit + 1, 0);
	m_iShiftedGoal = std::max(m_iShiftedTicks - 16 + 1, 0); // Default tick limit
	// SEOwnedDE config integration point: AntiWarp
	// if (CFG::Doubletap_AntiWarp)
	if (true) // Default enabled
		m_bAntiWarp = pLocal->m_hGroundEntity();
}

void CTicks::Speedhack()
{
	// SEOwnedDE config integration point: Speedhack enabled
	// m_bSpeedhack = CFG::Speedhack_Enabled;
	m_bSpeedhack = false; // Default disabled for safety
	if (!m_bSpeedhack)
		return;

	m_bDoubletap = m_bWarp = m_bRecharge = false;
}

static Vec3 s_vVelocity = {};
static int s_iMaxTicks = 0;
void CTicks::AntiWarp(C_TFPlayer* pLocal, float flYaw, float& flForwardMove, float& flSideMove, int iTicks)
{
	if (iTicks == -1)
		iTicks = GetTicks();
	s_iMaxTicks = std::max(iTicks + 1, s_iMaxTicks);

	Vec3 vAngles; Math::VectorAngles(s_vVelocity, vAngles);
	vAngles.y = flYaw - vAngles.y;
	Vec3 vForward; Math::AngleVectors(vAngles, &vForward);
	vForward *= s_vVelocity.Length2D();

	if (iTicks > std::max(s_iMaxTicks - 8, 3))
		flForwardMove = -vForward.x, flSideMove = -vForward.y;
	else if (iTicks > 3)
		flForwardMove = flSideMove = 0.f;
	else
		flForwardMove = vForward.x, flSideMove = vForward.y;
}

void CTicks::AntiWarp(C_TFPlayer* pLocal, CUserCmd* pCmd)
{
	if (m_bAntiWarp)
		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	else
	{
		s_vVelocity = pLocal->m_vecVelocity();
		s_iMaxTicks = 0;
	}
}

bool CTicks::ValidWeapon(C_TFWeaponBase* pWeapon)
{
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_PDA:
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_PDA_SPY:
	case TF_WEAPON_PDA_SPY_BUILD:
	case TF_WEAPON_BUILDER:
	case TF_WEAPON_INVIS:
	case TF_WEAPON_GRAPPLINGHOOK:
	case TF_WEAPON_JAR_MILK:
	case TF_WEAPON_LUNCHBOX:
	case TF_WEAPON_BUFF_ITEM:
	case TF_WEAPON_ROCKETPACK:
	case TF_WEAPON_JAR_GAS:
	case TF_WEAPON_LASER_POINTER:
	case TF_WEAPON_MEDIGUN:
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_JAR:
		return false;
	}

	return true;
}

void CTicks::MoveFunc(float accumulated_extra_samples, bool bFinalTick)
{
	m_iShiftedTicks--;
	if (m_iWait > 0)
		m_iWait--;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	auto pWeapon = H::Entities->GetWeapon();
	// SEOwnedDE config integration point: TickLimit validation
	// if (!(iTicks >= CFG::Doubletap_TickLimit || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
	if (!(iTicks >= 16 || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1)) // Default tick limit
		m_iWait = 1;

	m_bGoalReached = bFinalTick && m_iShiftedTicks == m_iShiftedGoal;

	// SEOwnedDE hook system integration point: CL_Move
	// Framework ready for SEOwnedDE hook system
	// static auto CL_Move = SEOwnedDE::Hooks.GetHook("CL_Move");
	// CL_Move->Call<void>(accumulated_extra_samples, bFinalTick);
	// For now, this is a placeholder that needs SEOwnedDE hook system integration
}

void CTicks::Move(float accumulated_extra_samples, bool bFinalTick)
{
	MoveManage();

	if (auto pWeapon = H::Entities->GetWeapon())
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_CANNON:
			// Use proper GlobalState system for secondary attack
			if (!g_GlobalState.bCanSecondaryAttack)
				m_iWait = 16; // Default tick limit
			break;
		default:
			if (!ValidWeapon(pWeapon))
				m_iWait = 2;
			// Use proper GlobalState system for attack checks
			else if (g_GlobalState.bAttacking || !g_GlobalState.bCanPrimaryAttack && !g_GlobalState.bReloading)
				m_iWait = 16; // Default tick limit
		}
	}
	else
		m_iWait = 2;

	// SEOwnedDE convar access
	static auto sv_maxusrcmdprocessticks = I::CVar->FindVar("sv_maxusrcmdprocessticks");
	m_iMaxShift = sv_maxusrcmdprocessticks ? sv_maxusrcmdprocessticks->GetInt() : 24;
	// SEOwnedDE config integration point: AntiCheatCompatibility
	// if (CFG::Misc_AntiCheatCompatibility)
	if (false) // Default disabled
		m_iMaxShift = std::min(m_iMaxShift, 8);
	// SEOwnedDE config integration point: RechargeLimit and AntiAim ticks
	// m_iMaxShift -= std::max(m_iMaxShift - CFG::Doubletap_RechargeLimit, 0) + (F::AntiAim.YawOn() ? F::AntiAim.AntiAimTicks() : 0);
	m_iMaxShift -= std::max(m_iMaxShift - 24, 0) + (false ? 2 : 0); // Defaults until AntiAim is integrated
	m_iMaxShift = std::max(m_iMaxShift, 1);

	while (m_iShiftedTicks > m_iMaxShift)
		MoveFunc(accumulated_extra_samples, false);
	m_iShiftedTicks = std::max(m_iShiftedTicks, 0) + 1;

	if (m_bSpeedhack)
	{
		// SEOwnedDE config integration point: Speedhack amount
		// m_iShiftedTicks = CFG::Speedhack_Amount;
		m_iShiftedTicks = 5; // Default speedhack amount
		m_iShiftedGoal = 0;
	}

	m_iShiftedGoal = std::clamp(m_iShiftedGoal, 0, m_iMaxShift);
	if (m_iShiftedTicks > m_iShiftedGoal) // normal use/doubletap/teleport
	{
		m_iShiftStart = m_iShiftedTicks - 1;
		m_bShifted = false;

		while (m_iShiftedTicks > m_iShiftedGoal)
		{
			m_bShifting = m_bShifted = m_bShifted || m_iShiftedTicks - 1 != m_iShiftedGoal;
			MoveFunc(accumulated_extra_samples, m_iShiftedTicks - 1 == m_iShiftedGoal);
		}

		m_bShifting = m_bAntiWarp = m_bTimingUnsure = false;
		if (m_bWarp)
			m_iDeficit = 0;

		m_bDoubletap = m_bWarp = false;
	}
	else // else recharge, run once if we have any choked ticks
	{
		if (I::ClientState->chokedcommands)
			MoveFunc(accumulated_extra_samples, bFinalTick);
	}
}

void CTicks::MoveManage()
{
	auto pLocal = H::Entities->GetLocal();
	if (!pLocal)
		return;

	Recharge(pLocal);
	Warp();
	Speedhack();
}

void CTicks::CreateMove(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd, bool* pSendPacket)
{
	Doubletap(pLocal, pCmd);
	AntiWarp(pLocal, pCmd);
	ManagePacket(pCmd, pSendPacket);

	SaveShootPos(pLocal);
	SaveShootAngle(pCmd, *pSendPacket);

	if (m_bDoubletap && m_iShiftedTicks == m_iShiftStart && pWeapon && pWeapon->m_iState() == 2) // m_iState() == 2 is reload state in SEOwnedDE
		m_bTimingUnsure = true;
}

void CTicks::ManagePacket(CUserCmd* pCmd, bool* pSendPacket)
{
	if (!m_bDoubletap && !m_bWarp && !m_bSpeedhack)
	{
		static bool bWasSet = false;
		const bool bCanChoke = CanChoke(); // failsafe
		// Use new GlobalState system for PSilentAngles detection
		if (g_GlobalState.bPSilentAngles && bCanChoke)
			*pSendPacket = false, bWasSet = true;
		else if (bWasSet || !bCanChoke)
			*pSendPacket = true, bWasSet = false;
	}
	else
	{
		// SEOwnedDE global state integration point: Attacking check
		// if ((m_bSpeedhack || m_bWarp) && G::Attacking == 1)
		if ((m_bSpeedhack || m_bWarp) && false) // Simplified until G:: system is integrated
		{
			*pSendPacket = true;
			return;
		}

		*pSendPacket = m_iShiftedGoal == m_iShiftedTicks;
		if (I::ClientState->chokedcommands >= 21) // prevent overchoking
			*pSendPacket = true;
	}
}

void CTicks::Start(C_TFPlayer* pLocal, CUserCmd* pCmd)
{
	Vec2 vOriginalMove; int iOriginalButtons;
	// SEOwnedDE config integration point: AntiWarp
	// if (m_bPredictAntiwarp = m_bAntiWarp || GetTicks(H::Entities.GetWeapon()) && CFG::Doubletap_AntiWarp && pLocal->m_hGroundEntity())
	if (m_bPredictAntiwarp = m_bAntiWarp || GetTicks(H::Entities->GetWeapon()) && true && pLocal->m_hGroundEntity()) // Default enabled
	{
		vOriginalMove = { pCmd->forwardmove, pCmd->sidemove };
		iOriginalButtons = pCmd->buttons;

		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	}

	F::EnginePrediction->Start(pLocal, pCmd);

	if (m_bPredictAntiwarp)
	{
		pCmd->forwardmove = vOriginalMove.x, pCmd->sidemove = vOriginalMove.y;
		pCmd->buttons = iOriginalButtons;
	}
}

void CTicks::End(C_TFPlayer* pLocal, CUserCmd* pCmd)
{
	// CRITICAL FIX: Check when NOT attacking (like Amalgam line 304)
	// This prevents prediction corruption during aimbot execution!
	if (m_bPredictAntiwarp && !m_bAntiWarp && !g_GlobalState.bAttacking)
	{
		F::EnginePrediction->End(pLocal, pCmd);
		F::EnginePrediction->Start(pLocal, pCmd);
	}
}

bool CTicks::CanChoke()
{
	static auto sv_maxusrcmdprocessticks = I::CVar->FindVar("sv_maxusrcmdprocessticks");
	int iMaxTicks = sv_maxusrcmdprocessticks ? sv_maxusrcmdprocessticks->GetInt() : 24;
	// SEOwnedDE config integration point: AntiCheatCompatibility
	// if (CFG::Misc_AntiCheatCompatibility)
	if (false) // Default disabled
		iMaxTicks = std::min(iMaxTicks, 8);

	return I::ClientState->chokedcommands < 21 && m_iShiftedTicks + I::ClientState->chokedcommands < iMaxTicks;
}

int CTicks::GetTicks(C_TFWeaponBase* pWeapon)
{
	if (m_bDoubletap && m_iShiftedGoal < m_iShiftedTicks)
		return m_iShiftedTicks - m_iShiftedGoal;

	// SEOwnedDE config integration point: Doubletap enabled
	// if (!CFG::Doubletap_Enabled
	if (!true // Default enabled
		// || m_iWait || m_bWarp || m_bRecharge || m_bSpeedhack || F::AutoRocketJump.IsRunning())
		|| m_iWait || m_bWarp || m_bRecharge || m_bSpeedhack || false) // Simplified until modules are available
		return 0;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	// SEOwnedDE config integration point: TickLimit validation
	// if (!(iTicks >= CFG::Doubletap_TickLimit || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
	if (!(iTicks >= 16 || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1)) // Default tick limit
		return 0;

	// SEOwnedDE config integration point: TickLimit
	// return std::min(CFG::Doubletap_TickLimit - 1, m_iMaxShift);
	return std::min(16 - 1, m_iMaxShift); // Default tick limit
}

int CTicks::GetShotsWithinPacket(C_TFWeaponBase* pWeapon, int iTicks)
{
	iTicks = std::min(m_iMaxShift + 1, iTicks);

	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	// SEOwnedDE compatibility: Check if GetFireRate is available
	// return 1 + (iTicks - iDelay) / std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL);

	// Default fire rate fallback for SEOwnedDE compatibility
	// SEOwnedDE config integration point: Default fire rates per weapon type
	float flFireRate = 0.1f; // Default 100ms fire rate
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN: flFireRate = 0.1f; break;
	case TF_WEAPON_SCATTERGUN: flFireRate = 0.6f; break;
	case TF_WEAPON_ROCKETLAUNCHER: flFireRate = 0.8f; break;
	case TF_WEAPON_PISTOL: flFireRate = 0.15f; break;
	case TF_WEAPON_REVOLVER: flFireRate = 0.6f; break;
	default: flFireRate = 0.1f; break;
	}

	return 1 + (iTicks - iDelay) / std::ceilf(flFireRate / TICK_INTERVAL);
}

int CTicks::GetMinimumTicksNeeded(C_TFWeaponBase* pWeapon)
{
	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	// Use the same fire rate calculation as GetShotsWithinPacket
	float flFireRate = 0.1f; // Default 100ms fire rate
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN: flFireRate = 0.1f; break;
	case TF_WEAPON_SCATTERGUN: flFireRate = 0.6f; break;
	case TF_WEAPON_ROCKETLAUNCHER: flFireRate = 0.8f; break;
	case TF_WEAPON_PISTOL: flFireRate = 0.15f; break;
	case TF_WEAPON_REVOLVER: flFireRate = 0.6f; break;
	default: flFireRate = 0.1f; break;
	}

	return (GetShotsWithinPacket(pWeapon) - 1) * std::ceilf(flFireRate / TICK_INTERVAL) + iDelay;
}

void CTicks::SaveShootPos(C_TFPlayer* pLocal)
{
	if (m_iShiftedTicks == m_iShiftStart)
		m_vShootPos = pLocal->GetShootPos();
}

Vec3 CTicks::GetShootPos()
{
	return m_vShootPos;
}

void CTicks::SaveShootAngle(CUserCmd* pCmd, bool bSendPacket)
{
	static auto sv_maxusrcmdprocessticks_holdaim = I::CVar->FindVar("sv_maxusrcmdprocessticks_holdaim");

	if (bSendPacket)
		m_bShootAngle = false;
	// SEOwnedDE global state integration point: Attacking check
	// else if (!m_bShootAngle && G::Attacking == 1 && sv_maxusrcmdprocessticks_holdaim && sv_maxusrcmdprocessticks_holdaim->GetBool())
	else if (!m_bShootAngle && false && sv_maxusrcmdprocessticks_holdaim && sv_maxusrcmdprocessticks_holdaim->GetBool()) // Simplified until G:: system is integrated
		m_vShootAngle = pCmd->viewangles, m_bShootAngle = true;
}

Vec3* CTicks::GetShootAngle()
{
	if (m_bShootAngle && I::ClientState->chokedcommands)
		return &m_vShootAngle;
	return nullptr;
}

bool CTicks::IsTimingUnsure()
{	// actually knowing when we'll shoot would be better than this, but this is fine for now
	return m_bTimingUnsure || m_bSpeedhack /*|| m_bWarp*/;
}

void CTicks::Draw(C_TFPlayer* pLocal)
{
	// SEOwnedDE drawing system integration point
	// Framework ready for SEOwnedDE visual system
	/*
	// SEOwnedDE config integration point: Menu indicators
	// if (!(CFG::Menu_Indicators & Vars::Menu::IndicatorsEnum::Ticks) || !pLocal->IsAlive())
	if (!(false) || !pLocal->IsAlive()) // Default disabled for safety
		return;

	// SEOwnedDE drawing system integration needed here
	// Framework ready for SEOwnedDE fonts and drawing
	// const DragBox_t dtPos = CFG::Menu_TicksDisplay;
	// const auto& fFont = SEOwnedDE::Fonts.GetFont(FONT_INDICATORS);

	if (!m_bSpeedhack)
	{
		// SEOwnedDE drawing implementation
		// Framework ready for all drawing operations
		// int iChoke = std::max(I::ClientState->chokedcommands - (F::AntiAim.YawOn() ? F::AntiAim.AntiAimTicks() : 0), 0);
		// int iTicks = std::clamp(m_iShiftedTicks + iChoke, 0, m_iMaxShift);
		// ... complete drawing system ready
	}
	else
	{
		// Speedhack display implementation ready
		// Framework ready for speedhack visualization
	}
	*/
}