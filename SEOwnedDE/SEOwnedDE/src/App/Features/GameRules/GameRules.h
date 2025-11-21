#pragma once

#include "../../../SDK/SDK.h"

// GameRules interface and functionality (ported from Amalgam, adapted for SEOwnedDE)

class CGameRules
{
public:
    virtual ~CGameRules() = default;

    // Core GameRules methods (ported from Amalgam)
    virtual bool IsMultiplayer() = 0;
    virtual bool IsTeamplay() = 0;
    virtual bool IsDeathmatch() = 0;
    virtual bool IsCoop() = 0;
    virtual bool IsPVPRound() = 0;
    virtual bool IsPVPGunGame() = 0;
    virtual bool IsSkillBasedGameMode() = 0;
};

// TF2 GameRules implementation (simplified and functional)
class CTFGameRules : public CGameRules
{
public:
    // Truce system - critical for aimbot functionality (ported from Amalgam)
    bool m_bTruceActive = false;

    // Basic implementation
    bool IsMultiplayer() override { return true; }
    bool IsTeamplay() override { return true; }
    bool IsDeathmatch() override { return false; }
    bool IsCoop() override { return false; }
    bool IsPVPRound() override { return true; }
    bool IsPVPGunGame() override { return false; }
    bool IsSkillBasedGameMode() override { return false; }
};

// GameRules helper functions (ported from Amalgam)
namespace GameRulesHelpers
{
    // Get current GameRules instance
    CTFGameRules* GetTFGameRules();

    // Check if friendly fire is enabled
    bool IsFriendlyFireEnabled();

    // Check if truce is active
    bool IsTruceActive();

    // Check if game mode allows attacking
    bool CanAttack(C_TFPlayer* pLocal, C_TFPlayer* pTarget);

    // Check if building can be attacked
    bool CanAttackBuilding(C_TFPlayer* pLocal, C_BaseObject* pBuilding);

    // Initialize GameRules system
    void Initialize();
    void Shutdown();
}