#pragma once

#include "../../../SDK/SDK.h"

class CESP
{
	bool GetDrawBounds(C_BaseEntity* pEntity, int& x, int& y, int& w, int& h);
	void DrawBones(C_TFPlayer* pPlayer, Color_t color);
	void DrawPlayerESP(C_TFPlayer* pPlayer, C_TFPlayer* pLocal, int x, int y, int w, int h);
	void DrawBuildingESP(C_BaseObject* pBuilding, C_TFPlayer* pLocal);
	void DrawProjectileESP(C_BaseEntity* pProjectile, C_TFPlayer* pLocal);
	void DrawWorldESP();

public:
	void Run();
	void RunImGui();

	// Helper function to apply alpha to colors
	static inline Color_t ApplyAlpha(Color_t color, float alpha)
	{
		color.a = static_cast<unsigned char>(255.0f * alpha);
		return color;
	}
};

MAKE_SINGLETON_SCOPED(CESP, ESP, F);
