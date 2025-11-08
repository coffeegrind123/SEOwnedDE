#pragma once

#include "../../TF2/c_tf_player.h"

enum class EEntGroup
{
	PLAYERS_ALL,
	PLAYERS_ENEMIES,
	PLAYERS_TEAMMATES,
	PLAYERS_OBSERVER,

	BUILDINGS_ALL,
	BUILDINGS_ENEMIES,
	BUILDINGS_TEAMMATES,

	PROJECTILES_ALL,
	PROJECTILES_ENEMIES,
	PROJECTILES_TEAMMATES,
	PROJECTILES_LOCAL_STICKIES,

	HEALTHPACKS,
	AMMOPACKS,
	HALLOWEEN_GIFT,
	MVM_MONEY
};

struct CachedEntityData
{
	matrix3x4_t transform;
	Vec3 renderOrigin;
	Vec3 mins;
	Vec3 maxs;
	int frameNumber;
	matrix3x4_t boneMatrix[MAXSTUDIOBONES];
	bool bBonesValid;
};

class CEntityHelper
{
public:
	C_TFPlayer* GetLocal();
	C_TFWeaponBase* GetWeapon();

private:
	std::map<EEntGroup, std::vector<C_BaseEntity*>> m_mapGroups = {};
	std::map<EEntGroup, std::vector<C_BaseEntity*>> m_mapGroupsTemp = {};
	std::map<int, bool> m_mapHealthPacks = {};
	std::map<int, bool> m_mapAmmoPacks = {};
	std::unordered_map<C_BaseEntity*, CachedEntityData> m_cachedEntityData = {};
	std::unordered_map<C_BaseEntity*, CachedEntityData> m_cachedEntityDataTemp = {};
	int m_nLastRenderCacheFrame = -1;

	bool IsHealthPack(C_BaseEntity* pEntity)
	{
		return m_mapHealthPacks.contains(pEntity->m_nModelIndex());
	}

	bool IsAmmoPack(C_BaseEntity* pEntity)
	{
		return m_mapAmmoPacks.contains(pEntity->m_nModelIndex());
	}

public:
	void UpdateCache();
	void UpdateRenderCache();
	void UpdateModelIndexes();
	void ClearCache(bool bClearAll = false);

	void ClearModelIndexes()
	{
		m_mapHealthPacks.clear();
		m_mapAmmoPacks.clear();
	}

	const std::vector<C_BaseEntity*>& GetGroup(const EEntGroup group) { return m_mapGroups[group]; }

	const CachedEntityData* GetCachedData(C_BaseEntity* pEntity) const
	{
		auto it = m_cachedEntityData.find(pEntity);
		return (it != m_cachedEntityData.end()) ? &it->second : nullptr;
	}
};

MAKE_SINGLETON_SCOPED(CEntityHelper, Entities, H);
