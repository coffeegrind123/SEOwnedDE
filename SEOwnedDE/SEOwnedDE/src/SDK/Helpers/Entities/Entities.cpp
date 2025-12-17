#include "Entities.h"
#include "../../TF2/icliententitylist.h"
#include "../../TF2/ivmodelinfo.h"
#include "../../TF2/c_baseentity.h"
#include "../../SDK.h"

C_TFPlayer* CEntityHelper::GetLocal()
{
	if (const auto pEntity = I::ClientEntityList->GetClientEntity(I::EngineClient->GetLocalPlayer()))
		return pEntity->As<C_TFPlayer>();

	return nullptr;
}

C_TFWeaponBase* CEntityHelper::GetWeapon()
{
	if (const auto pLocal = GetLocal())
	{
		if (const auto pEntity = pLocal->m_hActiveWeapon().Get())
			return pEntity->As<C_TFWeaponBase>();
	}

	return nullptr;
}

void CEntityHelper::UpdateCache()
{
	if (const auto pLocal = GetLocal())
	{
		int nLocalTeam = 0;

		if (!pLocal->IsInValidTeam(&nLocalTeam))
			return;

		// Build entity list in TEMPORARY buffer to prevent data race with rendering thread
		for (int n = 1; n < I::ClientEntityList->GetHighestEntityIndex(); n++)
		{
			IClientEntity* pClientEntity = I::ClientEntityList->GetClientEntity(n);

			if (!pClientEntity || pClientEntity->IsDormant())
				continue;

			auto pEntity = pClientEntity->As<C_BaseEntity>();

			switch (pEntity->GetClassId())
			{
			case ETFClassIds::CTFPlayer:
				{
					int nPlayerTeam = 0;

					const auto pPlayer = pEntity->As<C_TFPlayer>();
					if (pPlayer->deadflag() && pPlayer->m_iObserverMode() != OBS_MODE_NONE)
					{
						m_mapGroupsTemp[EEntGroup::PLAYERS_OBSERVER].push_back(pEntity);
					}

					if (!pEntity->IsInValidTeam(&nPlayerTeam))
						continue;

					m_mapGroupsTemp[EEntGroup::PLAYERS_ALL].push_back(pEntity);
					m_mapGroupsTemp[nLocalTeam != nPlayerTeam ? EEntGroup::PLAYERS_ENEMIES : EEntGroup::PLAYERS_TEAMMATES].push_back(pEntity);

					break;
				}

			case ETFClassIds::CObjectSentrygun:
			case ETFClassIds::CObjectDispenser:
			case ETFClassIds::CObjectTeleporter:
				{
					int nObjectTeam = 0;

					if (!pEntity->IsInValidTeam(&nObjectTeam))
						continue;

					m_mapGroupsTemp[EEntGroup::BUILDINGS_ALL].push_back(pEntity);
					m_mapGroupsTemp[nLocalTeam != nObjectTeam ? EEntGroup::BUILDINGS_ENEMIES : EEntGroup::BUILDINGS_TEAMMATES].push_back(pEntity);

					break;
				}

			case ETFClassIds::CTFProjectile_Rocket:
			case ETFClassIds::CTFProjectile_SentryRocket:
			case ETFClassIds::CTFProjectile_Jar:
			case ETFClassIds::CTFProjectile_JarGas:
			case ETFClassIds::CTFProjectile_JarMilk:
			case ETFClassIds::CTFProjectile_Arrow:
			case ETFClassIds::CTFProjectile_Flare:
			case ETFClassIds::CTFProjectile_Cleaver:
			case ETFClassIds::CTFProjectile_HealingBolt:
			case ETFClassIds::CTFGrenadePipebombProjectile:
			case ETFClassIds::CTFProjectile_BallOfFire:
			case ETFClassIds::CTFProjectile_EnergyRing:
			case ETFClassIds::CTFProjectile_EnergyBall:
				{
					int nProjectileTeam = 0;

					if (!pEntity->IsInValidTeam(&nProjectileTeam))
						continue;

					if (pEntity->GetClassId() == ETFClassIds::CTFGrenadePipebombProjectile)
					{
						const auto pPipebomb = pEntity->As<C_TFGrenadePipebombProjectile>();

						/*if (pPipebomb->m_iType() == TF_GL_MODE_REMOTE_DETONATE_PRACTICE)
							continue;*/

						if (pPipebomb->HasStickyEffects() && pPipebomb->As<C_BaseGrenade>()->m_hThrower().Get() == pLocal)
							m_mapGroupsTemp[EEntGroup::PROJECTILES_LOCAL_STICKIES].push_back(pEntity);
					}

					m_mapGroupsTemp[EEntGroup::PROJECTILES_ALL].push_back(pEntity);
					m_mapGroupsTemp[nLocalTeam != nProjectileTeam ? EEntGroup::PROJECTILES_ENEMIES : EEntGroup::PROJECTILES_TEAMMATES].push_back(pEntity);

					break;
				}

			case ETFClassIds::CBaseAnimating:
				{
					if (IsHealthPack(pEntity))
						m_mapGroupsTemp[EEntGroup::HEALTHPACKS].push_back(pEntity);

					if (IsAmmoPack(pEntity))
						m_mapGroupsTemp[EEntGroup::AMMOPACKS].push_back(pEntity);

					break;
				}

			case ETFClassIds::CTFAmmoPack:
				{
					m_mapGroupsTemp[EEntGroup::AMMOPACKS].push_back(pEntity);
					break;
				}

			case ETFClassIds::CHalloweenGiftPickup:
				{
					m_mapGroupsTemp[EEntGroup::HALLOWEEN_GIFT].push_back(pEntity);

					break;
				}

			case ETFClassIds::CCurrencyPack:
				{
					if (pEntity->As<C_CurrencyPack>()->m_bDistributed())
					{
						continue;
					}

					m_mapGroupsTemp[EEntGroup::MVM_MONEY].push_back(pEntity);

					break;
				}

			default: break;
			}
		}

		// ATOMIC SWAP: Replace the real list with the temp list in one operation
		std::swap(m_mapGroups, m_mapGroupsTemp);
	}
}

void CEntityHelper::UpdateRenderCache()
{
	// Skip if not in-game (prevents crash during level transitions)
	if (!I::EngineClient->IsInGame())
	{
		m_cachedEntityDataTemp.clear();
		std::swap(m_cachedEntityData, m_cachedEntityDataTemp);
		m_nLastRenderCacheFrame = -1;
		return;
	}

	// Additional safety: check if we have a valid local player (indicates we're actually in a playable state)
	const auto pLocal = GetLocal();
	if (!pLocal)
	{
		m_cachedEntityDataTemp.clear();
		std::swap(m_cachedEntityData, m_cachedEntityDataTemp);
		m_nLastRenderCacheFrame = -1;
		return;
	}

	// Cache interpolated render data ONCE per render frame at FRAME_RENDER_START
	// Prevent multiple updates in the same frame
	const int currentFrame = I::GlobalVars->framecount;
	if (m_nLastRenderCacheFrame == currentFrame)
		return;

	m_nLastRenderCacheFrame = currentFrame;


	m_cachedEntityDataTemp.clear();

	// Only iterate through "ALL" groups to avoid processing same entity multiple times
	static const EEntGroup groupsToCache[] = {
		EEntGroup::PLAYERS_ALL,
		EEntGroup::BUILDINGS_ALL,
		EEntGroup::PROJECTILES_ALL,
		EEntGroup::HEALTHPACKS,
		EEntGroup::AMMOPACKS,
		EEntGroup::HALLOWEEN_GIFT,
		EEntGroup::MVM_MONEY
	};

	for (const auto group : groupsToCache)
	{
		// Safe lookup - check if group exists in map first
		auto it = m_mapGroups.find(group);
		if (it == m_mapGroups.end())
			continue;

		const auto& entities = it->second;

		// Skip if entity list is empty
		if (entities.empty())
			continue;

		for (auto pEntity : entities)
		{
			if (!pEntity)
				continue;

			// Wrap in try-catch to prevent crashes from accessing freed entities during transitions
			try
			{
				if (pEntity->IsDormant())
					continue;

				CachedEntityData data = {};
				data.frameNumber = currentFrame;
				data.bBonesValid = false;

				// Get interpolated transform and origin (called ONCE per frame, safe)
				const auto& entityTransform = pEntity->RenderableToWorldTransform();
				memcpy(&data.transform, &entityTransform, sizeof(matrix3x4_t));
				data.renderOrigin = pEntity->GetRenderOrigin();

				// Skip entities with completely invalid transforms only (extreme coordinates)
				if (abs(data.renderOrigin.x) > 100000.0f || abs(data.renderOrigin.y) > 100000.0f ||
					abs(data.renderOrigin.z) > 100000.0f) {
					continue; // Entity has garbage coordinates - skip this frame
				}

				// Get bounds per entity type
				switch (pEntity->GetClassId())
				{
				case ETFClassIds::CTFPlayer:
					{
						const auto pPlayer = pEntity->As<C_TFPlayer>();

						// Relaxed player validation - only skip truly invalid states
						if (pPlayer->IsDormant()) {
							data.bBonesValid = false;
							// Still cache bounds but don't attempt SetupBones for dormant players
							data.mins = pPlayer->m_vecMins();
							data.maxs = pPlayer->m_vecMaxs();
							break;
						}

						data.mins = pPlayer->m_vecMins();
						data.maxs = pPlayer->m_vecMaxs();

						// Cache bone matrices with relaxed validation
						data.bBonesValid = false;
						if (!pPlayer->deadflag() || pPlayer->m_iHealth() > 0) {
							// Only attempt SetupBones for players who might need bones
							if (pPlayer->SetupBones(data.boneMatrix, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, I::GlobalVars->curtime)) {
								data.bBonesValid = true;
							}
						}
						break;
					}
				case ETFClassIds::CObjectSentrygun:
				case ETFClassIds::CObjectDispenser:
				case ETFClassIds::CObjectTeleporter:
					{
						data.mins = pEntity->m_vecMins();
						data.maxs = pEntity->m_vecMaxs();
						break;
					}
				default:
					{
						pEntity->GetRenderBounds(data.mins, data.maxs);
						break;
					}
				}

				m_cachedEntityDataTemp[pEntity] = data;
			}
			catch (...)
			{
				// Silently skip entities that cause access violations (freed during transition)
				continue;
			}
		}
	}

	std::swap(m_cachedEntityData, m_cachedEntityDataTemp);
}

void CEntityHelper::UpdateModelIndexes()
{
	m_mapHealthPacks.clear();
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/medkit_small.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/medkit_medium.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/medkit_large.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/props_halloween/halloween_medkit_small.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/props_halloween/halloween_medkit_medium.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/props_halloween/halloween_medkit_large.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/medkit_small_bday.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/medkit_medium_bday.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/medkit_large_bday.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/props_medieval/medieval_meat.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/plate.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/plate_sandwich_xmas.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/plate_robo_sandwich.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/workshop/weapons/c_models/c_fishcake/plate_fishcake.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/workshop/weapons/c_models/c_buffalo_steak/plate_buffalo_steak.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/workshop/weapons/c_models/c_chocolate/plate_chocolate.mdl")] = true;
	m_mapHealthPacks[I::ModelInfoClient->GetModelIndex("models/items/banana/plate_banana.mdl")] = true;

	m_mapAmmoPacks.clear();
	m_mapAmmoPacks[I::ModelInfoClient->GetModelIndex("models/items/ammopack_small.mdl")] = true;
	m_mapAmmoPacks[I::ModelInfoClient->GetModelIndex("models/items/ammopack_medium.mdl")] = true;
	m_mapAmmoPacks[I::ModelInfoClient->GetModelIndex("models/items/ammopack_large.mdl")] = true;
	m_mapAmmoPacks[I::ModelInfoClient->GetModelIndex("models/items/ammopack_small_bday.mdl")] = true;
	m_mapAmmoPacks[I::ModelInfoClient->GetModelIndex("models/items/ammopack_medium_bday.mdl")] = true;
	m_mapAmmoPacks[I::ModelInfoClient->GetModelIndex("models/items/ammopack_large_bday.mdl")] = true;
}

void CEntityHelper::ClearCache(bool bClearAll)
{
	// Clear but keep capacity to avoid reallocations
	for (auto& group : m_mapGroupsTemp | std::views::values)
	{
		group.clear();
		// Reserve reasonable capacity based on typical TF2 server
		// Typical 24-player server: 24 players + ~20 buildings/projectiles
		if (group.capacity() < 32)
			group.reserve(32);
	}

	m_cachedEntityDataTemp.clear();
	// Reserve capacity for typical entity count
	if (m_cachedEntityDataTemp.bucket_count() < 64)
		m_cachedEntityDataTemp.reserve(64);

	// When called from LevelShutdown, also clear main buffers to prevent dangling pointers
	if (bClearAll)
	{
		for (auto& group : m_mapGroups | std::views::values)
		{
			group.clear();
			if (group.capacity() < 32)
				group.reserve(32);
		}

		m_cachedEntityData.clear();
		if (m_cachedEntityData.bucket_count() < 64)
			m_cachedEntityData.reserve(64);
	}
}
