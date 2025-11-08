#pragma once

#include "../../../SDK/SDK.h"

class CSpyWarning
{
public:
	void Run();
	void RunImGui();
private:
	void DrawSpyWarningImGui();
};

MAKE_SINGLETON_SCOPED(CSpyWarning, SpyWarning, F);
