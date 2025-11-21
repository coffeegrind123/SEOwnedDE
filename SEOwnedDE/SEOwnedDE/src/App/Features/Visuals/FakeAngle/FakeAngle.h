#pragma once
#include "../../../../SDK/SDK.h"
#include "../../../../SDK/TF2/studio.h"
#include "../../../../SDK/TF2/multiplayer_animstate.h"

class CFakeAngle
{
public:
	void Run(C_TFPlayer* pLocal);

	matrix3x4_t aBones[MAXSTUDIOBONES];
	bool bBonesSetup = false;

	bool bDrawChams = false;

private:
	Vec3 vFakeAngles = Vec3(0.0f, 0.0f, 0.0f);
	bool bAntiAimOn = false;
	bool bFakelagOn = false;
	int iShiftedTicks = 0;
	int iMaxShift = 0;

	bool ShouldRun();
	void UpdateFakeAngles();
};

MAKE_SINGLETON_SCOPED(CFakeAngle, FakeAngle, F);