#pragma once
#include "../../../SDK/SDK.h"

#include "../../../SDK/TF2/c_tf_player.h"
#include "../../../SDK/TF2/tf_weaponbase.h"
#include "../../../SDK/TF2/prediction.h"
#include "../../../SDK/TF2/c_baseobject.h"
#include "../../../SDK/Helpers/AimUtils/AimUtils.h"
#include "../../../SDK/TF2/studio.h"
#include "../../../SDK/Impl/TraceFilters/TraceFilters.h"
#include "../EnginePrediction/EnginePrediction.h"
#include "../LagRecords/LagRecords.h"
#include "../ProjectileSim/ProjectileSim.h"
#include "../MovementSimulation/MovementSimulation.h"

// Forward declarations to avoid circular includes
class CUserCmd;

// Target types enum from Amalgam - renamed to avoid conflicts
enum TargetTypeEnum
{
	Target_Unknown = 0,
	Target_Player,
	Target_Sentry,
	Target_Dispenser,
	Target_Teleporter,
	Target_Sticky,
	Target_NPC,
	Target_Bomb
};

// Hitbox combinations (using existing ETFHitboxes from const.h)
#define HITBOX_COMBINATION_ALL (HITBOX_HEAD | HITBOX_BODY | HITBOX_PELVIS | HITBOX_RIGHT_UPPER_ARM | HITBOX_LEFT_UPPER_ARM | HITBOX_RIGHT_FOREARM | HITBOX_LEFT_FOREARM | HITBOX_RIGHT_THIGH | HITBOX_LEFT_THIGH | HITBOX_RIGHT_CALF | HITBOX_LEFT_CALF | HITBOX_RIGHT_FOOT | HITBOX_LEFT_FOOT)

// BOUNDS hitbox types (ported from Amalgam - using different names to avoid conflicts)
enum BOUNDS_HITBOX_TYPES
{
	BOUNDS_HEAD_IDX = 0,
	BOUNDS_BODY_IDX = 1,
	BOUNDS_FEET_IDX = 2
};

// Target structure from Amalgam
struct Target_t
{
	C_BaseEntity* m_pEntity = nullptr;
	int m_iTargetType = Target_Unknown;
	Vec3 m_vPos = {};
	Vec3 m_vAngleTo = {};
	float m_flFOVTo = std::numeric_limits<float>::max();
	float m_flDistTo = std::numeric_limits<float>::max();
	int m_nPriority = 0;
	int m_nAimedHitbox = -1;
};

// Global state access - use existing definition from GlobalState.h
#include "GlobalState.h"

class CAimbot
{
private:
	// General methods
	bool ShouldRun(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	void RunMain(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd);

	// Weapon type detection
	bool IsHitscanWeapon(C_TFWeaponBase* pWeapon);
	bool IsProjectileWeapon(C_TFWeaponBase* pWeapon);
	bool IsMeleeWeapon(C_TFWeaponBase* pWeapon);

	// Global aimbot functionality (from AimbotGlobal) - Enhanced with Amalgam features
	bool ShouldIgnore(C_BaseEntity* pTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	bool PlayerBoneInFOV(C_TFPlayer* pTarget, Vec3 vLocalPos, Vec3 vLocalAngles, float& flFOVTo, Vec3& vPos, Vec3& vAngleTo, int iHitboxes);
	int GetSelectedHitboxes();
	void SortTargets(std::vector<Target_t>& vTargets, int iMethod);
	void SortPriority(std::vector<Target_t>& vTargets);
	int GetPriority(int iIndex);

	// Hitbox validation methods (ported from Amalgam)
	bool IsHitboxValid(C_BaseEntity* pEntity, int nHitbox, int iHitboxes);
	bool IsHitboxValid(int nHitbox, int iHitboxes);
	bool ShouldMultipoint(C_BaseEntity* pEntity = nullptr, int nHitbox = -1, int iHitboxes = HITBOX_COMBINATION_ALL);

	// Aim control methods (ported from Amalgam)
	bool ShouldAim();
	bool ShouldHoldAttack(C_TFWeaponBase* pWeapon);
	bool ValidBomb(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, C_BaseEntity* pBomb);
	bool FriendlyFire();

	// Hitscan aimbot methods
	std::vector<Target_t> GetTargetsHitscan(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	int CanHitHitscan(Target_t& tTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	bool AimHitscan(Vec3 vCurAngle, Vec3 vToAngle, Vec3& vOut, int iMethod);
	void AimHitscan(CUserCmd* pCmd, Vec3& vAngle, int iMethod);
	bool ShouldFireHitscan(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd, const Target_t& tTarget);
	void RunHitscan(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd);

	// Projectile aimbot methods - Enhanced from Amalgam
	enum SolutionState
	{
		SOLUTION_PENDING = 0,
		SOLUTION_GOOD = 1,
		SOLUTION_TIME = 2,
		SOLUTION_BAD = 3
	};

	enum PointType
	{
		POINT_NONE = 0,
		POINT_REGULAR = 1 << 0,
		POINT_OBSCURED = 1 << 1,
		POINT_OBSCURED_EXTRA = 1 << 2,
		POINT_OBSCURED_MULTI = 1 << 3
	};

	struct Solution_t
	{
		float m_flPitch = 0.f;
		float m_flYaw = 0.f;
		float m_flTime = 0.f;
		int m_iCalculated = SOLUTION_PENDING;
	};

	struct Point_t
	{
		Vec3 m_vPoint = {};
		Solution_t m_tSolution = {};
		int m_iPointType = POINT_NONE;
	};

	struct Info_t
	{
		C_TFPlayer* m_pLocal = nullptr;
		C_TFWeaponBase* m_pWeapon = nullptr;
		Vec3 m_vLocalEye = {};
		Vec3 m_vTargetEye = {};
		float m_flLatency = 0.f;
		Vec3 m_vHull = {};
		Vec3 m_vOffset = {};
		Vec3 m_vAngFix = {};
		float m_flVelocity = 0.f;
		float m_flGravity = 0.f;
		float m_flRadius = 0.f;
		float m_flRadiusTime = 0.f;
		float m_flBoundingTime = 0.f;
		float m_flOffsetTime = 0.f;
		int m_iSplashCount = 0;
		int m_iSplashMode = 0;
		float m_flPrimeTime = 0.f;
		int m_iPrimeTime = 0;
	};

	std::vector<Target_t> GetTargetsProjectile(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	void CalculateAngle(const Vec3& vLocalPos, const Vec3& vTargetPos, int iSimTime, Solution_t& out);
	bool TestAngleProjectile(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, Target_t& tTarget, Vec3& vPoint, Vec3& vAngles, int iSimTime);
	int CanHitProjectile(Target_t& tTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	bool AimProjectile(Vec3 vCurAngle, Vec3 vToAngle, Vec3& vOut, int iMethod);
	void AimProjectile(CUserCmd* pCmd, Vec3& vAngle, int iMethod);
	void RunProjectile(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd);
	float GetProjectileSpeed(C_TFWeaponBase* pWeapon);
	float GetProjectileGravity(C_TFWeaponBase* pWeapon);
	Vec3 GetProjectileSpawnPosition(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, Vec3 vAngles);

	// Melee aimbot methods
	std::vector<Target_t> GetTargetsMelee(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	int GetSwingTime(C_TFWeaponBase* pWeapon, bool bVar = true);
	bool CanBackstab(C_BaseEntity* pTarget, C_TFPlayer* pLocal, Vec3 vEyeAngles);
	int CanHitMelee(Target_t& tTarget, C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon);
	bool AimMelee(Vec3 vCurAngle, Vec3 vToAngle, Vec3& vOut, int iMethod);
	void AimMelee(CUserCmd* pCmd, Vec3& vAngle, int iMethod);
	void RunMelee(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd);

	// Member variables
	size_t m_iSize = 0;
	int m_iPlayer = 0;
	Vec3 m_vEyePos = {};

	// Projectile specific
	Info_t m_tProjectileInfo = {};
	float m_flTimeTo = std::numeric_limits<float>::max();

	// Melee specific
	float m_flMeleeRange = 0.f;
	bool m_bShouldSwing = false;

public:
	void Run(C_TFPlayer* pLocal, C_TFWeaponBase* pWeapon, CUserCmd* pCmd);
	void Draw(C_TFPlayer* pLocal);
	void Store(C_BaseEntity* pEntity, size_t iSize);
	void Store(bool bFrameStageNotify = true);

	bool m_bRan = false;
	bool m_bRunningSecondary = false;
};

MAKE_SINGLETON_SCOPED(CAimbot, Aimbot, F);