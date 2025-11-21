#pragma once

#include "../../../SDK/SDK.h"

// Entity helper functions (ported from Amalgam, adapted for SEOwnedDE)

namespace EntityHelpers
{
    // Building type detection (ported from Amalgam)
    bool IsSentrygun(C_BaseEntity* pEntity);
    bool IsDispenser(C_BaseEntity* pEntity);
    bool IsTeleporter(C_BaseEntity* pEntity);
    bool IsTeleporterEntrance(C_BaseEntity* pEntity);
    bool IsTeleporterExit(C_BaseEntity* pEntity);

    // NPC detection (ported from Amalgam)
    bool IsNPC(C_BaseEntity* pEntity);
    bool IsEyeballBoss(C_BaseEntity* pEntity);
    bool IsMerasmus(C_BaseEntity* pEntity);
    bool IsHeadlessHorseman(C_BaseEntity* pEntity);
    bool IsMonoculus(C_BaseEntity* pEntity);
    bool IsSkeletonKing(C_BaseEntity* pEntity);

    // Bomb detection (ported from Amalgam)
    bool IsBomb(C_BaseEntity* pEntity);
    bool IsGenericBomb(C_BaseEntity* pEntity);
    bool IsPumpkinBomb(C_BaseEntity* pEntity);
    bool IsJarate(C_BaseEntity* pEntity);
    bool IsMadMilk(C_BaseEntity* pEntity);

    // Entity ClassID detection (ported from Amalgam)
    int GetEntityClassID(C_BaseEntity* pEntity);
    const char* GetEntityClassName(C_BaseEntity* pEntity);
    bool IsEntityType(C_BaseEntity* pEntity, const char* szClassName);
    bool IsEntityType(C_BaseEntity* pEntity, int nClassID);

    // Advanced entity validation (ported from Amalgam)
    bool IsValidEntity(C_BaseEntity* pEntity);
    bool IsEnemy(C_BaseEntity* pEntity, C_TFPlayer* pLocal);
    bool IsFriendly(C_BaseEntity* pEntity, C_TFPlayer* pLocal);
    bool CanBeTargeted(C_BaseEntity* pEntity, C_TFPlayer* pLocal);

    // Building specific helpers (ported from Amalgam)
    C_TFPlayer* GetBuildingOwner(C_BaseObject* pBuilding);
    bool IsBuildingActive(C_BaseObject* pBuilding);
    bool IsBuildingSapped(C_BaseObject* pBuilding);
    bool IsBuildingConstructing(C_BaseObject* pBuilding);
    bool IsBuildingUpgrading(C_BaseObject* pBuilding);
    int GetBuildingLevel(C_BaseObject* pBuilding);
    float GetBuildingHealth(C_BaseObject* pBuilding);
    float GetBuildingMaxHealth(C_BaseObject* pBuilding);

    // Initialize entity helpers
    void Initialize();
    void Shutdown();
}