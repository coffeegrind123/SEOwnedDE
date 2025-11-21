#pragma once

#include "../../../Utils/Vector/Vector.h"

// Forward declarations
class CUserCmd;

// Global state structure (ported from Amalgam, adapted for SEOwnedDE)
struct GlobalState_t
{
	bool bPSilentAngles = false;
	bool bSilentAngles = false;
	bool bAttacking = false;
	bool bThrowing = false;
	bool bReloading = false;
	bool bCanPrimaryAttack = false;
	bool bCanSecondaryAttack = false;
	bool bCanHeadshot = false;
	bool bChoking = false; // Missing field from Amalgam - critical for packet management
	CUserCmd* pLastUserCmd = nullptr;
	CUserCmd* pCurrentUserCmd = nullptr;
	// CUserCmd OriginalCmd = {}; // Commented out - OriginalCmd was causing compilation issues
	Vec3 vUserCmdAngles = {};
	int nOldButtons = 0;

	// Additional Amalgam fields for complete compatibility
	int nShiftedTicks = 0;
	float flShiftTime = 0.0f;
	bool bShouldShift = false;
	bool bIsCharging = false;
};

// Global state access
extern GlobalState_t g_GlobalState;