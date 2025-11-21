#include "PredictionHelpers.h"
#include "DatamapAccess.h"

// NOTE: CPredictionCopy is now defined in CPredictionCopy.h/cpp

namespace PredictionHelpers
{
    // NOTE: GetPredDescMap is now handled by DatamapAccess namespace
    // Keeping this for backward compatibility
    datamap_t* GetPredDescMap(C_BaseEntity* pEntity)
    {
        return DatamapAccess::GetPredDescMap(pEntity);
    }

    // Enhanced datamap copy functions (SEOwnedDE adapted)
    void CopyDatamapToBuffer(C_BaseEntity* pEntity, byte* pBuffer)
    {
        if (!pEntity || !pBuffer)
            return;

        // SEOwnedDE: Store basic entity state using SEOwnedDE's NetVar system
        // Store essential fields that are safe to backup and restore
        if (auto pPlayer = pEntity->As<C_TFPlayer>())
        {
            // Store critical player state for restoration
            float* pData = reinterpret_cast<float*>(pBuffer);
            pData[0] = pPlayer->m_flSimulationTime(); // Simulation time
            pData[1] = pPlayer->m_flAnimTime();       // Animation time
            pData[2] = pPlayer->m_flOldSimulationTime(); // Old simulation time
        }
        else
        {
            // For non-players, clear buffer
            memset(pBuffer, 0, 4096);
        }
    }

    void CopyBufferToDatamap(C_BaseEntity* pEntity, byte* pBuffer)
    {
        if (!pEntity || !pBuffer)
            return;

        // SEOwnedDE: Restore entity state using SEOwnedDE's NetVar system
        if (auto pPlayer = pEntity->As<C_TFPlayer>())
        {
            // Restore critical player state
            float* pData = reinterpret_cast<float*>(pBuffer);

            // Only restore if values are reasonable
            if (pData[0] > 0.0f) pPlayer->m_flSimulationTime() = pData[0];
            if (pData[1] > 0.0f) pPlayer->m_flAnimTime() = pData[1];
            if (pData[2] > 0.0f) pPlayer->m_flOldSimulationTime() = pData[2];
        }
        // Non-player entities don't need restoration in this basic implementation
    }

    // Validate datamap operations
    bool IsValidDatamap(datamap_t* pMap)
    {
        return pMap != nullptr && pMap->dataDesc != nullptr && pMap->dataNumFields > 0;
    }

    int GetDatamapSize(datamap_t* pMap)
    {
        // SEOwnedDE: Calculate actual datamap size like Amalgam
        if (!IsValidDatamap(pMap))
            return 0;

        // Calculate total size of all datamap fields
        int totalSize = 0;
        for (int i = 0; i < pMap->dataNumFields; i++)
        {
            typedescription_t& field = pMap->dataDesc[i];
            if (field.fieldName && field.fieldSize > 0)
            {
                totalSize += field.fieldSize;
            }
        }

        // Return reasonable size (minimum 64 bytes, maximum 4096 bytes like Amalgam)
        return (totalSize > 4096) ? 4096 : (totalSize < 64) ? 64 : totalSize;
    }
}