#include "EnginePrediction.h"
#include "CPredictionCopy.h"
#include "DatamapAccess.h"

#include "../Ticks/Ticks.h"
#include "../Aimbot/GlobalState.h"

// account for interp and origin compression when simulating local player (exact Amalgam port)
void CEnginePrediction::AdjustPlayers(C_TFPlayer* pLocal)
{
	m_mRestore.clear();

	for (auto pEntity : H::Entities->GetGroup(EEntGroup::PLAYERS_ALL))
	{
		auto pPlayer = pEntity->As<C_TFPlayer>();
		if (!pPlayer || pPlayer == pLocal || pPlayer->m_lifeState() != LIFE_ALIVE || pPlayer->InCond(TF_COND_HALLOWEEN_GHOST_MODE))
			continue;

		m_mRestore[pPlayer] = { pPlayer->GetAbsOrigin(), pPlayer->m_vecMins(), pPlayer->m_vecMaxs() };

		pPlayer->SetAbsOrigin(pPlayer->m_vecOrigin());
		pPlayer->m_vecMins() += 0.125f;
		pPlayer->m_vecMaxs() -= 0.125f;
	}
}
void CEnginePrediction::RestorePlayers()
{
	for (auto& [pPlayer, tRestore] : m_mRestore)
	{
		pPlayer->SetAbsOrigin(tRestore.m_vOrigin);
		pPlayer->m_vecMins() = tRestore.m_vMins;
		pPlayer->m_vecMaxs() = tRestore.m_vMaxs;
	}
}

void CEnginePrediction::Simulate(C_TFPlayer* pLocal, CUserCmd* pCmd)
{
	// Exact Amalgam Simulate method - 1:1 port (Amalgam lines 34-65)
	const int nOldTickBase = pLocal->m_nTickBase();
	const bool bOldIsFirstPrediction = I::Prediction->m_bFirstTimePredicted;
	const bool bOldInPrediction = I::Prediction->m_bInPrediction;

	I::MoveHelper->SetHost(pLocal);
	pLocal->SetCurrentCommand(pCmd);  // SEOwnedDE method (Amalgam calls m_pCurrentCommand())
	*SDKUtils::RandomSeed() = MD5_PseudoRandom(pCmd->command_number) & std::numeric_limits<int>::max();  // SEOwnedDE method

	I::Prediction->m_bFirstTimePredicted = false;
	I::Prediction->m_bInPrediction = true;
	I::Prediction->SetLocalViewAngles(pCmd->viewangles);

	AdjustPlayers(pLocal);
	I::Prediction->SetupMove(pLocal, pCmd, I::MoveHelper, &m_MoveData);
	I::GameMovement->ProcessMovement(pLocal, &m_MoveData);
	I::Prediction->FinishMove(pLocal, pCmd, &m_MoveData);
	RestorePlayers();

	I::MoveHelper->SetHost(nullptr);
	pLocal->SetCurrentCommand(nullptr);  // SEOwnedDE method (Amalgam calls m_pCurrentCommand())
	*SDKUtils::RandomSeed() = -1;  // SEOwnedDE method (Amalgam uses G::RandomSeed())

	pLocal->m_nTickBase() = nOldTickBase;
	I::Prediction->m_bFirstTimePredicted = bOldIsFirstPrediction;
	I::Prediction->m_bInPrediction = bOldInPrediction;

	m_vOrigin = m_MoveData.m_vecAbsOrigin;
	m_vVelocity = m_MoveData.m_vecVelocity;
	m_vDirection = { m_MoveData.m_flForwardMove, -m_MoveData.m_flSideMove, m_MoveData.m_flUpMove };
	m_vAngles = m_MoveData.m_vecViewAngles;
}



void CEnginePrediction::Start(C_TFPlayer* pLocal, CUserCmd* pCmd)
{
	// Amalgam's exact Start method with SEOwnedDE systems (Amalgam lines 69-103)
	m_bInPrediction = true;
	if (pLocal->m_lifeState() != LIFE_ALIVE)  // SEOwnedDE method (Amalgam uses IsAlive())
		return;

	// Use Amalgam's exact approach: Direct datamap access like pLocal->GetPredDescMap()
	auto pMap = DatamapAccess::GetPredDescMap(pLocal);
	if (!pMap)
		return;  // Amalgam exact behavior - return if no datamap

	m_nOldTickCount = I::GlobalVars->tickcount;
	m_flOldCurrentTime = I::GlobalVars->curtime;
	m_flOldFrameTime = I::GlobalVars->frametime;

	I::GlobalVars->tickcount = pLocal->m_nTickBase();
	I::GlobalVars->curtime = TICKS_TO_TIME(I::GlobalVars->tickcount);
	I::GlobalVars->frametime = I::Prediction->m_bEnginePaused ? 0.f : TICK_INTERVAL;

	// Use Amalgam's exact size calculation: pLocal->GetIntermediateDataSize()
	size_t iSize = DatamapAccess::GetIntermediateDataSize(pLocal);
	if (!m_tLocal.m_pData)
	{
		m_tLocal.m_pData = static_cast<byte*>(DatamapAccess::Allocate(iSize));  // Use Amalgam-style memory
		m_tLocal.m_iSize = iSize;
	}
	else if (m_tLocal.m_iSize != iSize)
	{
		m_tLocal.m_pData = static_cast<byte*>(DatamapAccess::Reallocate(m_tLocal.m_pData, iSize));  // Use Amalgam-style memory
		m_tLocal.m_iSize = iSize;
	}

	// Use Amalgam's exact CPredictionCopy creation and call (Amalgam line 99-100)
	CPredictionCopy copy(PC_EVERYTHING, m_tLocal.m_pData, PC_DATA_PACKED, pLocal, PC_DATA_NORMAL);
	copy.TransferData("EnginePredictionStart", pLocal->entindex(), pMap);

	Simulate(pLocal, pCmd);
}

void CEnginePrediction::End(C_TFPlayer* pLocal, CUserCmd* pCmd)
{
	// Amalgam's exact End method (Amalgam lines 105-121)
	m_bInPrediction = false;
	if (pLocal->m_lifeState() != LIFE_ALIVE)  // SEOwnedDE method (Amalgam uses IsAlive())
		return;

	// Use Amalgam's exact approach: Direct datamap access like pLocal->GetPredDescMap()
	auto pMap = DatamapAccess::GetPredDescMap(pLocal);
	if (!pMap)
		return;  // Amalgam exact behavior - return if no datamap

	// Restore original global state (exact Amalgam approach)
	I::GlobalVars->tickcount = m_nOldTickCount;
	I::GlobalVars->curtime = m_flOldCurrentTime;
	I::GlobalVars->frametime = m_flOldFrameTime;

	// Use Amalgam's exact CPredictionCopy creation and call (Amalgam line 119-120)
	CPredictionCopy copy(PC_EVERYTHING, pLocal, PC_DATA_NORMAL, m_tLocal.m_pData, PC_DATA_PACKED);
	copy.TransferData("EnginePredictionEnd", pLocal->entindex(), pMap);
}

void CEnginePrediction::Unload()
{
	// Amalgam's exact Unload method (Amalgam lines 123-129)
	if (m_tLocal.m_pData)
	{
		DatamapAccess::Free(m_tLocal.m_pData);  // Use Amalgam-style memory management
		m_tLocal = {};
	}
}