#pragma once

#include "../../../SDK/SDK.h"

class CRadar
{
	void Drag();
	bool GetDrawPosition(int& x, int& y, const Vec3& vWorld);
	void DrawRadarImGui();
	void DrawPlayerRadar(C_TFPlayer* pPlayer, C_TFPlayer* pLocal);
	void DrawBuildingRadar(C_BaseObject* pBuilding, C_TFPlayer* pLocal);
	void DrawWorldRadar();

	Vec3 m_cachedViewAngles = {};
	int m_nLastViewAngleCacheFrame = -1;

public:
	void Run(); // Original MatSystemSurface rendering
	void RunImGui(); // ImGui rendering for stream-proof
	void UpdateViewAngleCache();
};

MAKE_SINGLETON_SCOPED(CRadar, Radar, F);
