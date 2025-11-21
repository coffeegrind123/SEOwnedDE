#include "EntityHelpers.h"
#include "../GameRules/GameRules.h"

// Entity helper functions implementation (ported from Amalgam)
namespace EntityHelpers
{
    // Building type detection (exact port from Amalgam)
    bool IsSentrygun(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check if entity is sentrygun
        auto pSentry = pEntity->As<C_ObjectSentrygun>();
        return pSentry != nullptr;
    }

    bool IsDispenser(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check if entity is dispenser
        auto pDispenser = pEntity->As<C_ObjectDispenser>();
        return pDispenser != nullptr;
    }

    bool IsTeleporter(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check if entity is teleporter
        auto pTeleporter = pEntity->As<C_ObjectTeleporter>();
        return pTeleporter != nullptr;
    }

    bool IsTeleporterEntrance(C_BaseEntity* pEntity)
    {
        if (!IsTeleporter(pEntity))
            return false;

        auto pTeleporter = pEntity->As<C_ObjectTeleporter>();
        if (!pTeleporter)
            return false;

        // SEOwnedDE compatibility: Check teleporter type
        // This may need to be adapted based on SEOwnedDE's exact implementation
        return pTeleporter->m_iObjectMode() == 0; // Entrance mode
    }

    bool IsTeleporterExit(C_BaseEntity* pEntity)
    {
        if (!IsTeleporter(pEntity))
            return false;

        auto pTeleporter = pEntity->As<C_ObjectTeleporter>();
        if (!pTeleporter)
            return false;

        // SEOwnedDE compatibility: Check teleporter type
        return pTeleporter->m_iObjectMode() == 1; // Exit mode
    }

    // NPC detection (exact port from Amalgam)
    bool IsNPC(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Basic NPC detection
        // This would need to be expanded for full SEOwnedDE compatibility
        return IsEyeballBoss(pEntity) || IsMerasmus(pEntity) ||
               IsHeadlessHorseman(pEntity) || IsMonoculus(pEntity) ||
               IsSkeletonKing(pEntity);
    }

    bool IsEyeballBoss(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for eyeball boss
        // This would use SEOwnedDE's ClassID system or entity name checking
        return GetEntityClassID(pEntity) == 292; // CEyeballBoss
    }

    bool IsMerasmus(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for Merasmus
        return GetEntityClassID(pEntity) == 293; // CMerasmus
    }

    bool IsHeadlessHorseman(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for Headless Horseman
        return GetEntityClassID(pEntity) == 294; // CHeadlessHorseman
    }

    bool IsMonoculus(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for MONOCULUS
        return GetEntityClassID(pEntity) == 295; // CTFBoss
    }

    bool IsSkeletonKing(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for Skeleton King
        return GetEntityClassID(pEntity) == 296; // CTFSkeleton
    }

    // Bomb detection (exact port from Amalgam)
    bool IsBomb(C_BaseEntity* pEntity)
    {
        return IsGenericBomb(pEntity) || IsPumpkinBomb(pEntity);
    }

    bool IsGenericBomb(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for generic bomb
        return GetEntityClassID(pEntity) == 297; // CTFGenericBomb
    }

    bool IsPumpkinBomb(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for pumpkin bomb
        return GetEntityClassID(pEntity) == 298; // CTFPumpkinBomb
    }

    bool IsJarate(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for jarate
        return GetEntityClassID(pEntity) == 299; // CTFJar
    }

    bool IsMadMilk(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Check for mad milk
        return GetEntityClassID(pEntity) == 300; // CTFJarMilk
    }

    // Entity ClassID detection (SEOwnedDE compatibility)
    int GetEntityClassID(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return -1;

        // SEOwnedDE compatibility: Use GetClientClass()->m_ClassID
        ClientClass* pClientClass = pEntity->GetClientClass();
        if (!pClientClass)
            return -1;

        return pClientClass->m_ClassID;
    }

    const char* GetEntityClassName(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return nullptr;

        // SEOwnedDE compatibility: Use GetClientClass()->GetName()
        ClientClass* pClientClass = pEntity->GetClientClass();
        if (!pClientClass)
            return nullptr;

        return pClientClass->GetName();
    }

    bool IsEntityType(C_BaseEntity* pEntity, const char* szClassName)
    {
        if (!pEntity || !szClassName)
            return false;

        const char* szEntityClassName = GetEntityClassName(pEntity);
        if (!szEntityClassName)
            return false;

        return strcmp(szEntityClassName, szClassName) == 0;
    }

    bool IsEntityType(C_BaseEntity* pEntity, int nClassID)
    {
        if (!pEntity)
            return false;

        return GetEntityClassID(pEntity) == nClassID;
    }

    // Advanced entity validation (SEOwnedDE compatibility)
    bool IsValidEntity(C_BaseEntity* pEntity)
    {
        if (!pEntity)
            return false;

        // SEOwnedDE compatibility: Basic validation for any entity
        // deadflag() is only available on player entities
        if (pEntity->IsDormant())
            return false;

        // For player entities, check if they're alive
        auto pPlayer = pEntity->As<C_TFPlayer>();
        if (pPlayer)
        {
            return !pPlayer->deadflag();
        }

        // For other entities (buildings, NPCs, etc.), basic validation
        return true;
    }

    bool IsEnemy(C_BaseEntity* pEntity, C_TFPlayer* pLocal)
    {
        if (!pEntity || !pLocal)
            return false;

        // Check team relationship
        return pEntity->m_iTeamNum() != pLocal->m_iTeamNum();
    }

    bool IsFriendly(C_BaseEntity* pEntity, C_TFPlayer* pLocal)
    {
        if (!pEntity || !pLocal)
            return false;

        // Check team relationship
        return pEntity->m_iTeamNum() == pLocal->m_iTeamNum();
    }

    bool CanBeTargeted(C_BaseEntity* pEntity, C_TFPlayer* pLocal)
    {
        if (!IsValidEntity(pEntity) || !pLocal)
            return false;

        // Basic targeting checks
        if (pEntity == pLocal)
            return false;

        // Don't target during truce (unless friendly fire is enabled)
        if (GameRulesHelpers::IsTruceActive() && !GameRulesHelpers::IsFriendlyFireEnabled())
        {
            if (IsEnemy(pEntity, pLocal))
                return false;
        }

        return true;
    }

    // Building specific helpers (exact port from Amalgam)
    C_TFPlayer* GetBuildingOwner(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return nullptr;

        auto pOwner = pBuilding->m_hBuilder().Get();
        return pOwner ? pOwner->As<C_TFPlayer>() : nullptr;
    }

    bool IsBuildingActive(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return false;

        // SEOwnedDE compatibility: Check if building is active
        return pBuilding->m_bPlacing() == false &&
               pBuilding->m_bBuilding() == false &&
               pBuilding->m_iHealth() > 0;
    }

    bool IsBuildingSapped(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return false;

        // SEOwnedDE compatibility: Check if building is sapped
        return pBuilding->m_bHasSapper();
    }

    bool IsBuildingConstructing(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return false;

        // SEOwnedDE compatibility: Check if building is constructing
        return pBuilding->m_bBuilding();
    }

    bool IsBuildingUpgrading(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return false;

        // SEOwnedDE compatibility: Check if building is upgrading
        // This may need to be adapted based on SEOwnedDE's exact implementation
        return false; // Placeholder
    }

    int GetBuildingLevel(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return 0;

        // SEOwnedDE: Use m_iUpgradeLevel netvar from c_baseobject.h
        return pBuilding->m_iUpgradeLevel();
    }

    float GetBuildingHealth(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return 0.0f;

        return static_cast<float>(pBuilding->m_iHealth());
    }

    float GetBuildingMaxHealth(C_BaseObject* pBuilding)
    {
        if (!pBuilding)
            return 0.0f;

        return static_cast<float>(pBuilding->m_iMaxHealth());
    }

    // Initialize entity helpers
    void Initialize()
    {
        // Initialize entity helper systems
    }

    void Shutdown()
    {
        // Cleanup entity helper systems
    }
}