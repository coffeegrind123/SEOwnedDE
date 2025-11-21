#pragma once

#include "../../../SDK/SDK.h"

struct ProjectileInfo
{
	ProjectileType_t m_type{};

	Vec3 m_pos{};
	Vec3 m_ang{};

	float m_speed{};
	float m_gravity_mod{};

	bool no_spin{};

	// Drag support (ported from Amalgam)
	bool m_bDragEnabled{};
	Vec3 m_vDragBasis{};
};

class CProjectileSim
{
public:
	bool GetInfo(C_TFPlayer *player, C_TFWeaponBase *weapon, const Vec3 &angles, ProjectileInfo &out);
	bool Init(const ProjectileInfo &info, bool no_vec_up = false);
	void RunTick();
	Vec3 GetOrigin();

	// Drag support methods (ported from Amalgam)
	bool IsDragEnabled() const { return m_ProjectileInfo.m_bDragEnabled; }
	Vec3 GetDragBasis() const { return m_ProjectileInfo.m_vDragBasis; }

private:
	ProjectileInfo m_ProjectileInfo{};
};

MAKE_SINGLETON_SCOPED(CProjectileSim, ProjectileSim, F);