#include "App.h"

#include "Hooks/WINAPI_WndProc.h"

#include "Features/Materials/Materials.h"
#include "Features/Outlines/Outlines.h"
#include "Features/WorldModulation/WorldModulation.h"
#include "Features/Paint/Paint.h"
#include "Features/Menu/Menu.h"
#include "Features/Menu/basicHook.h"
#include "Features/Players/Players.h"

#include "Features/CFG.h"

void CApp::Start()
{
	while (!Memory::FindSignature("client.dll", "48 8B 0D ? ? ? ? 4C 8D 05") ||
	       !Memory::FindSignature("client.dll", "48 8B 0D ? ? ? ? F3 0F 59 CA 44 8D 42"))
	{
		bUnload = GetAsyncKeyState(VK_F11) & 0x8000;
		if (bUnload)
			return;

		Sleep(500);
	}

	U::Storage->Init("SEOwnedDE");
	U::SignatureManager->InitializeAllSignatures();
	U::InterfaceManager->InitializeAllInterfaces();

	H::Draw->UpdateScreenSize();
	
	H::Fonts->Reload();

	if (I::EngineClient->IsInGame() && I::EngineClient->IsConnected())
	{
		H::Entities->UpdateModelIndexes();
	}
	
	U::HookManager->InitializeAllHooks();

	Hooks::WINAPI_WndProc::Init();

	// Initialize Steam overlay hooks for ImGui rendering
	hooks::Initialize();

	F::Players->Parse();

	Config::Load(U::Storage->GetConfigFolder() / "default.json");

	const auto month = []
	{
		const std::time_t t = std::time(nullptr);
		tm Time = {};
		localtime_s(&Time, &t);

		return Time.tm_mon + 1;
	}();

	Color_t msgColor = { 197, 108, 240, 255 };
	if (month == 10)
	{
		I::MatSystemSurface->PlaySound("vo\\halloween_boss\\knight_alert.mp3");
		msgColor = { 247, 136, 18, 255 };
	}
	else if (month == 12 || month == 1 || month == 2)
	{
		if (month == 12)
		{
			I::MatSystemSurface->PlaySound("misc\\jingle_bells\\jingle_bells_nm_04.wav");
		}

		msgColor = { 28, 179, 210, 255 };
	}

	I::CVar->ConsoleColorPrintf(msgColor, "SEOwnedDE Loaded!\n");
}

void CApp::Loop()
{
	while (true)
	{
		bool bShouldUnload = GetAsyncKeyState(VK_F11) & 0x8000 && SDKUtils::IsGameWindowInFocus() || bUnload;
		if (bShouldUnload)
			break;

		Sleep(50);
	}
}

void CApp::Shutdown()
{
	if (!bUnload)
	{
		// Uninitialize Steam overlay hooks first
		hooks::Uninitialize();

		U::HookManager->FreeAllHooks();

		Hooks::WINAPI_WndProc::Release();

		Sleep(250);

		F::Materials->CleanUp();
		F::Outlines->CleanUp();
		F::Paint->CleanUp();

		F::WorldModulation->RestoreWorldModulation();

		if (const auto cl_wpn_sway_interp{ I::CVar->FindVar("cl_wpn_sway_interp") })
		{
			cl_wpn_sway_interp->SetValue(0.0f);
		}

		if (F::Menu->IsOpen())
		{
			I::MatSystemSurface->SetCursorAlwaysVisible(false);
		}
	}

	I::CVar->ConsoleColorPrintf({ 255, 70, 70, 255 }, "SEOwnedDE Unloaded!\n");
}
