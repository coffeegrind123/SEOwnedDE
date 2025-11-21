#pragma once

#include "../../../SDK/SDK.h"

class CTeamWellBeing
{
	void Drag();

public:
	void Run();
	void RunImGui();
private:
	void DrawTeamWellBeingImGui();
};

MAKE_SINGLETON_SCOPED(CTeamWellBeing, TeamWellBeing, F);
