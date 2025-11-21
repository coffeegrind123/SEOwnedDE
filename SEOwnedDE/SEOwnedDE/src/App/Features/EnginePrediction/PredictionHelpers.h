#pragma once

#include "../../../SDK/SDK.h"
#include "CPredictionCopy.h"

// Engine prediction helper functions
namespace PredictionHelpers
{
    // Get prediction description map (Amalgam compatibility)
    datamap_t* GetPredDescMap(C_BaseEntity* pEntity);

    // Enhanced datamap copy functions
    void CopyDatamapToBuffer(C_BaseEntity* pEntity, byte* pBuffer);
    void CopyBufferToDatamap(C_BaseEntity* pEntity, byte* pBuffer);

    // Validate datamap operations
    bool IsValidDatamap(datamap_t* pMap);
    int GetDatamapSize(datamap_t* pMap);
}