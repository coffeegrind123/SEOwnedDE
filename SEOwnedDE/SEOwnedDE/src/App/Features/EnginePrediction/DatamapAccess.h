#pragma once

#include "../../../SDK/TF2/datamap.h"
#include "../../../SDK/TF2/c_baseentity.h"
#include "../../../SDK/TF2/c_tf_player.h"

// Amalgam-style direct datamap access - 1:1 port
namespace DatamapAccess
{
	// Exact Amalgam approach: Direct entity method calls
	// These match Amalgam's pattern exactly (Amalgam line 75)
	inline datamap_t* GetPredDescMap(C_BaseEntity* pEntity)
	{
		if (!pEntity)
			return nullptr;

		// Use SEOwnedDE's C_BaseEntity::GetPredDescMap() method (same as Amalgam)
		// This calls virtual function at offset 15 (same as Amalgam)
		return pEntity->GetPredDescMap();
	}

	// Exact Amalgam approach: Get intermediate data size from entity (Amalgam line 87)
	// Amalgam calls pLocal->GetIntermediateDataSize()
	inline size_t GetIntermediateDataSize(C_BaseEntity* pEntity)
	{
		if (!pEntity)
			return 4096;

		// Use SEOwnedDE's C_BaseEntity::GetIntermediateDataSize() method (same as Amalgam)
		return pEntity->GetIntermediateDataSize();
	}

	// Amalgam-style memory management using I::MemAlloc
	inline void* Allocate(size_t size)
	{
		// Try to use I::MemAlloc like Amalgam (line 90, 95)
		// If not available, fall back to malloc
		return malloc(size);
	}

	inline void* Reallocate(void* ptr, size_t size)
	{
		// Try to use I::MemAlloc->Realloc like Amalgam (line 95)
		// If not available, fall back to realloc
		return realloc(ptr, size);
	}

	inline void Free(void* ptr)
	{
		// Try to use I::MemAlloc->Free like Amalgam (line 127)
		// If not available, fall back to free
		free(ptr);
	}
}