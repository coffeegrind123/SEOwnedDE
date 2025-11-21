#include "GameRules.h"

// GameRules helper functions implementation (ported from Amalgam)
namespace GameRulesHelpers
{
    // Get current GameRules instance
    CTFGameRules* GetTFGameRules()
    {
        // SEOwnedDE compatibility: Basic GameRules access
        // In a full implementation, this would use SEOwnedDE's GameRules interface
        static CTFGameRules sTFGameRules;
        return &sTFGameRules;
    }

    // Check if friendly fire is enabled (ported from Amalgam)
    bool IsFriendlyFireEnabled()
    {
        static auto mp_friendlyfire = I::CVar->FindVar("mp_friendlyfire");
        return mp_friendlyfire ? mp_friendlyfire->GetBool() : false;
    }

    // Check if truce is active (ported from Amalgam)
    bool IsTruceActive()
    {
        CTFGameRules* pGameRules = GetTFGameRules();
        if (!pGameRules)
            return false;

        // Check various truce conditions
        return pGameRules->m_bTruceActive;
    }

    // Check if game mode allows attacking
    bool CanAttack(C_TFPlayer* pLocal, C_TFPlayer* pTarget)
    {
        if (!pLocal || !pTarget)
            return false;

        // During truce, no attacking allowed (except with friendly fire)
        if (IsTruceActive() && !IsFriendlyFireEnabled() && pLocal->m_iTeamNum() != pTarget->m_iTeamNum())
            return false;

        // Basic attack checks
        if (!pTarget || pTarget->deadflag() || pTarget->InCond(TF_COND_HALLOWEEN_GHOST_MODE))
            return false;

        if (!IsFriendlyFireEnabled() && pLocal->m_iTeamNum() == pTarget->m_iTeamNum())
            return false;

        return true;
    }

    // Check if building can be attacked
    bool CanAttackBuilding(C_TFPlayer* pLocal, C_BaseObject* pBuilding)
    {
        if (!pLocal || !pBuilding)
            return false;

        // During truce, no attacking buildings allowed
        if (IsTruceActive())
            return false;

        // Don't attack friendly buildings
        if (pLocal->m_iTeamNum() == pBuilding->m_iTeamNum())
            return false;

        return true;
    }

    // Initialize GameRules system
    void Initialize()
    {
        // Initialize GameRules state
        CTFGameRules* pGameRules = GetTFGameRules();
        if (pGameRules)
        {
            // Set default truce state (usually false)
            pGameRules->m_bTruceActive = false;
        }
    }

    void Shutdown()
    {
        // Cleanup GameRules system
    }
}