#pragma once

#include "../../../SDK/SDK.h"

class CMiscVisuals
{
public:
	void AimbotFOVCircleImGui();
	void ViewModelSway();
	void DetailProps();
	void ShiftBar();
	void ShiftBarImGui();

	void SniperLines();
	void ProjectileArc();

	void CustomFOV(CViewSetup* pSetup);
	void Thirdperson(CViewSetup* pSetup);
private:
	void DrawAimbotFOVCircleImGui();
	void DrawShiftBarImGui();
};

MAKE_SINGLETON_SCOPED(CMiscVisuals, MiscVisuals, F);
