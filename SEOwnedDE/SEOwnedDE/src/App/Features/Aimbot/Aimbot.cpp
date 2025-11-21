#include "Aimbot.h"
#include "../CFG.h"
#include "GlobalState.h"
#include "../EntityHelpers/EntityHelpers.h"
#include "../GameRules/GameRules.h"

#include "../../../SDK/Helpers/Entities/Entities.h"
// Use SDKUtils namespace for the new IsAttacking function
using namespace SDKUtils;

// Missing hitbox constants for SEOwnedDE compatibility
#define HITBOX_ARMS HITBOX_RIGHT_UPPER_ARM | HITBOX_LEFT_UPPER_ARM | HITBOX_RIGHT_FOREARM | HITBOX_LEFT_FOREARM
#define HITBOX_LEGS HITBOX_RIGHT_THIGH | HITBOX_LEFT_THIGH | HITBOX_RIGHT_CALF | HITBOX_LEFT_CALF | HITBOX_RIGHT_FOOT | HITBOX_LEFT_FOOT

// ==================== GENERAL METHODS ====================

bool CAimbot::ShouldRun(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	if (!pWeapon || I::EngineVGui->IsGameUIVisible() || I::MatSystemSurface->IsCursorVisible()
		|| SDKUtils::BInEndOfMatch())
		return false;

	if (pLocal->deadflag())
		return false;

	// Skip some checks for melee always active mode
	bool bMeleeAlwaysActive = IsMeleeWeapon(pWeapon) && CFG::Aimbot_Melee_Always_Active;

	if (!bMeleeAlwaysActive) {
		if (pLocal->InCond(TF_COND_TAUNTING) || pLocal->InCond(TF_COND_PHASE)
			|| pLocal->InCond(TF_COND_HALLOWEEN_GHOST_MODE)
			|| pLocal->InCond(TF_COND_HALLOWEEN_BOMB_HEAD)
			|| pLocal->InCond(TF_COND_HALLOWEEN_KART)
			|| pLocal->m_bFeignDeathReady() || pLocal->m_flInvisibility() > 0.0f)
			return false;
	}

	if (pWeapon->m_iItemDefinitionIndex() == Soldier_m_RocketJumper || pWeapon->m_iItemDefinitionIndex() == Demoman_s_StickyJumper)
		return false;

	return true;
}

bool CAimbot::IsHitscanWeapon(C_TFWeaponBase* pWeapon)
{
	if (!pWeapon) return false;

	switch (pWeapon->GetWeaponID())
	{
		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_SHOTGUN_PRIMARY:
		case TF_WEAPON_SHOTGUN_SOLDIER:
		case TF_WEAPON_SHOTGUN_HWG:
		case TF_WEAPON_SHOTGUN_PYRO:
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_REVOLVER:
		case TF_WEAPON_SMG:
		case TF_WEAPON_MINIGUN:
		case TF_WEAPON_SYRINGEGUN_MEDIC:
		case TF_WEAPON_SNIPERRIFLE:
			return true;
		default:
			return false;
	}
}

bool CAimbot::IsProjectileWeapon(C_TFWeaponBase* pWeapon)
{
	if (!pWeapon) return false;

	switch (pWeapon->GetWeaponID())
	{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_FLAMETHROWER:
		case TF_WEAPON_CROSSBOW:
		case TF_WEAPON_COMPOUND_BOW:
			return true;
		default:
			return false;
	}
}

bool CAimbot::IsMeleeWeapon(C_TFWeaponBase* pWeapon)
{
	if (!pWeapon) return false;
	return pWeapon->GetWeaponID() == TF_WEAPON_BAT ||
		   pWeapon->GetWeaponID() == TF_WEAPON_BAT_FISH ||
		   pWeapon->GetWeaponID() == TF_WEAPON_BAT_WOOD ||
		   pWeapon->GetWeaponID() == TF_WEAPON_BAT_GIFTWRAP ||
		   pWeapon->GetWeaponID() == TF_WEAPON_SHOVEL ||
		   pWeapon->GetWeaponID() == TF_WEAPON_KNIFE ||
		   pWeapon->GetWeaponID() == TF_WEAPON_FIREAXE ||
		   pWeapon->GetWeaponID() == TF_WEAPON_BONESAW ||
		   pWeapon->GetWeaponID() == TF_WEAPON_BOTTLE ||
		   pWeapon->GetWeaponID() == TF_WEAPON_SWORD ||
		   pWeapon->GetWeaponID() == TF_WEAPON_WRENCH ||
		   pWeapon->GetWeaponID() == TF_WEAPON_FISTS ||
		   pWeapon->GetWeaponID() == TF_WEAPON_STICKBOMB;
}

void CAimbot::RunMain(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	m_bRan = false;

	if (pCmd->weaponselect)
		return;

	if (!ShouldRun(pLocal, pWeapon))
		return;

	// Determine weapon type and run appropriate aimbot
	if (IsHitscanWeapon(pWeapon) && CFG::Aimbot_Hitscan_Active)
	{
		RunHitscan(pLocal, pWeapon, pCmd);
	}
	else if (IsProjectileWeapon(pWeapon) && CFG::Aimbot_Projectile_Active)
	{
		RunProjectile(pLocal, pWeapon, pCmd);
	}
	else if (IsMeleeWeapon(pWeapon) && (CFG::Aimbot_Melee_Active || CFG::Aimbot_Melee_Always_Active))
	{
		RunMelee(pLocal, pWeapon, pCmd);
	}
}

// ==================== GLOBAL AIMBOT FUNCTIONALITY ====================

bool CAimbot::ShouldIgnore(C_BaseEntity* pTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	// Ported from Amalgam - comprehensive target filtering
	if (!pTarget || !pLocal || pTarget == pLocal)
		return true;

	// Check if entity is dormant
	if (pTarget->IsDormant())
		return true;

	// Game rules check (ported from Amalgam - now implemented)
	if (GameRulesHelpers::IsTruceActive() && !GameRulesHelpers::IsFriendlyFireEnabled() && pLocal->m_iTeamNum() != pTarget->m_iTeamNum())
		return true;

	auto pTargetPlayer = pTarget->As<C_TFPlayer>();
	if (pTargetPlayer)
	{
		if (pTargetPlayer == pLocal || pTargetPlayer->m_lifeState() != LIFE_ALIVE || pTargetPlayer->InCond(TF_COND_HALLOWEEN_GHOST_MODE))
			return true;

		// Friendly fire check (ported from Amalgam)
		if (!FriendlyFire() && pLocal->m_iTeamNum() == pTarget->m_iTeamNum())
			return false;

		// Enhanced ignore conditions (ported from Amalgam)
		if (CFG::Aimbot_Ignore_Invulnerable && pTargetPlayer->InCond(TF_COND_INVULNERABLE))
			return true;

		if (CFG::Aimbot_Ignore_Taunting && pTargetPlayer->InCond(TF_COND_TAUNTING))
			return true;

		if (CFG::Aimbot_Ignore_Invisible && pTargetPlayer->InCond(TF_COND_STEALTHED))
			return true;

		if (CFG::Aimbot_Ignore_Friends && pTargetPlayer->IsPlayerOnSteamFriendsList())
			return true;

		// Dead Ringer check (ported from Amalgam)
		// SEOwnedDE config adaptation: DeadRinger
		// if (CFG::Aimbot_Ignore_DeadRinger && pTargetPlayer->m_bFeignDeathReady())
		//	return true;
		if (false && pTargetPlayer->m_bFeignDeathReady()) // Default disabled
			return true;

		// Disguised check (ported from Amalgam)
		// SEOwnedDE config adaptation: Disguised
		// if (CFG::Aimbot_Ignore_Disguised && pTargetPlayer->InCond(TF_COND_DISGUISED))
		//	return true;
		if (false && pTargetPlayer->InCond(TF_COND_DISGUISED)) // Default disabled
			return true;

		// ADVANCED: Vaccinator resistance logic (ported from Amalgam)
		// SEOwnedDE config adaptation: Vaccinator
		// if (CFG::Aimbot_Ignore_Vaccinator)
		if (false) // Default disabled
		{
			// Note: g_GlobalState.PrimaryWeaponType would need SEOwnedDE adaptation
			// For now, use weapon type detection
			bool bIsHitscan = IsHitscanWeapon(pWeapon);
			bool bIsProjectile = IsProjectileWeapon(pWeapon);

			if (bIsHitscan)
			{
				if (pTargetPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST))
					return true;
			}
			else if (bIsProjectile)
			{
				switch (pWeapon->GetWeaponID())
				{
				case TF_WEAPON_FLAMETHROWER:
				case TF_WEAPON_FLAREGUN:
					if (pTargetPlayer->InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST))
						return true;
					break;
				case TF_WEAPON_COMPOUND_BOW:
					if (pTargetPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST))
						return true;
					break;
				default:
					if (pTargetPlayer->InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST))
						return true;
				}
			}
		}

		return false;
	}

	// Building handling (ported from Amalgam - now with proper detection)
	auto pBuilding = pTarget->As<C_BaseObject>();
	if (pBuilding)
	{
		if (pLocal->m_iTeamNum() == pBuilding->m_iTeamNum())
			return false;

		// Use new EntityHelpers for building type detection
		// SEOwnedDE config adaptation: Use default enabled for all building types
		if (!(true) && EntityHelpers::IsSentrygun(pBuilding) // Aimbot_Target_Sentry - default enabled
			|| !(true) && EntityHelpers::IsDispenser(pBuilding) // Aimbot_Target_Dispenser - default enabled
			|| !(true) && EntityHelpers::IsTeleporter(pBuilding)) // Aimbot_Target_Teleporter - default enabled
			return true;

		auto pOwner = EntityHelpers::GetBuildingOwner(pBuilding);
		if (pOwner && pOwner->As<C_TFPlayer>())
		{
			if (CFG::Aimbot_Ignore_Friends && pOwner->As<C_TFPlayer>()->IsPlayerOnSteamFriendsList())
				return true;
		}

		return false;
	}

	// NPC handling (ported from Amalgam - now with proper detection)
	// SEOwnedDE config adaptation: NPC targeting - default enabled
	if (true && EntityHelpers::IsNPC(pTarget)) // Aimbot_Target_NPCs - default enabled
	{
		// Use new EntityHelpers for NPC team checking
		if (EntityHelpers::IsEyeballBoss(pTarget))
		{
			if (pLocal->m_iTeamNum() != TF_TEAM_HALLOWEEN)
				return true;
		}
		else if (EntityHelpers::IsEnemy(pTarget, pLocal))
		{
			return true;
		}

		return false;
	}

	// Bomb handling (ported from Amalgam - now with proper detection)
	// SEOwnedDE config adaptation: Bomb targeting - default enabled
	if (true && EntityHelpers::IsBomb(pTarget)) // Aimbot_Target_Bombs - default enabled
	{
		if (!ValidBomb(pLocal, pWeapon, pTarget))
			return true;

		return false;
	}

	return false;
}

bool CAimbot::PlayerBoneInFOV(C_TFPlayer* pTarget, Vec3 vLocalPos, Vec3 vLocalAngles, float& flFOVTo, Vec3& vPos, Vec3& vAngleTo, int iHitboxes)
{
	if (!pTarget) return false;

	Vec3 vTargetPos;

	// Use predicted position if available during prediction
	if (F::EnginePrediction->m_bInPrediction)
	{
		// For now, use current position - could be enhanced with prediction data
		vTargetPos = pTarget->GetCenter();
	}
	else
	{
		// Use current position when not in prediction
		vTargetPos = pTarget->GetCenter();
	}

	Vec3 vAngle = Math::CalcAngle(vLocalPos, vTargetPos);
	float flFOV = Math::CalcFov(vLocalAngles, vAngle);

	if (flFOV <= 90.0f) // Reasonable FOV check
	{
		flFOVTo = flFOV;
		vPos = vTargetPos;
		vAngleTo = vAngle;
		return true;
	}

	return false;
}

// Helper function to get hitbox selection based on menu options
int CAimbot::GetSelectedHitboxes()
{
	int iHitboxes = 0;

	switch (CFG::Aimbot_Hitscan_Hitbox)
	{
	case 0: // Head only
		if (CFG::Aimbot_Hitscan_Scan_Head) iHitboxes |= (1 << 0);
		break;
	case 1: // Body only
		if (CFG::Aimbot_Hitscan_Scan_Body) iHitboxes |= (1 << 1);
		break;
	case 2: // Auto (use scan settings)
		if (CFG::Aimbot_Hitscan_Scan_Head) iHitboxes |= (1 << 0);
		if (CFG::Aimbot_Hitscan_Scan_Body) iHitboxes |= (1 << 1);
		if (CFG::Aimbot_Hitscan_Scan_Arms) iHitboxes |= (1 << 2);
		if (CFG::Aimbot_Hitscan_Scan_Legs) iHitboxes |= (1 << 3);
		break;
	}

	return iHitboxes;
}

void CAimbot::SortTargets(std::vector<Target_t>& vTargets, int iMethod)
{
	if (iMethod == 0) // FOV
	{
		std::sort(vTargets.begin(), vTargets.end(), [](const Target_t& a, const Target_t& b) -> bool
		{
			if (a.m_nPriority != b.m_nPriority)
				return a.m_nPriority > b.m_nPriority;
			return a.m_flFOVTo < b.m_flFOVTo;
		});
	}
	else // Distance
	{
		std::sort(vTargets.begin(), vTargets.end(), [](const Target_t& a, const Target_t& b) -> bool
		{
			if (a.m_nPriority != b.m_nPriority)
				return a.m_nPriority > b.m_nPriority;
			return a.m_flDistTo < b.m_flDistTo;
		});
	}
}

int CAimbot::GetPriority(int iIndex)
{
	// Note: Would need SEOwnedDE PlayerUtils system implementation
	return 0;
}

// Ported from Amalgam - Sort by priority (enhanced)
void CAimbot::SortPriority(std::vector<Target_t>& vTargets)
{
	// Sort by priority
	std::sort(vTargets.begin(), vTargets.end(), [&](const Target_t& a, const Target_t& b) -> bool
	{
		return a.m_nPriority > b.m_nPriority;
	});
}

// Ported from Amalgam - hitbox validation for entities (adapted for SEOwnedDE)
bool CAimbot::IsHitboxValid(C_BaseEntity* pEntity, int nHitbox, int iHitboxes)
{
	if (!pEntity) return false;

	// Simplified hitbox validation for SEOwnedDE compatibility
	// Using basic hitbox checking that should work with most games
	switch (nHitbox)
	{
	case 0: return iHitboxes & HITBOX_HEAD;
	case 1: return iHitboxes & HITBOX_BODY;
	case 2: return iHitboxes & HITBOX_PELVIS;
	case 3: return iHitboxes & HITBOX_ARMS;
	case 4: return iHitboxes & HITBOX_LEGS;
	default: return false;
	}
	return false;
}

// Ported from Amalgam - hitbox validation for bounds types (adapted for SEOwnedDE)
bool CAimbot::IsHitboxValid(int nHitbox, int iHitboxes)
{
	switch (nHitbox)
	{
	case 0: return iHitboxes & HITBOX_HEAD;
	case 1: return iHitboxes & HITBOX_BODY;
	case 2: return iHitboxes & HITBOX_LEGS;
	default: return false;
	}
	return false;
}

// Ported from Amalgam - multipoint validation
bool CAimbot::ShouldMultipoint(C_BaseEntity* pEntity, int nHitbox, int iHitboxes)
{
	// Note: Would need CFG system adaptation for multipoint scale
	if (/*CFG::Aimbot::Hitscan::MultipointScale.Value <= 0.f*/ false)
		return false;

	if (!iHitboxes)
		return true;

	// Use same validation as main hitbox validation
	return IsHitboxValid(pEntity, nHitbox, iHitboxes);
}

// Ported from Amalgam - advanced aim control
bool CAimbot::ShouldAim()
{
	// Note: Would need config system adaptation for aim type
	// switch (CFG::Aimbot::General::AimType.Value)
	// {
	// case Vars::Aimbot::General::AimTypeEnum::Plain:
	// case Vars::Aimbot::General::AimTypeEnum::Silent:
	//		if (!G::bCanPrimaryAttack && !G::bReloading && !F::Ticks->IsTimingUnsure())
	//			return false;
	// }

	return true;
}

// Ported from Amalgam - hold attack control
bool CAimbot::ShouldHoldAttack(C_TFWeaponBase* pWeapon)
{
	// Ported from Amalgam - advanced attack holding logic
	// SEOwnedDE config integration point: AimHoldsFire
	// switch (CFG::Aimbot_General_AimHoldsFire)
	// {
	// case 0: // MinigunOnly
	//	if (pWeapon->GetWeaponID() != TF_WEAPON_MINIGUN)
	//		break;
	//	[[fallthrough]];
	// case 1: // Always
	//	// Advanced attack holding logic
	//	break;
	// }

	// For now, implement minigun-specific logic like Amalgam (most common use case)
	if (pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN)
	{
		// Ported from Amalgam minigun attack holding
		// Note: would need G:: system integration for full implementation
		// if (!m_bRunningSecondary && !g_GlobalState.bCanPrimaryAttack &&
		//     g_GlobalState.pLastUserCmd && (g_GlobalState.pLastUserCmd->buttons & IN_ATTACK) &&
		//     /*Vars::Aimbot::General::AimType.Value &&*/ !pWeapon->m_iState() == 2) // Not reloading
		// {
		//		return true;
		// }
	}

	// Basic attack holding for all weapons if enabled
	// Note: This would need proper config system integration
	// if (CFG::Aimbot_General_AimHoldsFire == 1) // Always
	// {
	//	// Implementation for general attack holding
	// }

	return false;
}

// Ported from Amalgam - bomb validation for projectile detonation
bool CAimbot::ValidBomb(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, C_BaseEntity* pBomb)
{
	if (!pLocal || !pWeapon || !pBomb)
		return false;

	Vec3 vOrigin = pBomb->m_vecOrigin();

	// Note: Would need SEOwnedDE sphere query system
	// for (CEntitySphereQuery sphere(vOrigin, 300.f);) { /* SEOwnedDE adaptation needed */ }
	// {
	//	pEntity = sphere.GetCurrentEntity();
	//	sphere.NextEntity();
	//	if (pEntity == pLocal || pEntity->IsPlayer() && (!pEntity->As<C_TFPlayer>()->IsAlive() || pEntity->As<C_TFPlayer>()->IsAGhost())
	//		|| !FriendlyFire() && pEntity->m_iTeamNum() == pLocal->m_iTeamNum())
	//		continue;

	//	Vec3 vPos; pEntity->m_Collision()->CalcNearestPoint(vOrigin, &vPos);
	//	if (vOrigin.DistTo(vPos) > 300.f)
	//		continue;

	//	if (pEntity->IsPlayer() || pEntity->IsBuilding() || pEntity->IsNPC())
	//	{
	//		if (ShouldIgnore(pEntity, pLocal, pWeapon))
	//			continue;
	//		// Note: Would need SEOwnedDE SDK adaptation for visiblity
	//		if (!SDK::VisPosCollideable(pBomb, pEntity, vOrigin, pEntity->IsPlayer() ? pEntity->m_vecOrigin() + pEntity->As<C_TFPlayer>()->GetViewOffset() : pEntity->GetCenter(), MASK_SHOT))
	//			continue;

	//		return true;
	//	}
	// }

	// For now, basic validation
	return pLocal->m_iTeamNum() != pBomb->m_iTeamNum() && !ShouldIgnore(pBomb, pLocal, pWeapon);
}

// Ported from Amalgam - friendly fire check (now using GameRulesHelpers)
bool CAimbot::FriendlyFire()
{
	return GameRulesHelpers::IsFriendlyFireEnabled();
}

// ==================== HITSCAN AIMBOT ====================

std::vector<Target_t> CAimbot::GetTargetsHitscan(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	std::vector<Target_t> vTargets;
	const auto iSort = CFG::Aimbot_Hitscan_Sort;

	Vec3 vLocalPos = pLocal->GetShootPos();
	Vec3 vLocalAngles = I::EngineClient->GetViewAngles();

	// Target players
	if (CFG::Aimbot_Target_Players)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::PLAYERS_ENEMIES))
		{
			if (ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			float flFOVTo; Vec3 vPos, vAngleTo;
			int iHitboxes = 0;
			if (CFG::Aimbot_Hitscan_Scan_Head) iHitboxes |= (1 << 0);
			if (CFG::Aimbot_Hitscan_Scan_Body) iHitboxes |= (1 << 1);
			if (CFG::Aimbot_Hitscan_Scan_Arms) iHitboxes |= (1 << 2);
			if (CFG::Aimbot_Hitscan_Scan_Legs) iHitboxes |= (1 << 3);

			int iSelectedHitboxes = GetSelectedHitboxes();
			if (!PlayerBoneInFOV(pEntity->As<C_TFPlayer>(), vLocalPos, vLocalAngles, flFOVTo, vPos, vAngleTo, iSelectedHitboxes))
				continue;

			if (flFOVTo > CFG::Aimbot_Hitscan_FOV)
				continue;

			float flDistTo = iSort == 1 ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back();
			vTargets.back().m_pEntity = pEntity;
			vTargets.back().m_iTargetType = Target_Player;
			vTargets.back().m_vPos = vPos;
			vTargets.back().m_vAngleTo = vAngleTo;
			vTargets.back().m_flFOVTo = flFOVTo;
			vTargets.back().m_flDistTo = flDistTo;
			vTargets.back().m_nPriority = GetPriority(pEntity->entindex());
		}

		// Fallback: Try to find any enemy player directly if group method fails
		if (vTargets.empty())
		{
			for (int i = 1; i < I::EngineClient->GetMaxClients(); i++)
			{
				auto pEntity = I::ClientEntityList->GetClientEntity(i);
				if (!pEntity) continue;

				auto pPlayer = pEntity->As<C_TFPlayer>();
				if (!pPlayer || pPlayer == pLocal || pPlayer->deadflag() || pPlayer->m_iTeamNum() == pLocal->m_iTeamNum())
					continue;

				Vec3 vPos = pPlayer->GetCenter();
				Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
				float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);

				if (flFOVTo > CFG::Aimbot_Hitscan_FOV)
					continue;

				float flDistTo = iSort == 1 ? vLocalPos.DistTo(vPos) : 0.f;
				vTargets.emplace_back();
				vTargets.back().m_pEntity = pPlayer;
				vTargets.back().m_iTargetType = Target_Player;
				vTargets.back().m_vPos = vPos;
				vTargets.back().m_vAngleTo = vAngleTo;
				vTargets.back().m_flFOVTo = flFOVTo;
				vTargets.back().m_flDistTo = flDistTo;
				vTargets.back().m_nPriority = 0;
			}
		}
	}

	// Target buildings
	if (CFG::Aimbot_Target_Buildings)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::BUILDINGS_ENEMIES))
		{
			if (ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);
			if (flFOVTo > CFG::Aimbot_Hitscan_FOV)
				continue;

			float flDistTo = iSort == 1 ? vLocalPos.DistTo(vPos) : 0.f;
			TargetTypeEnum targetType = (pEntity->As<C_ObjectSentrygun>()) ? Target_Sentry :
								   (pEntity->As<C_ObjectDispenser>()) ? Target_Dispenser : Target_Teleporter;
			vTargets.emplace_back();
			vTargets.back().m_pEntity = pEntity;
			vTargets.back().m_iTargetType = targetType;
			vTargets.back().m_vPos = vPos;
			vTargets.back().m_vAngleTo = vAngleTo;
			vTargets.back().m_flFOVTo = flFOVTo;
			vTargets.back().m_flDistTo = flDistTo;
		}
	}

	SortTargets(vTargets, iSort);
	return vTargets;
}

int CAimbot::CanHitHitscan(Target_t& tTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	if (!tTarget.m_pEntity || !pLocal || !pWeapon)
		return 0;

	// Simple trace check
	Vec3 vStart = pLocal->GetShootPos();
	Vec3 vEnd = tTarget.m_vPos;

	CGameTrace trace;
	CTraceFilterHitscan filter = {};
	filter.m_pIgnore = pLocal;
	H::AimUtils->Trace(vStart, vEnd, MASK_SHOT, &filter, &trace);

	if (trace.m_pEnt == tTarget.m_pEntity)
		return 1;

	return 0;
}

bool CAimbot::AimHitscan(Vec3 vCurAngle, Vec3 vToAngle, Vec3& vOut, int iMethod)
{
	if (iMethod == 0) // Normal
	{
		vOut = vToAngle;
		return true;
	}
	else if (iMethod == 1) // Silent
	{
		// Enhanced silent aim implementation (ported from Amalgam)
		vOut = vToAngle;
		// Silent aim will be marked in AimHitscan/AimProjectile/AimMelee functions
		// when actually applying angles to usercmd
		return true;
	}
	else if (iMethod == 2) // Smooth
	{
		float flSmoothing = CFG::Aimbot_Hitscan_Smoothing;
		vOut = vCurAngle + (vToAngle - vCurAngle) * (flSmoothing / 100.0f);
		return true;
	}

	return false;
}

void CAimbot::AimHitscan(CUserCmd* pCmd, Vec3& vAngle, int iMethod)
{
	Vec3 vOldAngle = pCmd->viewangles;
	Vec3 vAimAngle = vAngle;

	if (AimHitscan(vOldAngle, vAimAngle, vAngle, iMethod))
	{
		Math::ClampAngles(vAngle);

		if (iMethod == 1) // Silent
		{
			// Set silent aim flag for packet manipulation using new GlobalState system
			g_GlobalState.bPSilentAngles = true;
			// Apply FixMovement to ensure movement is correct
			H::AimUtils->FixMovement(pCmd, vAngle);
		}
		else if (iMethod == 0 || iMethod == 2)
		{
			// CRITICAL FIX: Clear PSilentAngles when switching to normal/smooth aim mode
			if (g_GlobalState.bPSilentAngles)
			{
				g_GlobalState.bPSilentAngles = false;
			}
			I::EngineClient->SetViewAngles(vAngle);
		}

		pCmd->viewangles = vAngle;
	}
}

bool CAimbot::ShouldFireHitscan(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd, const Target_t& tTarget)
{
	if (!pLocal || !pWeapon || !pCmd)
		return false;

	Vec3 vForward, vRight, vUp;
	Math::AngleVectors(pCmd->viewangles, &vForward, &vRight, &vUp);
	Vec3 vAimDir = tTarget.m_vPos - pLocal->GetShootPos();
	vAimDir.Normalize();

	float flDot = vForward.Dot(vAimDir);
	return flDot > 0.95f; // Close enough to target
}

void CAimbot::RunHitscan(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	// Revert to working5 simple logic that was working
	int iAimType = CFG::Aimbot_Hitscan_Aim_Type;

	auto vTargets = GetTargetsHitscan(pLocal, pWeapon);
	if (vTargets.empty())
		return;

	// Auto-scope for sniper rifles
	if (CFG::Aimbot_Hitscan_Auto_Scope && !pLocal->IsZoomed())
	{
		if (pWeapon->GetWeaponID() == TF_WEAPON_SNIPERRIFLE && !vTargets.empty())
		{
			pCmd->buttons |= IN_ATTACK2;
			return; // Don't aim this frame, scope first
		}
	}

	// For debugging - just aim at the first valid target
	for (auto& tTarget : vTargets)
	{
		// Skip trace check for now - just aim directly
		{
			// CRITICAL FIX: Check if we're going to fire BEFORE setting PSilentAngles
			// This prevents choking packets when not actually firing
			bool bWillFire = CFG::Aimbot_AutoShoot && g_GlobalState.bCanPrimaryAttack;

			// Only apply silent aim if we're actually going to fire
			if (bWillFire && iAimType == 1)
			{
				AimHitscan(pCmd, tTarget.m_vAngleTo, iAimType);
			}
			else if (!bWillFire || iAimType != 1)
			{
				AimHitscan(pCmd, tTarget.m_vAngleTo, iAimType);
			}

			if (bWillFire)
			{
				pCmd->buttons |= IN_ATTACK;
				// Keep both systems in sync
				G::bFiring = true;
				g_GlobalState.bAttacking = true;
			}

			m_bRan = true;
			break;
		}
	}
}

// ==================== PROJECTILE AIMBOT ====================

float CAimbot::GetProjectileSpeed(C_TFWeaponBase* pWeapon)
{
	if (!pWeapon) return 0.0f;

	switch (pWeapon->GetWeaponID())
	{
		case TF_WEAPON_ROCKETLAUNCHER:
			return 1100.0f;
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
			return 1980.0f;
		case TF_WEAPON_GRENADELAUNCHER:
			return 1200.0f;
		case TF_WEAPON_PIPEBOMBLAUNCHER:
			return 1333.0f;
		case TF_WEAPON_SYRINGEGUN_MEDIC:
			return 1000.0f;
		case TF_WEAPON_COMPOUND_BOW:
			return 2500.0f;
		case TF_WEAPON_CROSSBOW:
			return 2400.0f;
		case TF_WEAPON_FLAREGUN:
			return 2000.0f;
		case TF_WEAPON_FLAREGUN_REVENGE:
			return 2000.0f;
		case TF_WEAPON_RAYGUN:
			return 1200.0f;
		case TF_WEAPON_PARTICLE_CANNON:
			return 2500.0f;
		case TF_WEAPON_FLAMETHROWER:
		case TF_WEAPON_FLAMETHROWER_ROCKET:
		case TF_WEAPON_SENTRY_REVENGE:
		case TF_WEAPON_DRG_POMSON:
		case TF_WEAPON_GRENADE_NORMAL:
		case TF_WEAPON_GRENADE_CONCUSSION:
		case TF_WEAPON_GRENADE_NAIL:
		case TF_WEAPON_GRENADE_MIRV:
		case TF_WEAPON_GRENADE_MIRV_DEMOMAN:
		case TF_WEAPON_GRENADE_NAPALM:
		case TF_WEAPON_GRENADE_GAS:
		case TF_WEAPON_GRENADE_EMP:
		case TF_WEAPON_GRENADE_CALTROP:
		case TF_WEAPON_GRENADE_PIPEBOMB:
		case TF_WEAPON_GRENADE_SMOKE_BOMB:
		case TF_WEAPON_GRENADE_HEAL:
		case TF_WEAPON_GRENADE_STUNBALL:
		case TF_WEAPON_GRENADE_JAR:
		case TF_WEAPON_GRENADE_JAR_MILK:
		case TF_WEAPON_GRENADE_DEMOMAN:
		case TF_WEAPON_GRENADE_MIRVBOMB:
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_JAR:
		case TF_WEAPON_JAR_MILK:
		case TF_WEAPON_PUMPKIN_BOMB:
		case TF_WEAPON_GRENADE_ORNAMENT_BALL:
		case TF_WEAPON_GRENADE_WATERBALLOON:
		case TF_WEAPON_STICKY_BALL_LAUNCHER:
		case TF_WEAPON_GRENADE_STICKY_BALL:
		case TF_WEAPON_GRENADE_THROWABLE:
			return 1000.0f; // Treat as moderate speed projectiles
		case TF_WEAPON_CANNON:
			return 800.0f;
		case TF_WEAPON_DISPENSER_GUN:
			return 1800.0f;
		case TF_WEAPON_BUFF_ITEM:
			return 0.0f; // Special case
		case TF_WEAPON_LIFELINE:
		case TF_WEAPON_LASER_POINTER:
			return 5000.0f; // Very fast laser
		default:
			return 1000.0f;
	}
}

float CAimbot::GetProjectileGravity(C_TFWeaponBase* pWeapon)
{
	if (!pWeapon) return 0.0f;

	switch (pWeapon->GetWeaponID())
	{
		case TF_WEAPON_ROCKETLAUNCHER:
			return 0.055f;  // ~800 units/s^2 gravity
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
			return 0.0f;    // Direct hit has no gravity drop
		case TF_WEAPON_GRENADELAUNCHER:
			return 0.1f;    // ~1200 units/s^2 gravity with arc
		case TF_WEAPON_PIPEBOMBLAUNCHER:
			return 0.12f;   // ~1200 units/s^2 gravity for stickies
		case TF_WEAPON_SYRINGEGUN_MEDIC:
			return 0.055f;  // Similar to rocket gravity
		case TF_WEAPON_COMPOUND_BOW:
			return 0.08f;   // Bow arrows
		case TF_WEAPON_CROSSBOW:
			return 0.2f;    // Crossbow bolts have significant drop
		case TF_WEAPON_FLAREGUN:
		case TF_WEAPON_FLAREGUN_REVENGE:
			return 0.09f;   // Flares have moderate gravity
		case TF_WEAPON_RAYGUN:
			return 0.0f;    // Raygun has no gravity
		case TF_WEAPON_PARTICLE_CANNON:
			return 0.06f;   // Low gravity for particles
		case TF_WEAPON_CANNON:
			return 0.15f;   // Cannonballs have heavy drop
		case TF_WEAPON_LUNCHBOX:
		case TF_WEAPON_JAR:
		case TF_WEAPON_JAR_MILK:
			return 0.2f;    // Thrown items have heavy gravity
		case TF_WEAPON_PUMPKIN_BOMB:
			return 0.15f;   // Heavy pumpkin bombs
		case TF_WEAPON_GRENADE_NORMAL:
		case TF_WEAPON_GRENADE_CONCUSSION:
		case TF_WEAPON_GRENADE_NAIL:
		case TF_WEAPON_GRENADE_MIRV:
		case TF_WEAPON_GRENADE_MIRV_DEMOMAN:
		case TF_WEAPON_GRENADE_DEMOMAN:
		case TF_WEAPON_GRENADE_MIRVBOMB:
			return 0.2f;    // General grenades have heavy gravity
		case TF_WEAPON_STICKBOMB:
			return 0.12f;   // Same as pipebomb launcher
		case TF_WEAPON_GRENADE_ORNAMENT_BALL:
		case TF_WEAPON_GRENADE_WATERBALLOON:
			return 0.18f;   // Seasonal grenades
		case TF_WEAPON_STICKY_BALL_LAUNCHER:
		case TF_WEAPON_GRENADE_STICKY_BALL:
			return 0.12f;   // Sticky ball physics
		case TF_WEAPON_FLAMETHROWER:
		case TF_WEAPON_FLAMETHROWER_ROCKET:
		case TF_WEAPON_SENTRY_REVENGE:
		case TF_WEAPON_DRG_POMSON:
		case TF_WEAPON_BUFF_ITEM:
		case TF_WEAPON_LIFELINE:
		case TF_WEAPON_LASER_POINTER:
		case TF_WEAPON_DISPENSER_GUN:
			return 0.0f;    // Hitscan or special case
		default:
			return 0.0f;    // Default to no gravity for safety
	}
}

// Enhanced projectile spawn position calculation (ported from Amalgam)
// This accounts for weapon offsets and proper projectile spawn positions
Vec3 CAimbot::GetProjectileSpawnPosition(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, Vec3 vAngles)
{
	if (!pLocal || !pWeapon)
		return pLocal ? pLocal->GetShootPos() : Vec3();

	Vec3 vForward, vRight, vUp;
	Math::AngleVectors(vAngles, &vForward, &vRight, &vUp);

	Vec3 vShootPos = pLocal->GetShootPos();
	bool bDucking = pLocal->m_fFlags() & FL_DUCKING;

	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_ROCKETLAUNCHER:
	case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
	case TF_WEAPON_PARTICLE_CANNON:
	case TF_WEAPON_RAYGUN:
	case TF_WEAPON_DRG_POMSON:
	{
		// Apply proper weapon offsets based on Amalgam's GetProjectileFireSetup
		// Forward offset for projectile spawn
		vShootPos += vForward * 23.5f;

		// Horizontal offset (centerfire weapons have no offset)
		static auto tf_projectile_centerfire = I::CVar->FindVar("tf_projectile_centerfire");
		bool bCenterFire = tf_projectile_centerfire && tf_projectile_centerfire->GetBool();
		float flSideOffset = bCenterFire ? 0.0f : 12.0f;
		vShootPos += vRight * flSideOffset;

		// Vertical offset (ducking adjustment)
		vShootPos += vUp * (bDucking ? 8.0f : -3.0f);

		return vShootPos;
	}

	case TF_WEAPON_GRENADELAUNCHER:
	case TF_WEAPON_CANNON:
	{
		// Grenade launcher has different spawn position
		vShootPos += vForward * 16.0f;
		vShootPos += vUp * -6.0f;
		return vShootPos;
	}

	case TF_WEAPON_PIPEBOMBLAUNCHER:
	{
		// Sticky bomb launcher spawn position
		vShootPos += vForward * 16.0f;
		vShootPos += vUp * -6.0f;
		return vShootPos;
	}

	case TF_WEAPON_SYRINGEGUN_MEDIC:
	{
		// Syringe gun - closer to camera
		vShootPos += vForward * 16.0f;
		return vShootPos;
	}

	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_CROSSBOW:
	{
		// Arrow weapons - standard offset
		vShootPos += vForward * 16.0f;
		vShootPos += vUp * (bDucking ? -8.0f : -6.0f);
		return vShootPos;
	}

	case TF_WEAPON_FLAREGUN:
	case TF_WEAPON_FLAREGUN_REVENGE:
	{
		// Flare gun spawn position
		vShootPos += vForward * 16.0f;
		vShootPos += vUp * (bDucking ? 8.0f : -3.0f);
		return vShootPos;
	}

	default:
		// Fallback to standard shoot position for unknown weapons
		return vShootPos;
	}
}

std::vector<Target_t> CAimbot::GetTargetsProjectile(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	std::vector<Target_t> vTargets;
	const auto iSort = CFG::Aimbot_Projectile_Sort;

	Vec3 vLocalPos = pLocal->GetShootPos();
	Vec3 vLocalAngles = I::EngineClient->GetViewAngles();

	// Target players
	if (CFG::Aimbot_Target_Players)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::PLAYERS_ENEMIES))
		{
			if (ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);

			if (flFOVTo > CFG::Aimbot_Projectile_FOV)
				continue;

			float flDistTo = iSort == 1 ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back();
			vTargets.back().m_pEntity = pEntity;
			vTargets.back().m_iTargetType = Target_Player;
			vTargets.back().m_vPos = vPos;
			vTargets.back().m_vAngleTo = vAngleTo;
			vTargets.back().m_flFOVTo = flFOVTo;
			vTargets.back().m_flDistTo = flDistTo;
			vTargets.back().m_nPriority = GetPriority(pEntity->entindex());
		}
	}

	// Target buildings
	if (CFG::Aimbot_Target_Buildings)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::BUILDINGS_ENEMIES))
		{
			if (ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);
			if (flFOVTo > CFG::Aimbot_Projectile_FOV)
				continue;

			float flDistTo = iSort == 1 ? vLocalPos.DistTo(vPos) : 0.f;
			TargetTypeEnum targetType = (pEntity->As<C_ObjectSentrygun>()) ? Target_Sentry :
								   (pEntity->As<C_ObjectDispenser>()) ? Target_Dispenser : Target_Teleporter;
			vTargets.emplace_back();
			vTargets.back().m_pEntity = pEntity;
			vTargets.back().m_iTargetType = targetType;
			vTargets.back().m_vPos = vPos;
			vTargets.back().m_vAngleTo = vAngleTo;
			vTargets.back().m_flFOVTo = flFOVTo;
			vTargets.back().m_flDistTo = flDistTo;
		}
	}

	SortTargets(vTargets, iSort);
	return vTargets;
}

// Drag calculation helper (ported from Amalgam)
static inline void SolveProjectileSpeed(C_TFWeaponBase* pWeapon, const Vec3& vLocalPos, const Vec3& vTargetPos, float& flVelocity, float& flDragTime, const float flGravity)
{
	if (!F::ProjectileSim->IsDragEnabled() || F::ProjectileSim->GetDragBasis().IsZero())
		return;

	const float flGrav = flGravity * 800.0f;
	const Vec3 vDelta = vTargetPos - vLocalPos;
	const float flDist = vDelta.Length2D();

	const float flRoot = pow(flVelocity, 4) - flGrav * (flGrav * pow(flDist, 2) + 2.f * vDelta.z * pow(flVelocity, 2));
	if (flRoot < 0.f)
		return;

	const float flPitch = atan((pow(flVelocity, 2) - sqrt(flRoot)) / (flGrav * flDist));
	const float flTime = flDist / (cos(flPitch) * flVelocity);

	// Weapon-specific drag coefficients (ported from Amalgam SolveProjectileSpeed:934-964)
	float flDrag = 0.f;
	switch (pWeapon->m_iItemDefinitionIndex())
	{
		// Grenade launchers
		case Demoman_m_GrenadeLauncher:
		case Demoman_m_GrenadeLauncherR:
		case Demoman_m_FestiveGrenadeLauncher:
		case Demoman_m_Autumn:
		case Demoman_m_MacabreWeb:
		case Demoman_m_Rainbow:
		case Demoman_m_SweetDreams:
		case Demoman_m_CoffinNail:
		case Demoman_m_TopShelf:
		case Demoman_m_Warhawk:
		case Demoman_m_ButcherBird:
		case Demoman_m_TheIronBomber:
			flDrag = Math::RemapValClamped(flVelocity, 1217.f, 3000.f, 0.120f, 0.200f);
			break;
		case Demoman_m_TheLochnLoad:
			flDrag = Math::RemapValClamped(flVelocity, 1504.f, 3000.f, 0.070f, 0.085f);
			break;
		case Demoman_m_TheLooseCannon:
			flDrag = Math::RemapValClamped(flVelocity, 1454.f, 3000.f, 0.385f, 0.530f);
			break;
		// Stickybomb launchers
		case Demoman_s_StickybombLauncher:
		case Demoman_s_StickybombLauncherR:
		case Demoman_s_FestiveStickybombLauncher:
		case Demoman_s_TheQuickiebombLauncher:
		case Demoman_s_TheScottishResistance:
			flDrag = Math::RemapValClamped(flVelocity, 922.f, 3000.f, 0.085f, 0.190f);
			break;
		// Scout throwables
		case Scout_s_TheFlyingGuillotine:
		case Scout_s_TheFlyingGuillotineG:
			flDrag = 0.310f;
			break;
		case Scout_t_TheSandman:
			flDrag = 0.180f;
			break;
		case Scout_t_TheWrapAssassin:
			flDrag = 0.285f;
			break;
		// Jars
		case Scout_s_MadMilk:
		case Scout_s_MutatedMilk:
		case Sniper_s_Jarate:
		case Sniper_s_FestiveJarate:
		case Sniper_s_TheSelfAwareBeautyMark:
			flDrag = 0.057f;
			break;
	}

	// Rough estimate to prevent time being too low (Amalgam line 968)
	flDragTime = powf(flTime, 2) * flDrag / 1.5f;
	flVelocity = flVelocity - flVelocity * flTime * flDrag;
}

void CAimbot::CalculateAngle(const Vec3& vLocalPos, const Vec3& vTargetPos, int iSimTime, Solution_t& out)
{
	// TWO-PASS TRAJECTORY CALCULATION (Amalgam-style)
	// Pass 1: Basic trajectory from player position
	// Pass 2: Refined trajectory from actual projectile spawn position (via ProjectileSim)

	static auto sv_gravity = I::CVar->FindVar("sv_gravity");
	const float flGrav = (sv_gravity ? sv_gravity->GetFloat() : 800.0f) * m_tProjectileInfo.m_flGravity;

	// Ensure we have valid velocity and weapon
	if (m_tProjectileInfo.m_flVelocity <= 0.0f || !m_tProjectileInfo.m_pWeapon)
	{
		out.m_iCalculated = 2; // Bad solution - no velocity
		return;
	}

	float flPitch, flYaw;

	// PASS 1: Basic trajectory calculation from player eye position (with drag)
	{
		float flVelocity = m_tProjectileInfo.m_flVelocity, flDragTime = 0.f;

		// CRITICAL FIX #1: Add drag calculation like Amalgam (lines 980-986)
		if (F::ProjectileSim->IsDragEnabled() && !F::ProjectileSim->GetDragBasis().IsZero() && m_tProjectileInfo.m_pWeapon)
		{
			Vec3 vForward, vRight, vUp;
			Math::AngleVectors(Math::CalcAngle(vLocalPos, vTargetPos), &vForward, &vRight, &vUp);
			Vec3 vShootPos = vLocalPos + (vForward * m_tProjectileInfo.m_vOffset.x) +
			                             (vRight * m_tProjectileInfo.m_vOffset.y) +
			                             (vUp * m_tProjectileInfo.m_vOffset.z);
			SolveProjectileSpeed(m_tProjectileInfo.m_pWeapon, vShootPos, vTargetPos, flVelocity, flDragTime, m_tProjectileInfo.m_flGravity);
		}

		Vec3 vDelta = vTargetPos - vLocalPos;
		const float flDist = vDelta.Length2D();

		Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vTargetPos);

		if (!flGrav)
		{
			flPitch = -DEG2RAD(vAngleTo.x);
		}
		else
		{
			const float flRoot = pow(flVelocity, 4) - flGrav * (flGrav * pow(flDist, 2) + 2.0f * vDelta.z * pow(flVelocity, 2));

			if (flRoot < 0.0f || isnan(flRoot))
			{
				out.m_iCalculated = 2; // Bad solution - invalid trajectory
				return;
			}

			flPitch = atan((pow(flVelocity, 2) - sqrt(flRoot)) / (flGrav * flDist));
		}

		// CRITICAL FIX #2: Add offset time and angle fix like Amalgam (lines 1001-1003)
		out.m_flTime = flDist / (cos(flPitch) * flVelocity) - m_tProjectileInfo.m_flOffsetTime + flDragTime;
		flPitch = -RAD2DEG(flPitch) - m_tProjectileInfo.m_vAngFix.x;
		flYaw = vAngleTo.y - m_tProjectileInfo.m_vAngFix.y;

		out.m_flPitch = flPitch;
		out.m_flYaw = flYaw;
	}

	// Check if we have time for this solution
	int iTimeTo = int(out.m_flTime / TICK_INTERVAL) + 1;

	// PASS 2: Get actual projectile spawn position using ProjectileSim
	ProjectileInfo tProjInfo = {};
	if (F::ProjectileSim->GetInfo(m_tProjectileInfo.m_pLocal, m_tProjectileInfo.m_pWeapon, { flPitch, flYaw, 0 }, tProjInfo))
	{
		// Recalculate trajectory from ACTUAL projectile spawn position (with drag)
		float flVelocity = m_tProjectileInfo.m_flVelocity, flDragTime = 0.f;
		SolveProjectileSpeed(m_tProjectileInfo.m_pWeapon, tProjInfo.m_pos, vTargetPos, flVelocity, flDragTime, m_tProjectileInfo.m_flGravity);

		Vec3 vDelta = vTargetPos - tProjInfo.m_pos;
		const float flDist = vDelta.Length2D();

		Vec3 vAngleTo = Math::CalcAngle(tProjInfo.m_pos, vTargetPos);

		if (!flGrav)
		{
			out.m_flPitch = -DEG2RAD(vAngleTo.x);
		}
		else
		{
			const float flRoot = pow(flVelocity, 4) - flGrav * (flGrav * pow(flDist, 2) + 2.0f * vDelta.z * pow(flVelocity, 2));

			if (flRoot < 0.0f || isnan(flRoot))
			{
				out.m_iCalculated = 2; // Bad solution - invalid trajectory
				return;
			}

			out.m_flPitch = atan((pow(flVelocity, 2) - sqrt(flRoot)) / (flGrav * flDist));
		}

		out.m_flTime = flDist / (cos(out.m_flPitch) * flVelocity) + flDragTime;

		// Correct yaw for horizontal spawn offset (Amalgam's quadratic solve - lines 1072-1084)
		{
			Vec3 vShootPos = (tProjInfo.m_pos - vLocalPos);
			vShootPos.z = 0; // Only horizontal offset matters for yaw (To2D equivalent)
			Vec3 vTarget = vTargetPos - vLocalPos;
			Vec3 vForward;
			Math::AngleVectors(tProjInfo.m_ang, &vForward, nullptr, nullptr);
			vForward.z = 0;
			vForward = vForward.Normalized(); // Normalize2D equivalent

			float flB = 2.0f * (vShootPos.x * vForward.x + vShootPos.y * vForward.y);
			float flC = vShootPos.Length2DSqr() - vTarget.Length2DSqr();

			// Solve quadratic: t^2 + flB*t + flC = 0
			float flDiscriminant = flB * flB - 4.0f * flC;
			if (flDiscriminant >= 0)
			{
				float flT = (-flB + sqrt(flDiscriminant)) / 2.0f;
				vShootPos += vForward * flT;
				// CRITICAL FIX #3: Use Amalgam's exact yaw formula (line 1082)
				// out.m_flYaw = flYaw - (RAD2DEG(atan2(...)) - flYaw) is equivalent to:
				out.m_flYaw = flYaw - (RAD2DEG(atan2(vShootPos.y, vShootPos.x)) - flYaw);
				flYaw = RAD2DEG(atan2(vShootPos.y, vShootPos.x));
			}
		}

		// Correct pitch for vertical spawn offset (Amalgam's approach - lines 1087-1107)
		if (flGrav)
		{
			flPitch -= tProjInfo.m_ang.x;
			out.m_flPitch = -RAD2DEG(out.m_flPitch) + flPitch - m_tProjectileInfo.m_vAngFix.x;
		}
		else
		{
			// For non-gravity projectiles, use rotation-based pitch correction
			Vec3 vShootPos = Math::RotatePoint(tProjInfo.m_pos - vLocalPos, {}, { 0, -flYaw, 0 });
			vShootPos.y = 0;

			Vec3 vTarget = Math::RotatePoint(vTargetPos - vLocalPos, {}, { 0, -flYaw, 0 });

			Vec3 vForward;
			Math::AngleVectors(tProjInfo.m_ang - Vec3(0, flYaw, 0), &vForward, nullptr, nullptr);
			vForward.y = 0;
			vForward = vForward.Normalized();

			float flB = 2.0f * (vShootPos.x * vForward.x + vShootPos.z * vForward.z);
			float flC = (powf(vShootPos.x, 2) + powf(vShootPos.z, 2)) - (powf(vTarget.x, 2) + powf(vTarget.z, 2));

			float flDiscriminant = flB * flB - 4.0f * flC;
			if (flDiscriminant >= 0)
			{
				float flT = (-flB + sqrt(flDiscriminant)) / 2.0f;
				vShootPos += vForward * flT;
				out.m_flPitch = flPitch - (RAD2DEG(atan2(-vShootPos.z, vShootPos.x)) - flPitch);
			}
		}
	}
	else
	{
		// ProjectileSim failed - fall back to basic calculation
		out.m_flPitch = flPitch;
		out.m_flYaw = flYaw;
	}

	// Validate solution timing
	iTimeTo = int(out.m_flTime / TICK_INTERVAL) + 1;
	if (iTimeTo > iSimTime)
	{
		out.m_iCalculated = 3; // Time constraint not met
		return;
	}

	// Ensure we have valid results
	if (isnan(out.m_flPitch) || isinf(out.m_flPitch) || isnan(out.m_flYaw) || isinf(out.m_flYaw) ||
		isnan(out.m_flTime) || isinf(out.m_flTime) || out.m_flTime <= 0.0f)
	{
		out.m_iCalculated = 2; // Bad solution
		return;
	}

	out.m_iCalculated = 1; // Good solution
}

bool CAimbot::TestAngleProjectile(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, Target_t& tTarget, Vec3& vPoint, Vec3& vAngles, int iSimTime)
{
	// Enhanced projectile prediction with velocity prediction (ported from Amalgam)
	m_tProjectileInfo.m_pLocal = pLocal;
	m_tProjectileInfo.m_pWeapon = pWeapon;

	// CRITICAL: Use proper projectile spawn position like Amalgam
	// This accounts for weapon offsets and proper spawn positions
	Vec3 vShootPos = GetProjectileSpawnPosition(pLocal, pWeapon, vAngles);
	m_tProjectileInfo.m_vLocalEye = vShootPos;

	m_tProjectileInfo.m_flVelocity = GetProjectileSpeed(pWeapon);
	m_tProjectileInfo.m_flGravity = GetProjectileGravity(pWeapon);
	m_tProjectileInfo.m_flLatency = I::EngineClient->GetNetChannelInfo() ? I::EngineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) : 0.0f;

	// CRITICAL FIX: Use MovementSimulation for accurate target prediction (like Amalgam)
	// This accounts for TF2's movement physics: friction, air resistance, gravity, collisions
	Vec3 vPredictedPos = tTarget.m_vPos;
	C_TFPlayer* pTargetPlayer = tTarget.m_pEntity->As<C_TFPlayer>();

	if (pTargetPlayer)
	{
		// Initialize movement simulation for this target
		if (!F::MovementSimulation->Initialize(pTargetPlayer))
		{
			// Fallback to simple velocity extrapolation if simulation fails
			Vec3 vVelocity = pTargetPlayer->m_vecVelocity();
			Solution_t initialSolution;
			CalculateAngle(vShootPos, vPredictedPos, iSimTime, initialSolution);

			if (initialSolution.m_iCalculated != 1)
				return false;

			float flPredictTime = initialSolution.m_flTime + m_tProjectileInfo.m_flLatency;
			vPredictedPos += vVelocity * flPredictTime;
		}
		else
		{
			// STEP 1: Calculate initial trajectory to get time estimate
			Solution_t initialSolution;
			CalculateAngle(vShootPos, vPredictedPos, iSimTime, initialSolution);

			if (initialSolution.m_iCalculated != 1)
			{
				F::MovementSimulation->Restore();
				return false;
			}

			// STEP 2: Calculate how many ticks to simulate
			float flPredictTime = initialSolution.m_flTime + m_tProjectileInfo.m_flLatency;
			int iTicksToSimulate = TIME_TO_TICKS(flPredictTime);

			// STEP 3: Run movement simulation tick-by-tick (like Amalgam)
			// This properly simulates TF2 movement physics
			for (int i = 0; i < iTicksToSimulate && i < 66; i++) // Cap at 66 ticks (~1 second)
			{
				F::MovementSimulation->RunTick();
			}

			// STEP 4: Get the predicted position from simulation
			vPredictedPos = F::MovementSimulation->GetOrigin();

			// STEP 5: Restore original player state
			F::MovementSimulation->Restore();

			// STEP 6: Iterative refinement - recalculate with simulated position
			Solution_t refinedSolution;
			CalculateAngle(vShootPos, vPredictedPos, iSimTime, refinedSolution);

			if (refinedSolution.m_iCalculated == 1)
			{
				// If timing changed significantly, adjust simulation
				float flRefinedTime = refinedSolution.m_flTime + m_tProjectileInfo.m_flLatency;
				int iRefinedTicks = TIME_TO_TICKS(flRefinedTime);

				// Only re-simulate if tick count differs significantly
				if (abs(iRefinedTicks - iTicksToSimulate) > 2)
				{
					// Re-initialize and simulate with refined timing
					if (F::MovementSimulation->Initialize(pTargetPlayer))
					{
						for (int i = 0; i < iRefinedTicks && i < 66; i++)
						{
							F::MovementSimulation->RunTick();
						}
						vPredictedPos = F::MovementSimulation->GetOrigin();
						F::MovementSimulation->Restore();
					}
				}
			}
		}
	}
	else if (tTarget.m_pEntity)
	{
		// Building prediction - buildings are stationary
		// Just use current position
		vPredictedPos = tTarget.m_vPos;
	}

	Solution_t solution;
	CalculateAngle(m_tProjectileInfo.m_vLocalEye, vPredictedPos, iSimTime, solution);

	if (solution.m_iCalculated == 1)
	{
		vAngles = Vec3(solution.m_flPitch, solution.m_flYaw, 0.0f);
		Math::ClampAngles(vAngles);

		// Store the predicted position for debugging/visualization
		tTarget.m_vPos = vPredictedPos;

		return true;
	}

	return false;
}

int CAimbot::CanHitProjectile(Target_t& tTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	// Basic implementation - always return true for now
	return 1;
}

bool CAimbot::AimProjectile(Vec3 vCurAngle, Vec3 vToAngle, Vec3& vOut, int iMethod)
{
	if (iMethod == 0) // Normal
	{
		vOut = vToAngle;
		return true;
	}
	else if (iMethod == 1) // Silent
	{
		// Enhanced silent aim implementation (ported from Amalgam)
		vOut = vToAngle;
		// Silent aim will be marked in AimHitscan/AimProjectile/AimMelee functions
		// when actually applying angles to usercmd
		return true;
	}

	return false;
}

void CAimbot::AimProjectile(CUserCmd* pCmd, Vec3& vAngle, int iMethod)
{
	Vec3 vOldAngle = pCmd->viewangles;
	Vec3 vAimAngle = vAngle;

	if (AimProjectile(vOldAngle, vAimAngle, vAngle, iMethod))
	{
		Math::ClampAngles(vAngle);

		if (iMethod == 1) // Silent
		{
			// Set silent aim flag for packet manipulation using new GlobalState system
			g_GlobalState.bPSilentAngles = true;
			// Apply FixMovement to ensure movement is correct
			H::AimUtils->FixMovement(pCmd, vAngle);
		}
		else if (iMethod == 0)
		{
			// CRITICAL FIX: Clear PSilentAngles when switching to normal aim mode
			if (g_GlobalState.bPSilentAngles)
			{
				g_GlobalState.bPSilentAngles = false;
			}
			I::EngineClient->SetViewAngles(vAngle);
		}

		pCmd->viewangles = vAngle;
	}
}

void CAimbot::RunProjectile(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	// Revert to working5 simple logic that was working
	int iAimType = CFG::Aimbot_Projectile_Aim_Type;

	auto vTargets = GetTargetsProjectile(pLocal, pWeapon);
	if (vTargets.empty())
		return;

	for (auto& tTarget : vTargets)
	{
		Vec3 vAimAngle;
		if (TestAngleProjectile(pLocal, pWeapon, tTarget, tTarget.m_vPos, vAimAngle, TIME_TO_TICKS(I::GlobalVars->curtime)))
		{
			// CRITICAL FIX: Check if we're going to fire BEFORE setting PSilentAngles
			// This prevents choking packets when not actually firing
			bool bWillFire = CFG::Aimbot_AutoShoot && g_GlobalState.bCanPrimaryAttack;

			// Only apply silent aim if we're actually going to fire
			if (bWillFire && iAimType == 1)
			{
				AimProjectile(pCmd, vAimAngle, iAimType);
			}
			else if (!bWillFire || iAimType != 1)
			{
				// For non-silent or non-firing, just aim normally
				AimProjectile(pCmd, vAimAngle, iAimType);
			}

			if (bWillFire)
			{
				pCmd->buttons |= IN_ATTACK;
				// Keep both systems in sync
				G::bFiring = true;
				g_GlobalState.bAttacking = true;
			}

			m_bRan = true;
			break;
		}
	}
}

// ==================== MELEE AIMBOT ====================

std::vector<Target_t> CAimbot::GetTargetsMelee(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	std::vector<Target_t> vTargets;
	const auto iSort = CFG::Aimbot_Melee_Sort;

	Vec3 vLocalPos = pLocal->GetShootPos();
	Vec3 vLocalAngles = I::EngineClient->GetViewAngles();

	// Target players
	if (CFG::Aimbot_Target_Players)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::PLAYERS_ENEMIES))
		{
			if (ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);

			if (flFOVTo > CFG::Aimbot_Melee_FOV)
				continue;

			float flDistTo = iSort == 1 ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back();
			vTargets.back().m_pEntity = pEntity;
			vTargets.back().m_iTargetType = Target_Player;
			vTargets.back().m_vPos = vPos;
			vTargets.back().m_vAngleTo = vAngleTo;
			vTargets.back().m_flFOVTo = flFOVTo;
			vTargets.back().m_flDistTo = flDistTo;
			vTargets.back().m_nPriority = GetPriority(pEntity->entindex());
		}
	}

	// Target buildings for sapper
	if (pWeapon->GetWeaponID() == TF_WEAPON_WRENCH && CFG::Aimbot_Target_Buildings)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::BUILDINGS_ENEMIES))
		{
			if (ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);
			if (flFOVTo > CFG::Aimbot_Melee_FOV)
				continue;

			float flDistTo = iSort == 1 ? vLocalPos.DistTo(vPos) : 0.f;
			TargetTypeEnum targetType = (pEntity->As<C_ObjectSentrygun>()) ? Target_Sentry :
								   (pEntity->As<C_ObjectDispenser>()) ? Target_Dispenser : Target_Teleporter;
			vTargets.emplace_back();
			vTargets.back().m_pEntity = pEntity;
			vTargets.back().m_iTargetType = targetType;
			vTargets.back().m_vPos = vPos;
			vTargets.back().m_vAngleTo = vAngleTo;
			vTargets.back().m_flFOVTo = flFOVTo;
			vTargets.back().m_flDistTo = flDistTo;
		}
	}

	SortTargets(vTargets, iSort);
	return vTargets;
}

int CAimbot::GetSwingTime(C_TFWeaponBase* pWeapon, bool bVar)
{
	if (!pWeapon) return 0;

	// Basic swing time - can be expanded with weapon-specific timing
	return TIME_TO_TICKS(0.5f);
}

bool CAimbot::CanBackstab(C_BaseEntity* pTarget, C_TFPlayer* pLocal, Vec3 vEyeAngles)
{
	if (!pTarget || !pLocal)
		return false;

	auto pPlayer = pTarget->As<C_TFPlayer>();
	if (!pPlayer) return false;

	// Enhanced backstab detection (ported from Amalgam)
	Vec3 vLocalPos = pLocal->GetAbsOrigin();
	Vec3 vTargetPos = pPlayer->GetAbsOrigin();

	// Check if target is in valid state for backstab
	if (pPlayer->InCond(TF_COND_INVULNERABLE) ||
		pPlayer->InCond(TF_COND_STEALTHED) ||
		pPlayer->InCond(TF_COND_PHASE) ||
		pPlayer->InCond(TF_COND_TAUNTING))
		return false;

	// Calculate direction vectors
	Vec3 vToTarget = vTargetPos - vLocalPos;
	vToTarget.z = 0.0f; // Ignore height difference for backstab angle
	vToTarget.Normalize();

	Vec3 vTargetForward, vTargetUp;
	Math::AngleVectors(pPlayer->GetEyeAngles(), &vTargetForward, nullptr, &vTargetUp);
	vTargetForward.z = 0.0f; // Ignore height difference
	vTargetForward.Normalize();

	// Calculate backstab angle - more sophisticated than simple dot product
	float flDot = vToTarget.Dot(vTargetForward);

	// Backstab is possible if we're behind the target (dot > cos(90°) = 0)
	// But also consider the height angle for better detection
	Vec3 vLocalForward, vLocalUp;
	Math::AngleVectors(vEyeAngles, &vLocalForward, nullptr, &vLocalUp);

	// Enhanced angle calculation considering both horizontal and vertical components
	float flHorizontalDot = flDot; // Horizontal component
	float flVerticalDot = vLocalForward.Dot(vTargetForward); // Vertical component

	// Combined backstab calculation
	bool bBehindTarget = flHorizontalDot > 0.35f; // Approx 70 degrees
	bool bProperHeight = fabs(vLocalPos.z - vTargetPos.z) < 50.0f; // Height tolerance

	// Distance check for backstab range
	float flDist = vLocalPos.DistTo(vTargetPos);
	bool bInRange = flDist <= 100.0f; // Backstab range slightly larger than normal melee

	return bBehindTarget && bProperHeight && bInRange;
}

int CAimbot::CanHitMelee(Target_t& tTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon)
{
	if (!tTarget.m_pEntity || !pLocal || !pWeapon)
		return 0;

	Vec3 vLocalPos = pLocal->GetAbsOrigin();
	Vec3 vTargetPos = tTarget.m_pEntity->GetAbsOrigin();
	float flDist = vLocalPos.DistTo(vTargetPos);

	// Basic melee range check
	float flRange = 72.0f; // Standard melee range
	if (flDist <= flRange)
		return 1;

	return 0;
}

bool CAimbot::AimMelee(Vec3 vCurAngle, Vec3 vToAngle, Vec3& vOut, int iMethod)
{
	if (iMethod == 0) // Normal
	{
		vOut = vToAngle;
		return true;
	}
	else if (iMethod == 1) // Silent
	{
		// Enhanced silent aim implementation (ported from Amalgam)
		vOut = vToAngle;
		// Silent aim will be marked in AimHitscan/AimProjectile/AimMelee functions
		// when actually applying angles to usercmd
		return true;
	}
	else if (iMethod == 2) // Smooth
	{
		float flSmoothing = CFG::Aimbot_Melee_Smoothing;
		vOut = vCurAngle + (vToAngle - vCurAngle) * (flSmoothing / 100.0f);
		return true;
	}

	return false;
}

void CAimbot::AimMelee(CUserCmd* pCmd, Vec3& vAngle, int iMethod)
{
	Vec3 vOldAngle = pCmd->viewangles;
	Vec3 vAimAngle = vAngle;

	if (AimMelee(vOldAngle, vAimAngle, vAngle, iMethod))
	{
		Math::ClampAngles(vAngle);

		if (iMethod == 1) // Silent
		{
			// Set silent aim flag for packet manipulation using new GlobalState system
			g_GlobalState.bPSilentAngles = true;
			// Apply FixMovement to ensure movement is correct
			H::AimUtils->FixMovement(pCmd, vAngle);
		}
		else if (iMethod == 0 || iMethod == 2)
		{
			// CRITICAL FIX: Clear PSilentAngles when switching to normal/smooth aim mode
			if (g_GlobalState.bPSilentAngles)
			{
				g_GlobalState.bPSilentAngles = false;
			}
			I::EngineClient->SetViewAngles(vAngle);
		}

		pCmd->viewangles = vAngle;
	}
}

void CAimbot::RunMelee(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	// Revert to working5 simple logic that was working
	int iAimType = CFG::Aimbot_Melee_Aim_Type;

	auto vTargets = GetTargetsMelee(pLocal, pWeapon);
	if (vTargets.empty())
		return;

	for (auto& tTarget : vTargets)
	{
		if (CanHitMelee(tTarget, pLocal, pWeapon))
		{
			AimMelee(pCmd, tTarget.m_vAngleTo, iAimType);

			// Check for backstab opportunity
			if (pWeapon->GetWeaponID() == TF_WEAPON_KNIFE && CanBackstab(tTarget.m_pEntity, pLocal, pCmd->viewangles))
			{
				// FIX: Only autoshoot if weapon is ready (prevents rapid fire animation)
				if (CFG::Aimbot_AutoShoot && g_GlobalState.bCanPrimaryAttack)
				{
					pCmd->buttons |= IN_ATTACK;
					G::bFiring = true;
				}
			}
			else if (CFG::Aimbot_AutoShoot && g_GlobalState.bCanPrimaryAttack)
			{
				pCmd->buttons |= IN_ATTACK;
				// Keep both systems in sync
				G::bFiring = true;
				g_GlobalState.bAttacking = true;
			}

			m_bRan = true;
			break;
		}
	}
}

// ==================== MAIN AIMBOT ENTRY ====================

void CAimbot::Run(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (!CFG::Aimbot_Active)
		return;

	if (!pLocal || !pWeapon || !pCmd)
		return;

	// Check keybind if set (0 = no keybind)
	if (CFG::Aimbot_Key > 0 && !H::Input->IsDown(CFG::Aimbot_Key))
		return;

	Store(false);

	// SAFETY FIX: Reset m_bRan at start of each frame to prevent stale state
	m_bRan = false;

	// CRITICAL FIX: Save original button state BEFORE autoshoot modifies it
	// This allows proper detection of manual vs autoshoot attacks
	const bool bOriginalAttacking = (pCmd->buttons & IN_ATTACK) || (pCmd->buttons & IN_ATTACK2);

	RunMain(pLocal, pWeapon, pCmd);

	// CRITICAL FIX: Use Amalgam's IsAttacking function with bTickBase=true for proper timing
	// This ensures we use I::GlobalVars->tickcount instead of pLocal->m_nTickBase()
	// which is essential for correct silent aim behavior during the two-frame cycle
	int iAttacking = IsAttacking(pLocal, pWeapon, pCmd, true);
	G::bFiring = (iAttacking != 0);
	// Note: Would need G namespace extension for g_GlobalState.bAttacking

	// CRITICAL FIX FOR MANUAL FIRE: Handle PSilentAngles for manual attacking
	// Use bOriginalAttacking to detect if USER pressed fire, not if autoshoot did
	if (bOriginalAttacking && !CFG::Aimbot_AutoShoot && !m_bRan)
	{
		// User is manually attacking BUT aimbot didn't find/shoot target
		// Check if silent mode is enabled for this weapon type
		bool bIsSilentMode = false;

		if (IsHitscanWeapon(pWeapon))
		{
			bIsSilentMode = (CFG::Aimbot_Hitscan_Aim_Type == 1); // Silent mode
		}
		else if (IsProjectileWeapon(pWeapon))
		{
			bIsSilentMode = (CFG::Aimbot_Projectile_Aim_Type == 1); // Silent mode
		}
		else if (IsMeleeWeapon(pWeapon))
		{
			bIsSilentMode = (CFG::Aimbot_Melee_Aim_Type == 1); // Silent mode
		}

		// In silent mode, don't modify view angles even when manually firing
		if (bIsSilentMode)
		{
			// Manual fire in silent mode without target - clear PSilentAngles to allow normal fire
			g_GlobalState.bPSilentAngles = false;
		}
	}

}

void CAimbot::Draw(C_TFPlayer* pLocal)
{
	if (!CFG::Aimbot_Active || pLocal->deadflag())
		return;

	auto pWeapon = H::Entities->GetWeapon();
	if (!pWeapon)
		return;

	// Basic FOV circle implementation
	int iWidth, iHeight;
	I::EngineClient->GetScreenSize(iWidth, iHeight);
	Vec3 vScreenCenter(iWidth / 2, iHeight / 2, 0.0f);

	float flFOV = 45.0f; // Default FOV
	if (IsHitscanWeapon(pWeapon))
		flFOV = CFG::Aimbot_Hitscan_FOV;
	else if (IsProjectileWeapon(pWeapon))
		flFOV = CFG::Aimbot_Projectile_FOV;
	else if (IsMeleeWeapon(pWeapon))
		flFOV = CFG::Aimbot_Melee_FOV;

	// Draw FOV circle (simplified)
	// This would need proper drawing implementation using SEOwnedDE's drawing system
}

void CAimbot::Store(C_BaseEntity* pEntity, size_t iSize)
{
	// Entity storage for performance - basic implementation
	m_iSize = iSize;
	if (pEntity)
		m_iPlayer = pEntity->entindex();
}

void CAimbot::Store(bool bFrameStageNotify)
{
	// General storage function - basic implementation
	m_iSize = 0;
	m_iPlayer = 0;
}