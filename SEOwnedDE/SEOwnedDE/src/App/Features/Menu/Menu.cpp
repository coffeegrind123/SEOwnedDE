#include "Menu.h"
#include "imguiHook.h"

#include "../CFG.h"
#include "../VisualUtils/VisualUtils.h"
#include "../Players/Players.h"
#include "../../../Utils/Utils.h"
#include <Windows.h>
#include <filesystem>

void CMenu::Initialize(IDirect3DDevice9* pDevice)
{
    if (m_bInitialized)
        return;

    m_pDevice = pDevice;

    static ImVec2 windowSize = { static_cast<float>(CFG::Menu_Width), static_cast<float>(CFG::Menu_Height) };
    static ImVec2 windowPos = { static_cast<float>(CFG::Menu_Pos_X), static_cast<float>(CFG::Menu_Pos_Y) };
    m_vWindowSize = &windowSize;
    m_vWindowPos = &windowPos;

    HWND hWnd = SDKUtils::GetTeamFortressWindow();
    if (!hWnd)
    {
        m_bInitialized = true;
        return;
    }

    if (ImGui::GetCurrentContext() != nullptr)
    {
        m_bImGuiContextCreated = true;
    }
    else if (pDevice)
    {
        try {
            imguiHook::InitializeImgui(pDevice);
            m_bImGuiContextCreated = (ImGui::GetCurrentContext() != nullptr);
        }
        catch (...) {
            m_bImGuiContextCreated = false;
        }
    }

    m_bInitialized = true;
}

void CMenu::Shutdown()
{
    if (!m_bInitialized)
        return;

    // Restore system cursor if menu was open
    if (m_bOpen) {
        ImGui::GetIO().MouseDrawCursor = false;

        // Force show system cursor (handle reference counting)
        while (::ShowCursor(TRUE) < 0) {}  // Keep calling until cursor is visible

        m_bOpen = false;
    }

    // Shutdown ImGui if context was created
    if (m_bImGuiContextCreated && ImGui::GetCurrentContext())
    {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        m_bImGuiContextCreated = false;
    }

    m_bInitialized = false;
}

std::string CMenu::GetKeyName(int nKey)
{
    switch (nKey)
    {
        case VK_LBUTTON: return "LButton";
        case VK_RBUTTON: return "RButton";
        case VK_MBUTTON: return "MButton";
        case VK_XBUTTON1: return "XButton1";
        case VK_XBUTTON2: return "XButton2";
        case VK_NUMPAD0: return "NumPad0";
        case VK_NUMPAD1: return "NumPad1";
        case VK_NUMPAD2: return "NumPad2";
        case VK_NUMPAD3: return "NumPad3";
        case VK_NUMPAD4: return "NumPad4";
        case VK_NUMPAD5: return "NumPad5";
        case VK_NUMPAD6: return "NumPad6";
        case VK_NUMPAD7: return "NumPad7";
        case VK_NUMPAD8: return "NumPad8";
        case VK_NUMPAD9: return "NumPad9";
        case VK_MENU: return "Alt";
        case VK_CAPITAL: return "Caps Lock";
        case 0x0: return "None";
        default: break;
    }

    CHAR output[16] = { "\0" };
    if (const int result = GetKeyNameTextA(MapVirtualKeyW(nKey, MAPVK_VK_TO_VSC) << 16, output, 16))
        return output;

    return "Unknown";
}

bool CMenu::InputKey(const char* szLabel, int& nKeyOut)
{
    ImGui::PushID(szLabel);

    std::string keyName = GetKeyName(nKeyOut);
    std::string buttonText = keyName + "##" + szLabel;

    bool bChanged = false;
    if (ImGui::Button(buttonText.c_str(), { 100, 0 }))
    {
        ImGui::OpenPopup(szLabel);
        m_bInKeybind = true;
    }

    ImGui::SameLine();
    ImGui::Text(szLabel);

    KeybindPopup(szLabel, nKeyOut);

    ImGui::PopID();
    return bChanged;
}

void CMenu::KeybindPopup(const char* szLabel, int& nKeyOut)
{
    if (ImGui::BeginPopup(szLabel))
    {
        ImGui::Text("Press any key...");

        bool bKeySet = false;
        for (int n = 0; n < 256; n++)
        {
            bool bMouse = (n > 0x0 && n < 0x7);
            bool bLetter = (n > L'A' - 1 && n < L'Z' + 1);
            bool bAllowed = (n == VK_LSHIFT || n == VK_RSHIFT || n == VK_SHIFT || n == VK_ESCAPE || n == VK_INSERT || n == VK_F3 || n == VK_MENU || n == VK_CAPITAL || n == VK_SPACE || n == VK_CONTROL);
            bool bNumPad = n > (VK_NUMPAD0 - 1) && n < (VK_NUMPAD9)+1;

            if (bMouse || bLetter || bAllowed || bNumPad)
            {
                if (H::Input->IsPressed(n))
                {
                    if (n == VK_INSERT || n == VK_F3) {
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                    else if (n == VK_ESCAPE) {
                        nKeyOut = 0x0;
                        ImGui::CloseCurrentPopup();
                        break;
                    }
                    else {
                        nKeyOut = n;
                        ImGui::CloseCurrentPopup();
                        bKeySet = true;
                        break;
                    }
                }
            }
        }

        if (!bKeySet && H::Input->IsPressed(VK_ESCAPE))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    else
    {
        m_bInKeybind = false;
    }
}

void CMenu::RenderAimTab()
{
    if (ImGui::BeginTabItem("Aim"))
    {
        static int aimSubTab = 0;
        const char* aimSubTabs[] = { "Aimbot", "Triggerbot" };

        if (ImGui::BeginTabBar("AimSubTabs"))
        {
            if (ImGui::BeginTabItem("Aimbot"))
            {
                RenderAimbotTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Triggerbot"))
            {
                RenderTriggerbotTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndTabItem();
    }
}

void CMenu::RenderAimbotTab()
{
    // Global
    if (ImGui::CollapsingHeader("Global##Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##Aimbot", &CFG::Aimbot_Active);
        ImGui::Checkbox("Auto Shoot", &CFG::Aimbot_AutoShoot);
        InputKey("Key", CFG::Aimbot_Key);

        // Targets
        ImGui::Text("Targets:");
        ImGui::Checkbox("Players##Aimbot", &CFG::Aimbot_Target_Players);
        ImGui::SameLine();
        ImGui::Checkbox("Buildings##Aimbot", &CFG::Aimbot_Target_Buildings);

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Friends##Aimbot", &CFG::Aimbot_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Invisible##Aimbot", &CFG::Aimbot_Ignore_Invisible);
        ImGui::SameLine();
        ImGui::Checkbox("Invulnerable##Aimbot", &CFG::Aimbot_Ignore_Invulnerable);
        ImGui::SameLine();
        ImGui::Checkbox("Taunting", &CFG::Aimbot_Ignore_Taunting);
    }

    // Melee
    if (ImGui::CollapsingHeader("Melee", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##AimbotMelee", &CFG::Aimbot_Melee_Active);
        ImGui::Checkbox("Always Active", &CFG::Aimbot_Melee_Always_Active);
        ImGui::Checkbox("Target Lag Records##Melee", &CFG::Aimbot_Melee_Target_LagRecords);
        ImGui::Checkbox("Predict Swing", &CFG::Aimbot_Melee_Predict_Swing);
        ImGui::Checkbox("Walk To Target", &CFG::Aimbot_Melee_Walk_To_Target);
        ImGui::Checkbox("Whip Teammates", &CFG::Aimbot_Melee_Whip_Teammates);

        // Aim Type
        const char* aimTypes[] = { "Normal", "Silent", "Smooth" };
        ImGui::Combo("Aim Type##Melee", &CFG::Aimbot_Melee_Aim_Type, aimTypes, IM_ARRAYSIZE(aimTypes));

        // Sort
        const char* sortTypes[] = { "FOV", "Distance" };
        ImGui::Combo("Sort##Melee", &CFG::Aimbot_Melee_Sort, sortTypes, IM_ARRAYSIZE(sortTypes));

        ImGui::SliderFloat("FOV##Melee", &CFG::Aimbot_Melee_FOV, 1.0f, 180.0f, "%.0f");
        ImGui::SliderFloat("Smoothing##Melee", &CFG::Aimbot_Melee_Smoothing, 0.0f, 20.0f, "%.1f");
        ImGui::SliderFloat("Predict Swing Time", &CFG::Aimbot_Melee_Predict_Swing_Amount, 0.1f, 0.2f, "%.2f");
    }

    // Hitscan
    if (ImGui::CollapsingHeader("Hitscan", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##AimbotHitscan", &CFG::Aimbot_Hitscan_Active);
        ImGui::Checkbox("Target Lag Records##Hitscan", &CFG::Aimbot_Hitscan_Target_LagRecords);
        ImGui::Checkbox("Target Stickies", &CFG::Aimbot_Hitscan_Target_Stickies);
        ImGui::Checkbox("Smooth Auto Shoot", &CFG::Aimbot_Hitscan_Advanced_Smooth_AutoShoot);
        ImGui::Checkbox("Auto Scope", &CFG::Aimbot_Hitscan_Auto_Scope);
        ImGui::Checkbox("Wait For Headshot", &CFG::Aimbot_Hitscan_Wait_For_Headshot);
        ImGui::Checkbox("Wait For Charge", &CFG::Aimbot_Hitscan_Wait_For_Charge);
        ImGui::Checkbox("Minigun Tapfire", &CFG::Aimbot_Hitscan_Minigun_TapFire);

        // Aim Type
        const char* aimTypes[] = { "Normal", "Silent", "Smooth" };
        ImGui::Combo("Aim Type##Hitscan", &CFG::Aimbot_Hitscan_Aim_Type, aimTypes, IM_ARRAYSIZE(aimTypes));

        // Hitbox
        const char* hitboxes[] = { "Head", "Body", "Auto" };
        ImGui::Combo("Hitbox", &CFG::Aimbot_Hitscan_Hitbox, hitboxes, IM_ARRAYSIZE(hitboxes));

        // Sort
        const char* sortTypes[] = { "FOV", "Distance" };
        ImGui::Combo("Sort##Hitscan", &CFG::Aimbot_Hitscan_Sort, sortTypes, IM_ARRAYSIZE(sortTypes));

        // Scan
        ImGui::Text("Scan:");
        ImGui::Checkbox("Head", &CFG::Aimbot_Hitscan_Scan_Head);
        ImGui::SameLine();
        ImGui::Checkbox("Body", &CFG::Aimbot_Hitscan_Scan_Body);
        ImGui::SameLine();
        ImGui::Checkbox("Arms", &CFG::Aimbot_Hitscan_Scan_Arms);
        ImGui::SameLine();
        ImGui::Checkbox("Legs", &CFG::Aimbot_Hitscan_Scan_Legs);
        ImGui::Checkbox("Buildings##Hitscan", &CFG::Aimbot_Hitscan_Scan_Buildings);

        ImGui::SliderFloat("FOV##Hitscan", &CFG::Aimbot_Hitscan_FOV, 1.0f, 180.0f, "%.0f");
        ImGui::SliderFloat("Smoothing##Hitscan", &CFG::Aimbot_Hitscan_Smoothing, 0.0f, 20.0f, "%.1f");
    }

    // Projectile
    if (ImGui::CollapsingHeader("Projectile", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##AimbotProjectile", &CFG::Aimbot_Projectile_Active);
        ImGui::Checkbox("No Spread", &CFG::Aimbot_Projectile_NoSpread);
        ImGui::Checkbox("Auto Double Donk", &CFG::Aimbot_Projectile_Auto_Double_Donk);
        ImGui::Checkbox("Advanced Head Aim", &CFG::Aimbot_Projectile_Advanced_Head_Aim);
        ImGui::Checkbox("Ground Strafe Prediction", &CFG::Aimbot_Projectile_Ground_Strafe_Prediction);
        ImGui::Checkbox("Air Strafe Prediction", &CFG::Aimbot_Projectile_Air_Strafe_Prediction);
        ImGui::Checkbox("BBOX Multipoint", &CFG::Aimbot_Projectile_BBOX_Multipoint);

        // Rocket Splash
        const char* rocketSplash[] = { "Disabled", "Enabled", "Preferred" };
        ImGui::Combo("Rocket Splash", &CFG::Aimbot_Projectile_Rocket_Splash, rocketSplash, IM_ARRAYSIZE(rocketSplash));

        // Aim Type
        const char* aimTypes[] = { "Normal", "Silent" };
        ImGui::Combo("Aim Type##Projectile", &CFG::Aimbot_Projectile_Aim_Type, aimTypes, IM_ARRAYSIZE(aimTypes));

        // Aim Position
        const char* aimPositions[] = { "Feet", "Body", "Head", "Auto" };
        ImGui::Combo("Aim Position", &CFG::Aimbot_Projectile_Aim_Position, aimPositions, IM_ARRAYSIZE(aimPositions));

        // Sort
        const char* sortTypes[] = { "FOV", "Distance" };
        ImGui::Combo("Sort##Projectile", &CFG::Aimbot_Projectile_Sort, sortTypes, IM_ARRAYSIZE(sortTypes));

        // Prediction Method
        const char* predictionMethods[] = { "Full Acceleration", "Current Velocity" };
        ImGui::Combo("Prediction Method", &CFG::Aimbot_Projectile_Aim_Prediction_Method, predictionMethods, IM_ARRAYSIZE(predictionMethods));

        ImGui::SliderFloat("FOV##Projectile", &CFG::Aimbot_Projectile_FOV, 1.0f, 180.0f, "%.0f");
        ImGui::SliderFloat("Max Simulation Time", &CFG::Aimbot_Projectile_Max_Simulation_Time, 1.0f, 5.0f, "%.1fs");
        ImGui::SliderInt("Max Targets", &CFG::Aimbot_Projectile_Max_Processing_Targets, 1, 6);
    }
}

void CMenu::RenderTriggerbotTab()
{
    // Global
    if (ImGui::CollapsingHeader("Global##Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##Triggerbot", &CFG::Triggerbot_Active);
        InputKey("Key", CFG::Triggerbot_Key);
    }

    // Auto Airblast
    if (ImGui::CollapsingHeader("Auto Airblast", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##TriggerbotAirblast", &CFG::Triggerbot_AutoAirblast_Active);
        ImGui::Checkbox("Aim Assist", &CFG::Triggerbot_AutoAirblast_Aim_Assist);

        // Mode
        const char* modes[] = { "Legit", "Rage" };
        ImGui::Combo("Mode##Airblast", &CFG::Triggerbot_AutoAirblast_Mode, modes, IM_ARRAYSIZE(modes));

        // Aim Mode
        const char* aimModes[] = { "Normal", "Silent" };
        ImGui::Combo("Aim Mode##Airblast", &CFG::Triggerbot_AutoAirblast_Aim_Mode, aimModes, IM_ARRAYSIZE(aimModes));

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Rocket", &CFG::Triggerbot_AutoAirblast_Ignore_Rocket);
        ImGui::SameLine();
        ImGui::Checkbox("Sentry Rocket", &CFG::Triggerbot_AutoAirblast_Ignore_SentryRocket);
        ImGui::Checkbox("Jarate", &CFG::Triggerbot_AutoAirblast_Ignore_Jar);
        ImGui::SameLine();
        ImGui::Checkbox("Gas", &CFG::Triggerbot_AutoAirblast_Ignore_JarGas);
        ImGui::SameLine();
        ImGui::Checkbox("Milk", &CFG::Triggerbot_AutoAirblast_Ignore_JarMilk);
        ImGui::Checkbox("Arrow", &CFG::Triggerbot_AutoAirblast_Ignore_Arrow);
        ImGui::SameLine();
        ImGui::Checkbox("Flare", &CFG::Triggerbot_AutoAirblast_Ignore_Flare);
        ImGui::SameLine();
        ImGui::Checkbox("Cleaver", &CFG::Triggerbot_AutoAirblast_Ignore_Cleaver);
        ImGui::Checkbox("Healing Bolt", &CFG::Triggerbot_AutoAirblast_Ignore_HealingBolt);
        ImGui::SameLine();
        ImGui::Checkbox("Pipebomb", &CFG::Triggerbot_AutoAirblast_Ignore_PipebombProjectile);
        ImGui::Checkbox("Ball of Fire", &CFG::Triggerbot_AutoAirblast_Ignore_BallOfFire);
        ImGui::SameLine();
        ImGui::Checkbox("Energy Ring", &CFG::Triggerbot_AutoAirblast_Ignore_EnergyRing);
        ImGui::SameLine();
        ImGui::Checkbox("Energy Ball", &CFG::Triggerbot_AutoAirblast_Ignore_EnergyBall);
    }

    // Auto Detonate
    if (ImGui::CollapsingHeader("Auto Detonate", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##TriggerbotDetonate", &CFG::Triggerbot_AutoDetonate_Active);

        // Targets
        ImGui::Text("Targets:");
        ImGui::Checkbox("Players##Triggerbot", &CFG::Triggerbot_AutoDetonate_Target_Players);
        ImGui::SameLine();
        ImGui::Checkbox("Buildings##Triggerbot", &CFG::Triggerbot_AutoDetonate_Target_Buildings);

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Friends##AutoDetonate", &CFG::Triggerbot_AutoDetonate_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Invisible##AutoDetonate", &CFG::Triggerbot_AutoDetonate_Ignore_Invisible);
        ImGui::SameLine();
        ImGui::Checkbox("Invulnerable##Detonate", &CFG::Triggerbot_AutoDetonate_Ignore_Invulnerable);
    }

    // Auto Backstab
    if (ImGui::CollapsingHeader("Auto Backstab", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##TriggerbotBackstab", &CFG::Triggerbot_AutoBackstab_Active);
        ImGui::Checkbox("Knife If Lethal", &CFG::Triggerbot_AutoBackstab_Knife_If_Lethal);

        // Mode
        const char* modes[] = { "Legit", "Rage" };
        ImGui::Combo("Mode##Backstab", &CFG::Triggerbot_AutoBacktab_Mode, modes, IM_ARRAYSIZE(modes));

        // Aim Mode
        const char* aimModes[] = { "Normal", "Silent" };
        ImGui::Combo("Aim Mode##Backstab", &CFG::Triggerbot_AutoBacktab_Aim_Mode, aimModes, IM_ARRAYSIZE(aimModes));

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Friends##AutoBackstab", &CFG::Triggerbot_AutoBackstab_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Invisible##AutoBackstab", &CFG::Triggerbot_AutoBackstab_Ignore_Invisible);
        ImGui::SameLine();
        ImGui::Checkbox("Invulnerable##Backstab", &CFG::Triggerbot_AutoBackstab_Ignore_Invulnerable);
    }
}

void CMenu::RenderVisualsTab()
{
    if (ImGui::BeginTabItem("Visuals"))
    {
        if (ImGui::BeginTabBar("VisualsSubTabs"))
        {
            if (ImGui::BeginTabItem("ESP"))
            {
                RenderESPTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Radar"))
            {
                RenderRadarTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Materials"))
            {
                RenderMaterialsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Outlines"))
            {
                RenderOutlinesTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Other"))
            {
                RenderOtherTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Other2"))
            {
                RenderOther2Tab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Colors"))
            {
                RenderColorsTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndTabItem();
    }
}

void CMenu::RenderESPTab()
{
    // Global
    if (ImGui::CollapsingHeader("Global##Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##ESP", &CFG::ESP_Active);

        // Tracer From
        const char* tracerFrom[] = { "Top", "Center", "Bottom" };
        ImGui::Combo("Tracer From", &CFG::ESP_Tracer_From, tracerFrom, IM_ARRAYSIZE(tracerFrom));

        // Tracer To
        const char* tracerTo[] = { "Top", "Center", "Bottom" };
        ImGui::Combo("Tracer To", &CFG::ESP_Tracer_To, tracerTo, IM_ARRAYSIZE(tracerTo));

        // Text Color
        const char* textColors[] = { "Default", "White" };
        ImGui::Combo("Text Color", &CFG::ESP_Text_Color, textColors, IM_ARRAYSIZE(textColors));
    }

    // World
    if (ImGui::CollapsingHeader("World##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##ESPWorld", &CFG::ESP_World_Active);
        ImGui::SliderFloat("Alpha##ESPWorld", &CFG::ESP_World_Alpha, 0.1f, 1.0f, "%.1f");

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Health Packs##ESPWorld", &CFG::ESP_World_Ignore_HealthPacks);
        ImGui::SameLine();
        ImGui::Checkbox("Ammo Packs##ESPWorld", &CFG::ESP_World_Ignore_AmmoPacks);
        ImGui::Checkbox("Local Projectiles##ESPWorld", &CFG::ESP_World_Ignore_LocalProjectiles);
        ImGui::SameLine();
        ImGui::Checkbox("Enemy Projectiles##ESPWorld", &CFG::ESP_World_Ignore_EnemyProjectiles);
        ImGui::SameLine();
        ImGui::Checkbox("Teammate Projectiles##ESPWorld", &CFG::ESP_World_Ignore_TeammateProjectiles);
        ImGui::Checkbox("Halloween Gifts##ESPWorld", &CFG::ESP_World_Ignore_Halloween_Gift);
        ImGui::SameLine();
        ImGui::Checkbox("MVM Money##ESPWorld", &CFG::ESP_World_Ignore_MVM_Money);

        // Draw
        ImGui::Text("Draw:");
        ImGui::Checkbox("Name##ESPWorld", &CFG::ESP_World_Name);
        ImGui::SameLine();
        ImGui::Checkbox("Box##ESPWorld", &CFG::ESP_World_Box);
        ImGui::SameLine();
        ImGui::Checkbox("Tracer##ESPWorld", &CFG::ESP_World_Tracer);
    }

    // Players
    if (ImGui::CollapsingHeader("Players##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##ESPPlayers", &CFG::ESP_Players_Active);
        ImGui::SliderFloat("Alpha##ESPPlayers", &CFG::ESP_Players_Alpha, 0.1f, 1.0f, "%.1f");
        ImGui::SliderFloat("Arrow Radius", &CFG::ESP_Players_Arrows_Radius, 50.0f, 400.0f, "%.0f");
        ImGui::SliderFloat("Arrow Max Distance", &CFG::ESP_Players_Arrows_Max_Distance, 100.0f, 1000.0f, "%.0f");

        // Bones Color
        const char* bonesColors[] = { "Default", "White" };
        ImGui::Combo("Bones Color", &CFG::ESP_Players_Bones_Color, bonesColors, IM_ARRAYSIZE(bonesColors));

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##ESPPlayers", &CFG::ESP_Players_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Friends##ESPPlayers", &CFG::ESP_Players_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##ESPPlayers", &CFG::ESP_Players_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##ESPPlayers", &CFG::ESP_Players_Ignore_Teammates);
        ImGui::Checkbox("Invisible##ESPPlayers", &CFG::ESP_Players_Ignore_Invisible);

        // Draw
        ImGui::Text("Draw:");
        ImGui::Checkbox("Name##ESPPlayers", &CFG::ESP_Players_Name);
        ImGui::SameLine();
        ImGui::Checkbox("Class", &CFG::ESP_Players_Class);
        ImGui::SameLine();
        ImGui::Checkbox("Class Icon", &CFG::ESP_Players_Class_Icon);
        ImGui::Checkbox("Health##ESPPlayers", &CFG::ESP_Players_Health);
        ImGui::SameLine();
        ImGui::Checkbox("Health Bar##ESPPlayers", &CFG::ESP_Players_HealthBar);
        ImGui::SameLine();
        ImGui::Checkbox("Uber", &CFG::ESP_Players_Uber);
        ImGui::Checkbox("Uber Bar", &CFG::ESP_Players_UberBar);
        ImGui::SameLine();
        ImGui::Checkbox("Box##ESPPlayers", &CFG::ESP_Players_Box);
        ImGui::SameLine();
        ImGui::Checkbox("Tracer##ESPPlayers", &CFG::ESP_Players_Tracer);
        ImGui::Checkbox("Bones", &CFG::ESP_Players_Bones);
        ImGui::SameLine();
        ImGui::Checkbox("Arrows", &CFG::ESP_Players_Arrows);
        ImGui::SameLine();
        ImGui::Checkbox("Conds##ESPPlayers", &CFG::ESP_Players_Conds);
        ImGui::Checkbox("Sniper Lines", &CFG::ESP_Players_Sniper_Lines);

        ImGui::Checkbox("Show Team Medics##ESPPlayers", &CFG::ESP_Players_Show_Teammate_Medics);
    }

    // Buildings
    if (ImGui::CollapsingHeader("Buildings##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##ESPBuildings", &CFG::ESP_Buildings_Active);
        ImGui::SliderFloat("Alpha##ESPBuildings", &CFG::ESP_Buildings_Alpha, 0.1f, 1.0f, "%.1f");

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##ESPBuildings", &CFG::ESP_Buildings_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##ESPBuildings", &CFG::ESP_Buildings_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##ESPBuildings", &CFG::ESP_Buildings_Ignore_Teammates);

        // Draw
        ImGui::Text("Draw:");
        ImGui::Checkbox("Name##ESPBuildings", &CFG::ESP_Buildings_Name);
        ImGui::SameLine();
        ImGui::Checkbox("Health##ESPBuildings", &CFG::ESP_Buildings_Health);
        ImGui::SameLine();
        ImGui::Checkbox("Health Bar##ESPBuildings", &CFG::ESP_Buildings_HealthBar);
        ImGui::SameLine();
        ImGui::Checkbox("Level", &CFG::ESP_Buildings_Level);
        ImGui::SameLine();
        ImGui::Checkbox("Level Bar", &CFG::ESP_Buildings_LevelBar);
        ImGui::Checkbox("Box##ESPBuildings", &CFG::ESP_Buildings_Box);
        ImGui::SameLine();
        ImGui::Checkbox("Tracer##ESPBuildings", &CFG::ESP_Buildings_Tracer);
        ImGui::SameLine();
        ImGui::Checkbox("Conds##ESPBuildings", &CFG::ESP_Buildings_Conds);

        ImGui::Checkbox("Show Team Dispensers##ESPBuildings", &CFG::ESP_Buildings_Show_Teammate_Dispensers);
    }
}

void CMenu::RenderRadarTab()
{
    // Global
    if (ImGui::CollapsingHeader("Global##Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##Radar", &CFG::Radar_Active);

        // Style
        const char* styles[] = { "Rectangle", "Circle" };
        ImGui::Combo("Style##Radar", &CFG::Radar_Style, styles, IM_ARRAYSIZE(styles));

        ImGui::SliderInt("Size", &CFG::Radar_Size, 100, 1000);
        ImGui::SliderInt("Icon Size", &CFG::Radar_Icon_Size, 18, 36);
        ImGui::SliderFloat("Radius", &CFG::Radar_Radius, 100.0f, 3000.0f, "%.0f");
        ImGui::SliderFloat("Cross Alpha", &CFG::Radar_Cross_Alpha, 0.0f, 1.0f, "%.1f");
        ImGui::SliderFloat("Outline Alpha##Radar", &CFG::Radar_Outline_Alpha, 0.0f, 1.0f, "%.1f");
        ImGui::SliderFloat("Background Alpha##Radar", &CFG::Radar_Background_Alpha, 0.0f, 1.0f, "%.1f");
    }

    // Players
    if (ImGui::CollapsingHeader("Players##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##RadarPlayers", &CFG::Radar_Players_Active);

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##RadarPlayers", &CFG::Radar_Players_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Friends##RadarPlayers", &CFG::Radar_Players_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##RadarPlayers", &CFG::Radar_Players_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##RadarPlayers", &CFG::Radar_Players_Ignore_Teammates);
        ImGui::SameLine();
        ImGui::Checkbox("Invisible##RadarPlayers", &CFG::Radar_Players_Ignore_Invisible);

        ImGui::Checkbox("Show Team Medics##RadarPlayers", &CFG::Radar_Players_Show_Teammate_Medics);
    }

    // Buildings
    if (ImGui::CollapsingHeader("Buildings##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##RadarBuildings", &CFG::Radar_Buildings_Active);

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##RadarBuildings", &CFG::Radar_Buildings_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##RadarBuildings", &CFG::Radar_Buildings_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##RadarBuildings", &CFG::Radar_Buildings_Ignore_Teammates);

        ImGui::Checkbox("Show Team Dispensers##RadarBuildings", &CFG::Radar_Buildings_Show_Teammate_Dispensers);
    }

    // World
    if (ImGui::CollapsingHeader("World##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##RadarWorld", &CFG::Radar_World_Active);

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Health Packs##RadarWorld", &CFG::Radar_World_Ignore_HealthPacks);
        ImGui::SameLine();
        ImGui::Checkbox("Ammo Packs##RadarWorld", &CFG::Radar_World_Ignore_AmmoPacks);
        ImGui::SameLine();
        ImGui::Checkbox("Halloween Gifts##RadarWorld", &CFG::Radar_World_Ignore_Halloween_Gift);
        ImGui::SameLine();
        ImGui::Checkbox("MVM Money##RadarWorld", &CFG::Radar_World_Ignore_MVM_Money);
    }
}

void CMenu::RenderMaterialsTab()
{
    // Global
    if (ImGui::CollapsingHeader("Global##Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##Materials", &CFG::Materials_Active);
    }

    // World
    if (ImGui::CollapsingHeader("World##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##MaterialsWorld", &CFG::Materials_World_Active);
        ImGui::Checkbox("No Depth##MaterialsWorld", &CFG::Materials_World_No_Depth);
        ImGui::SliderFloat("Alpha##MaterialsWorld", &CFG::Materials_World_Alpha, 0.0f, 1.0f, "%.1f");

        // Material
        const char* materials[] = { "Original", "Flat", "Shaded", "Glossy", "Glow", "Plastic" };
        ImGui::Combo("Material##MaterialsWorld", &CFG::Materials_World_Material, materials, IM_ARRAYSIZE(materials));

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Health Packs##MaterialsWorld", &CFG::Materials_World_Ignore_HealthPacks);
        ImGui::SameLine();
        ImGui::Checkbox("Ammo Packs##MaterialsWorld", &CFG::Materials_World_Ignore_AmmoPacks);
        ImGui::Checkbox("Local Projectiles##MaterialsWorld", &CFG::Materials_World_Ignore_LocalProjectiles);
        ImGui::SameLine();
        ImGui::Checkbox("Enemy Projectiles##MaterialsWorld", &CFG::Materials_World_Ignore_EnemyProjectiles);
        ImGui::SameLine();
        ImGui::Checkbox("Teammate Projectiles##MaterialsWorld", &CFG::Materials_World_Ignore_TeammateProjectiles);
        ImGui::Checkbox("Halloween Gifts##MaterialsWorld", &CFG::Materials_World_Ignore_Halloween_Gift);
        ImGui::SameLine();
        ImGui::Checkbox("MVM Money##MaterialsWorld", &CFG::Materials_World_Ignore_MVM_Money);
    }

    // View Model
    if (ImGui::CollapsingHeader("View Model", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##MaterialsViewModel", &CFG::Materials_ViewModel_Active);
        ImGui::SliderFloat("Hands Alpha", &CFG::Materials_ViewModel_Hands_Alpha, 0.0f, 1.0f, "%.1f");

        // Hands Material
        const char* materials[] = { "Original", "Flat", "Shaded", "Glossy", "Glow", "Plastic" };
        ImGui::Combo("Hands Material", &CFG::Materials_ViewModel_Hands_Material, materials, IM_ARRAYSIZE(materials));

        ImGui::SliderFloat("Weapon Alpha", &CFG::Materials_ViewModel_Weapon_Alpha, 0.0f, 1.0f, "%.1f");

        // Weapon Material
        ImGui::Combo("Weapon Material", &CFG::Materials_ViewModel_Weapon_Material, materials, IM_ARRAYSIZE(materials));
    }

    // Players
    if (ImGui::CollapsingHeader("Players##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##MaterialsPlayers", &CFG::Materials_Players_Active);
        ImGui::Checkbox("No Depth##MaterialsPlayers", &CFG::Materials_Players_No_Depth);
        ImGui::SliderFloat("Alpha##MaterialsPlayers", &CFG::Materials_Players_Alpha, 0.0f, 1.0f, "%.1f");

        // Material
        const char* materials[] = { "Original", "Flat", "Shaded", "Glossy", "Glow", "Plastic" };
        ImGui::Combo("Material##MaterialsPlayers", &CFG::Materials_Players_Material, materials, IM_ARRAYSIZE(materials));

        // Lag Records Style
        const char* lagRecordStyles[] = { "All", "Last Only" };
        ImGui::Combo("Lag Records Style", &CFG::Materials_Players_LagRecords_Style, lagRecordStyles, IM_ARRAYSIZE(lagRecordStyles));

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##MaterialsPlayers", &CFG::Materials_Players_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Friends##MaterialsPlayers", &CFG::Materials_Players_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##MaterialsPlayers", &CFG::Materials_Players_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##MaterialsPlayers", &CFG::Materials_Players_Ignore_Teammates);
        ImGui::SameLine();
        ImGui::Checkbox("Lag Records", &CFG::Materials_Players_Ignore_LagRecords);

        ImGui::Checkbox("Show Team Medics##MaterialsPlayers", &CFG::Materials_Players_Show_Teammate_Medics);
    }

    // Buildings
    if (ImGui::CollapsingHeader("Buildings##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##MaterialsBuildings", &CFG::Materials_Buildings_Active);
        ImGui::Checkbox("No Depth##MaterialsBuildings", &CFG::Materials_Buildings_No_Depth);
        ImGui::SliderFloat("Alpha##MaterialsBuildings", &CFG::Materials_Buildings_Alpha, 0.0f, 1.0f, "%.1f");

        // Material
        const char* materials[] = { "Original", "Flat", "Shaded", "Glossy", "Glow", "Plastic" };
        ImGui::Combo("Material##MaterialsBuildings", &CFG::Materials_Buildings_Material, materials, IM_ARRAYSIZE(materials));

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##MaterialsBuildings", &CFG::Materials_Buildings_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##MaterialsBuildings", &CFG::Materials_Buildings_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##MaterialsBuildings", &CFG::Materials_Buildings_Ignore_Teammates);

        ImGui::Checkbox("Show Team Dispensers##MaterialsBuildings", &CFG::Materials_Buildings_Show_Teammate_Dispensers);
    }
}

void CMenu::RenderOutlinesTab()
{
    // Global
    if (ImGui::CollapsingHeader("Global##Aimbot", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##Outlines", &CFG::Outlines_Active);

        // Style
        const char* styles[] = { "Bloom", "Crisp", "Cartoony", "Cartoony Alt" };
        ImGui::Combo("Style##Outlines", &CFG::Outlines_Style, styles, IM_ARRAYSIZE(styles));

        ImGui::SliderInt("Bloom Amount##Outlines", &CFG::Outlines_Bloom_Amount, 1, 10);
    }

    // World
    if (ImGui::CollapsingHeader("World##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##OutlinesWorld", &CFG::Outlines_World_Active);
        ImGui::SliderFloat("Alpha##OutlinesWorld", &CFG::Outlines_World_Alpha, 0.0f, 1.0f, "%.1f");

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Health Packs##OutlinesWorld", &CFG::Outlines_World_Ignore_HealthPacks);
        ImGui::SameLine();
        ImGui::Checkbox("Ammo Packs##OutlinesWorld", &CFG::Outlines_World_Ignore_AmmoPacks);
        ImGui::Checkbox("Local Projectiles##OutlinesWorld", &CFG::Outlines_World_Ignore_LocalProjectiles);
        ImGui::SameLine();
        ImGui::Checkbox("Enemy Projectiles##OutlinesWorld", &CFG::Outlines_World_Ignore_EnemyProjectiles);
        ImGui::SameLine();
        ImGui::Checkbox("Teammate Projectiles##OutlinesWorld", &CFG::Outlines_World_Ignore_TeammateProjectiles);
        ImGui::Checkbox("Halloween Gifts##OutlinesWorld", &CFG::Outlines_World_Ignore_Halloween_Gift);
        ImGui::SameLine();
        ImGui::Checkbox("MVM Money##OutlinesWorld", &CFG::Outlines_World_Ignore_MVM_Money);
    }

    // Players
    if (ImGui::CollapsingHeader("Players##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##OutlinesPlayers", &CFG::Outlines_Players_Active);
        ImGui::SliderFloat("Alpha##OutlinesPlayers", &CFG::Outlines_Players_Alpha, 0.0f, 1.0f, "%.1f");

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##OutlinesPlayers", &CFG::Outlines_Players_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Friends##OutlinesPlayers", &CFG::Outlines_Players_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##OutlinesPlayers", &CFG::Outlines_Players_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##OutlinesPlayers", &CFG::Outlines_Players_Ignore_Teammates);

        ImGui::Checkbox("Show Team Medics##OutlinesPlayers", &CFG::Outlines_Players_Show_Teammate_Medics);
    }

    // Buildings
    if (ImGui::CollapsingHeader("Buildings##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##OutlinesBuildings", &CFG::Outlines_Buildings_Active);
        ImGui::SliderFloat("Alpha##OutlinesBuildings", &CFG::Outlines_Buildings_Alpha, 0.0f, 1.0f, "%.1f");

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Local##OutlinesBuildings", &CFG::Outlines_Buildings_Ignore_Local);
        ImGui::SameLine();
        ImGui::Checkbox("Enemies##OutlinesBuildings", &CFG::Outlines_Buildings_Ignore_Enemies);
        ImGui::SameLine();
        ImGui::Checkbox("Teammates##OutlinesBuildings", &CFG::Outlines_Buildings_Ignore_Teammates);

        ImGui::Checkbox("Show Team Dispensers##OutlinesBuildings", &CFG::Outlines_Buildings_Show_Teammate_Dispensers);
    }
}

void CMenu::RenderOtherTab()
{
    // Local
    if (ImGui::CollapsingHeader("Local", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Aimbot FOV Circle", &CFG::Visuals_Aimbot_FOV_Circle);
        ImGui::SliderFloat("FOV Circle Alpha", &CFG::Visuals_Aimbot_FOV_Circle_Alpha, 0.01f, 1.0f, "%.2f");
        ImGui::Checkbox("Draw Projectile Arc", &CFG::Visuals_Draw_Projectile_Arc);
        ImGui::Checkbox("Reveal Scoreboard", &CFG::Visuals_Reveal_Scoreboard);
        ImGui::Checkbox("Clean Screenshot", &CFG::Misc_Clean_Screenshot);
        ImGui::SliderFloat("FOV Override", &CFG::Visuals_FOV_Override, 70.0f, 170.0f, "%.0f");

        // Removals
        ImGui::Text("Removals:");
        ImGui::Checkbox("Scope", &CFG::Visuals_Remove_Scope);
        ImGui::SameLine();
        ImGui::Checkbox("Zoom", &CFG::Visuals_Remove_Zoom);
        ImGui::SameLine();
        ImGui::Checkbox("Punch", &CFG::Visuals_Remove_Punch);
        ImGui::Checkbox("Screen Overlay", &CFG::Visuals_Remove_Screen_Overlay);
        ImGui::SameLine();
        ImGui::Checkbox("Screen Shake", &CFG::Visuals_Remove_Screen_Shake);
        ImGui::SameLine();
        ImGui::Checkbox("Screen Fade", &CFG::Visuals_Remove_Screen_Fade);

        // Removals Mode
        const char* removalsModes[] = { "Everyone", "Local Only" };
        ImGui::Combo("Removals Mode", &CFG::Visuals_Removals_Mode, removalsModes, IM_ARRAYSIZE(removalsModes));

        // Tracer Effect
        const char* tracerEffects[] = { "Default", "C.A.P.P.E.R", "Machina (White)", "Machina (Team)",
                                      "Big Nasty", "Short Circuit", "Mrasmus Zap", "Random", "Random (No Zap)" };
        ImGui::Combo("Tracer Effect", &CFG::Visuals_Tracer_Type, tracerEffects, IM_ARRAYSIZE(tracerEffects));

        // Projectile Arc Color Mode
        const char* arcColorModes[] = { "Custom", "Rainbow" };
        ImGui::Combo("Projectile Arc Color Mode", &CFG::Visuals_Draw_Projectile_Arc_Color_Mode, arcColorModes, IM_ARRAYSIZE(arcColorModes));

        // Movement Path Style
        const char* pathStyles[] = { "Disabled", "Line", "Dashed Line", "Alt Line" };
        ImGui::Combo("Movement Path Style", &CFG::Visuals_Draw_Movement_Path_Style, pathStyles, IM_ARRAYSIZE(pathStyles));
    }

    // Chat
    if (ImGui::CollapsingHeader("Chat", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Teammate Votes", &CFG::Visuals_Chat_Teammate_Votes);
        ImGui::Checkbox("Enemy Votes", &CFG::Visuals_Chat_Enemy_Votes);
        ImGui::Checkbox("Player List Info", &CFG::Visuals_Chat_Player_List_Info);
        ImGui::Checkbox("Name Tags", &CFG::Visuals_Chat_Name_Tags);
    }

    // World
    if (ImGui::CollapsingHeader("World##ESP", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Flat Textures", &CFG::Visuals_Flat_Textures);
        ImGui::Checkbox("Disable Fog", &CFG::Visuals_Remove_Fog);
        ImGui::Checkbox("Disable Sky Fog", &CFG::Visuals_Remove_Sky_Fog);
        ImGui::Checkbox("Distance Prop Alpha", &CFG::Visuals_Distance_Prop_Alpha);
        ImGui::Checkbox("Don't Modulate Sky", &CFG::Visuals_World_Modulation_No_Sky_Change);

        // World Modulation Mode
        const char* worldModModes[] = { "Night Mode", "Custom Color" };
        ImGui::Combo("World Modulation Mode", &CFG::Visuals_World_Modulation_Mode, worldModModes, IM_ARRAYSIZE(worldModModes));

        ImGui::SliderFloat("Night Mode", &CFG::Visuals_Night_Mode, 0.0f, 100.0f, "%.0f");

        // Particles Mode
        const char* particleModes[] = { "Original", "Custom Color", "Rainbow" };
        ImGui::Combo("Particles Mode", &CFG::Visuals_Particles_Mode, particleModes, IM_ARRAYSIZE(particleModes));

        ImGui::SliderFloat("Particles Rainbow Rate", &CFG::Visuals_Particles_Rainbow_Rate, 1.0f, 10.0f, "%.0f");
    }

    // Spectator List
    if (ImGui::CollapsingHeader("Spectator List", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsSpectatorList", &CFG::Visuals_SpectatorList_Active);
        ImGui::SliderFloat("Outline Alpha##SpectatorList", &CFG::Visuals_SpectatorList_Outline_Alpha, 0.1f, 1.0f, "%.1f");
        ImGui::SliderFloat("Background Alpha##SpectatorList", &CFG::Visuals_SpectatorList_Background_Alpha, 0.1f, 1.0f, "%.1f");
        ImGui::SliderInt("Width##SpectatorList", &CFG::Visuals_SpectatorList_Width, 200, 1000);
    }

    // Thirdperson
    if (ImGui::CollapsingHeader("Thirdperson", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsThirdperson", &CFG::Visuals_Thirdperson_Active);
        InputKey("Toggle Key", CFG::Visuals_Thirdperson_Key);
        ImGui::SliderFloat("Offset Forward##Thirdperson", &CFG::Visuals_Thirdperson_Offset_Forward, 10.0f, 200.0f, "%.0f");
        ImGui::SliderFloat("Offset Right##Thirdperson", &CFG::Visuals_Thirdperson_Offset_Right, -50.0f, 50.0f, "%.0f");
        ImGui::SliderFloat("Offset Up##Thirdperson", &CFG::Visuals_Thirdperson_Offset_Up, -50.0f, 50.0f, "%.0f");
    }

    // View Model
    if (ImGui::CollapsingHeader("View Model", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsViewModel", &CFG::Visuals_ViewModel_Active);
        ImGui::Checkbox("Sway", &CFG::Visuals_ViewModel_Sway);
        ImGui::SliderFloat("Sway Scale", &CFG::Visuals_ViewModel_Sway_Scale, 0.1f, 1.0f, "%.1f");
        ImGui::SliderFloat("Offset Forward##ViewModel", &CFG::Visuals_ViewModel_Offset_Forward, -50.00f, 50.0f, "%.0f");
        ImGui::SliderFloat("Offset Right##ViewModel", &CFG::Visuals_ViewModel_Offset_Right, -50.0f, 50.0f, "%.0f");
        ImGui::SliderFloat("Offset Up##ViewModel", &CFG::Visuals_ViewModel_Offset_Up, -50.0f, 50.0f, "%.0f");
    }
}

void CMenu::RenderOther2Tab()
{
    // Performance
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Disable Detail Props", &CFG::Visuals_Disable_Detail_Props);
        ImGui::Checkbox("Disable Ragdolls", &CFG::Visuals_Disable_Ragdolls);
        ImGui::Checkbox("Disable Wearables", &CFG::Visuals_Disable_Wearables);
        ImGui::Checkbox("Disable Post Processing", &CFG::Visuals_Disable_Post_Processing);
        ImGui::Checkbox("Disable Dropped Weapons", &CFG::Visuals_Disable_Dropped_Weapons);
        ImGui::Checkbox("Use Simple Models", &CFG::Visuals_Simple_Models);
    }

    // Paint
    if (ImGui::CollapsingHeader("Paint", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsPaint", &CFG::Visuals_Paint_Active);
        InputKey("Key", CFG::Visuals_Paint_Key);
        InputKey("Erase Key", CFG::Visuals_Paint_Erase_Key);

        const char* pszFmt = CFG::Visuals_Paint_LifeTime <= 0.0f ? "inf" : "%.0fs";
        ImGui::SliderFloat("Life Time##Paint", &CFG::Visuals_Paint_LifeTime, 0.0f, 10.0f, pszFmt);
        ImGui::SliderInt("Bloom Amount##Paint", &CFG::Visuals_Paint_Bloom_Amount, 3, 10);
    }

    // Team Well-Being
    if (ImGui::CollapsingHeader("Team Well-Being", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsTeamWellBeing", &CFG::Visuals_TeamWellBeing_Active);
        ImGui::Checkbox("Medic Only", &CFG::Visuals_TeamWellBeing_Medic_Only);
        ImGui::SliderFloat("Background Alpha##TeamWellBeing", &CFG::Visuals_TeamWellBeing_Background_Alpha, 0.1f, 1.0f, "%.1f");
        ImGui::SliderInt("Width##TeamWellBeing", &CFG::Visuals_TeamWellBeing_Width, 200, 1000);
    }

    // Spy Camera
    if (ImGui::CollapsingHeader("Spy Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsSpyCamera", &CFG::Visuals_SpyCamera_Active);
        ImGui::SliderFloat("Background Alpha##SpyCamera", &CFG::Visuals_SpyCamera_Background_Alpha, 0.1f, 1.0f, "%.1f");
        ImGui::SliderInt("Camera Width", &CFG::Visuals_SpyCamera_Pos_W, 100, 600);
        ImGui::SliderInt("Camera Height", &CFG::Visuals_SpyCamera_Pos_H, 100, 600);
        ImGui::SliderFloat("Camera FOV", &CFG::Visuals_SpyCamera_FOV, 70.0f, 170.0f, "%.0f");
    }

    // Spy Warning
    if (ImGui::CollapsingHeader("Spy Warning", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsSpyWarning", &CFG::Viuals_SpyWarning_Active);
        ImGui::Checkbox("Announce", &CFG::Viuals_SpyWarning_Announce);

        // Ignore
        ImGui::Text("Ignore:");
        ImGui::Checkbox("Cloaked##SpyWarning", &CFG::Viuals_SpyWarning_Ignore_Cloaked);
        ImGui::SameLine();
        ImGui::Checkbox("Friends##SpyWarning", &CFG::Viuals_SpyWarning_Ignore_Friends);
        ImGui::SameLine();
        ImGui::Checkbox("Invisible##SpyWarning", &CFG::Viuals_SpyWarning_Ignore_Invisible);
    }

    // Ragdolls
    if (ImGui::CollapsingHeader("Ragdolls", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsRagdolls", &CFG::Visuals_Ragdolls_Active);
        ImGui::Checkbox("No Gib", &CFG::Visuals_Ragdolls_No_Gib);
        ImGui::Checkbox("No Death Animation", &CFG::Visuals_Ragdolls_No_Death_Anim);

        // Effect
        const char* effects[] = { "Default", "Burning", "Electrocuted", "Ash", "Gold", "Ice", "Dissolve", "Random" };
        ImGui::Combo("Effect", &CFG::Visuals_Ragdolls_Effect, effects, IM_ARRAYSIZE(effects));

        ImGui::SliderFloat("Force Multiplier", &CFG::Visuals_Ragdolls_Force_Mult, 0.0f, 5.0f, "%.0f");
    }

    // Beams
    if (ImGui::CollapsingHeader("Beams", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Active##VisualsBeams", &CFG::Visuals_Beams_Active);
        ImGui::SliderFloat("Life Time##Beams", &CFG::Visuals_Beams_LifeTime, 1.0f, 10.0f, "%.0fs");
        ImGui::SliderFloat("Start Width", &CFG::Visuals_Beams_Width, 1.0f, 10.0f, "%.0f");
        ImGui::SliderFloat("End Width", &CFG::Visuals_Beams_EndWidth, 1.0f, 10.0f, "%.0f");
        ImGui::SliderFloat("Fade Length", &CFG::Visuals_Beams_FadeLength, 1.0f, 10.0f, "%.0f");
        ImGui::SliderFloat("Amplitude", &CFG::Visuals_Beams_Amplitude, 0.0f, 10.0f, "%.1f");
        ImGui::SliderFloat("Speed", &CFG::Visuals_Beams_Speed, 0.0f, 10.0f, "%.0f");

        // Flags
        ImGui::Text("Flags:");
        ImGui::Checkbox("FBEAM_FADEIN", &CFG::Visuals_Beams_Flag_FBEAM_FADEIN);
        ImGui::SameLine();
        ImGui::Checkbox("FBEAM_FADEOUT", &CFG::Visuals_Beams_Flag_FBEAM_FADEOUT);
        ImGui::SameLine();
        ImGui::Checkbox("FBEAM_SINENOISE", &CFG::Visuals_Beams_Flag_FBEAM_SINENOISE);
        ImGui::Checkbox("FBEAM_SOLID", &CFG::Visuals_Beams_Flag_FBEAM_SOLID);
        ImGui::SameLine();
        ImGui::Checkbox("FBEAM_SHADEIN", &CFG::Visuals_Beams_Flag_FBEAM_SHADEIN);
        ImGui::SameLine();
        ImGui::Checkbox("FBEAM_SHADEOUT", &CFG::Visuals_Beams_Flag_FBEAM_SHADEOUT);
    }
}

void CMenu::RenderColorsTab()
{
    // Menu Colors
    if (ImGui::CollapsingHeader("Menu", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Accent Primary", (float*)&CFG::Menu_Accent_Primary);
        ImGui::ColorEdit4("Accent Secondary", (float*)&CFG::Menu_Accent_Secondary);
        ImGui::ColorEdit4("Background", (float*)&CFG::Menu_Background);
        ImGui::Checkbox("Menu Snow", &CFG::Menu_Snow);
    }

    // Visual Colors
    if (ImGui::CollapsingHeader("Visuals", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Hands", (float*)&CFG::Color_Hands);
        ImGui::ColorEdit4("Hands Sheen", (float*)&CFG::Color_Hands_Sheen);
        ImGui::ColorEdit4("Weapon", (float*)&CFG::Color_Weapon);
        ImGui::ColorEdit4("Weapon Sheen", (float*)&CFG::Color_Weapon_Sheen);
        ImGui::ColorEdit4("Projectile Arc", (float*)&CFG::Color_Projectile_Arc);
    }

    // Entity Colors
    if (ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::ColorEdit4("Local", (float*)&CFG::Color_Local);
        ImGui::ColorEdit4("Friend", (float*)&CFG::Color_Friend);
        ImGui::ColorEdit4("Enemy", (float*)&CFG::Color_Enemy);
        ImGui::ColorEdit4("Teammate", (float*)&CFG::Color_Teammate);
        ImGui::ColorEdit4("Target", (float*)&CFG::Color_Target);
        ImGui::ColorEdit4("Invulnerable", (float*)&CFG::Color_Invulnerable);
        ImGui::ColorEdit4("Cheater", (float*)&CFG::Color_Cheater);
        ImGui::ColorEdit4("Retard Legit", (float*)&CFG::Color_RetardLegit);
        ImGui::ColorEdit4("Invisible", (float*)&CFG::Color_Invisible);
        ImGui::ColorEdit4("Over Heal", (float*)&CFG::Color_OverHeal);
        ImGui::ColorEdit4("Uber", (float*)&CFG::Color_Uber);
        ImGui::ColorEdit4("Conds", (float*)&CFG::Color_Conds);
        ImGui::ColorEdit4("Health Pack", (float*)&CFG::Color_HealthPack);
        ImGui::ColorEdit4("Ammo Pack", (float*)&CFG::Color_AmmoPack);
        ImGui::ColorEdit4("Beams", (float*)&CFG::Color_Beams);
        ImGui::ColorEdit4("Halloween Gifts", (float*)&CFG::Color_Halloween_Gift);
        ImGui::ColorEdit4("MVM Money", (float*)&CFG::Color_MVM_Money);
        ImGui::ColorEdit4("Particles", (float*)&CFG::Color_Particles);
        ImGui::ColorEdit4("World Modulation", (float*)&CFG::Color_World);
        ImGui::ColorEdit4("Sky Modulation", (float*)&CFG::Color_Sky);
        ImGui::ColorEdit4("Prop Modulation", (float*)&CFG::Color_Props);
    }
}

void CMenu::RenderMiscTab()
{
    if (ImGui::BeginTabItem("Misc"))
    {
        // Misc
        if (ImGui::CollapsingHeader("Misc", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Bunnyhop", &CFG::Misc_Bunnyhop);
            ImGui::Checkbox("Choke on Bunnyhop", &CFG::Misc_Choke_On_Bhop);
            ImGui::Checkbox("Bypass sv_pure", &CFG::Misc_Pure_Bypass);
            ImGui::Checkbox("Noise Maker Spam", &CFG::Misc_NoiseMaker_Spam);
            ImGui::Checkbox("No Push", &CFG::Misc_No_Push);
            ImGui::Checkbox("Giant Weapon Sounds", &CFG::Misc_MVM_Giant_Weapon_Sounds);
            ImGui::Checkbox("Equip Region Unlock", &CFG::Misc_Equip_Region_Unlock);
            ImGui::Checkbox("Fast Stop", &CFG::Misc_Fast_Stop);
            ImGui::Checkbox("Anti Server Angle Change", &CFG::Misc_Prevent_Server_Angle_Change);

            if (ImGui::Button("Unlock CVars"))
            {
                auto iter = ICvar::Iterator(I::CVar);
                for (iter.SetFirst(); iter.IsValid(); iter.Next())
                {
                    auto cmd = iter.Get();
                    if (!cmd) continue;

                    if (cmd->m_nFlags & FCVAR_DEVELOPMENTONLY)
                        cmd->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;

                    if (cmd->m_nFlags & FCVAR_HIDDEN)
                        cmd->m_nFlags &= ~FCVAR_HIDDEN;

                    if (cmd->m_nFlags & FCVAR_PROTECTED)
                        cmd->m_nFlags &= ~FCVAR_PROTECTED;

                    if (cmd->m_nFlags & FCVAR_CHEAT)
                        cmd->m_nFlags &= ~FCVAR_CHEAT;
                }
            }
        }

        // Game
        if (ImGui::CollapsingHeader("Game", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Network Fix", &CFG::Misc_Ping_Reducer);
            ImGui::Checkbox("Prediction Error Jitter Fix", &CFG::Misc_Pred_Error_Jitter_Fix);
            ImGui::Checkbox("ComputeLightingOrigin Fix", &CFG::Misc_ComputeLightingOrigin_Fix);
            ImGui::Checkbox("SetupBones Optimization", &CFG::Misc_SetupBones_Optimization);
            ImGui::Checkbox("Accuracy Improvements", &CFG::Misc_Accuracy_Improvements);
        }

        // Mann vs. Machine
        if (ImGui::CollapsingHeader("Mann vs. Machine", ImGuiTreeNodeFlags_DefaultOpen))
        {
            InputKey("Instant Respawn", CFG::Misc_MVM_Instant_Respawn_Key);
            ImGui::Checkbox("Instant Revive", &CFG::Misc_MVM_Instant_Revive);
        }

        // Chat
        if (ImGui::CollapsingHeader("Chat", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Medieval", &CFG::Misc_Chat_Medieval);
            ImGui::Checkbox("OwO-ify", &CFG::Misc_Chat_Owoify);
        }

        // Taunt
        if (ImGui::CollapsingHeader("Taunt", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Taunt Slide", &CFG::Misc_Taunt_Slide);
            ImGui::Checkbox("Taunt Control", &CFG::Misc_Taunt_Slide_Control);
            InputKey("Taunt Spin Key", CFG::Misc_Taunt_Spin_Key);
            ImGui::SliderFloat("Taunt Spin Speed", &CFG::Misc_Taunt_Spin_Speed, -50.0f, 50.0f, "%.0f");
            ImGui::Checkbox("Taunt Spin Sine", &CFG::Misc_Taunt_Spin_Sine);
            ImGui::Checkbox("Fake Taunt", &CFG::Misc_Fake_Taunt);
        }

        // Auto
        if (ImGui::CollapsingHeader("Auto", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Auto Disguise", &CFG::Misc_Auto_Disguise);
            ImGui::Checkbox("Auto Vaccinator", &CFG::AutoVaccinator_Active);

            // Auto Vaccinator Pop
            const char* vaccPopOptions[] = { "Everyone", "Friends Only" };
            ImGui::Combo("Auto Vaccinator Pop", &CFG::AutoVaccinator_Pop, vaccPopOptions, IM_ARRAYSIZE(vaccPopOptions));

            ImGui::Checkbox("Auto Strafe", &CFG::Misc_Auto_Strafe);
            ImGui::SliderFloat("Auto Strafe Turn Scale", &CFG::Misc_Auto_Strafe_Turn_Scale, 0.0f, 1.0f, "%.1f");

            InputKey("Auto RJ Key", CFG::Misc_Auto_Rocket_Jump_Key);
            InputKey("Auto AP Key", CFG::Misc_Auto_Air_Pogo_Key);
            InputKey("Auto Heal Key", CFG::Misc_Auto_Medigun_Key);
            InputKey("Undo Glue Key", CFG::Misc_Movement_Lock_Key);
            InputKey("Edge Jump Key", CFG::Misc_Edge_Jump_Key);
        }

        // Shifting
        if (ImGui::CollapsingHeader("Shifting", ImGuiTreeNodeFlags_DefaultOpen))
        {
            InputKey("Recharge Key", CFG::Exploits_Shifting_Recharge_Key);
            InputKey("Rapid Fire Key", CFG::Exploits_RapidFire_Key);
            ImGui::SliderInt("Rapid Fire Ticks", &CFG::Exploits_RapidFire_Ticks, 14, MAX_COMMANDS);
            ImGui::SliderInt("Rapid Fire Delay Ticks", &CFG::Exploits_RapidFire_Min_Ticks_Target_Same, 0, 5);
            ImGui::Checkbox("Rapid Fire Antiwarp", &CFG::Exploits_RapidFire_Antiwarp);
            InputKey("Warp Key", CFG::Exploits_Warp_Key);

            // Warp Mode
            const char* warpModes[] = { "Slow", "Full" };
            ImGui::Combo("Warp Mode", &CFG::Exploits_Warp_Mode, warpModes, IM_ARRAYSIZE(warpModes));

            // Warp Exploit
            const char* warpExploits[] = { "None", "Fake Peek", "0 Velocity" };
            ImGui::Combo("Warp Exploit (for 'Full')", &CFG::Exploits_Warp_Exploit, warpExploits, IM_ARRAYSIZE(warpExploits));

            ImGui::Checkbox("Draw Indicator##Shifting", &CFG::Exploits_Shifting_Draw_Indicator);

            if (CFG::Exploits_Shifting_Draw_Indicator)
            {
                // Indicator Style
                const char* indicatorStyles[] = { "Rectangle", "Circle" };
                ImGui::Combo("Indicator Style", &CFG::Exploits_Shifting_Indicator_Style, indicatorStyles, IM_ARRAYSIZE(indicatorStyles));
            }
        }

        // Crits
        if (ImGui::CollapsingHeader("Crits", ImGuiTreeNodeFlags_DefaultOpen))
        {
            InputKey("Key", CFG::Exploits_Crits_Force_Crit_Key);
            InputKey("Melee Key", CFG::Exploits_Crits_Force_Crit_Key_Melee);
            ImGui::Checkbox("Skip Random Crits", &CFG::Exploits_Crits_Skip_Random_Crits);
        }

        // Seed Pred
        if (ImGui::CollapsingHeader("Seed Pred", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Active##ExploitsSeedPred", &CFG::Exploits_SeedPred_Active);
            ImGui::Checkbox("Draw Indicator##SeedPred", &CFG::Exploits_SeedPred_DrawIndicator);
        }

        ImGui::EndTabItem();
    }
}

void CMenu::RenderPlayersTab()
{
    if (ImGui::BeginTabItem("Players"))
    {
        if (I::EngineClient->IsConnected())
        {
            if (ImGui::BeginTable("PlayerList", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("Player");
                ImGui::TableSetupColumn("Ignored");
                ImGui::TableSetupColumn("Cheater");
                ImGui::TableSetupColumn("Retard Legit");
                ImGui::TableHeadersRow();

                for (auto n{ 1 }; n < I::EngineClient->GetMaxClients() + 1; n++)
                {
                    if (n == I::EngineClient->GetLocalPlayer())
                        continue;

                    player_info_t player_info{};
                    if (!I::EngineClient->GetPlayerInfo(n, &player_info) || player_info.fakeplayer)
                        continue;

                    PlayerPriority custom_info{};
                    F::Players->GetInfo(n, custom_info);

                    ImGui::TableNextRow();

                    // Player name
                    ImGui::TableSetColumnIndex(0);

                    ImVec4 nameColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    if (custom_info.Ignored)
                        nameColor = ImVec4(CFG::Color_Friend.r / 255.0f, CFG::Color_Friend.g / 255.0f, CFG::Color_Friend.b / 255.0f, 1.0f);
                    else if (custom_info.Cheater)
                        nameColor = ImVec4(CFG::Color_Cheater.r / 255.0f, CFG::Color_Cheater.g / 255.0f, CFG::Color_Cheater.b / 255.0f, 1.0f);
                    else if (custom_info.RetardLegit)
                        nameColor = ImVec4(CFG::Color_RetardLegit.r / 255.0f, CFG::Color_RetardLegit.g / 255.0f, CFG::Color_RetardLegit.b / 255.0f, 1.0f);

                    ImGui::TextColored(nameColor, "%s", player_info.name);

                    // Ignored checkbox
                    ImGui::TableSetColumnIndex(1);
                    bool ignored = custom_info.Ignored;
                    if (ImGui::Checkbox(("##ignored_" + std::to_string(n)).c_str(), &ignored))
                    {
                        F::Players->Mark(n, { ignored, false });
                    }

                    // Cheater checkbox
                    ImGui::TableSetColumnIndex(2);
                    bool cheater = custom_info.Cheater;
                    if (ImGui::Checkbox(("##cheater_" + std::to_string(n)).c_str(), &cheater))
                    {
                        F::Players->Mark(n, { false, cheater });
                    }

                    // Retard Legit checkbox
                    ImGui::TableSetColumnIndex(3);
                    bool retardLegit = custom_info.RetardLegit;
                    if (ImGui::Checkbox(("##retard_" + std::to_string(n)).c_str(), &retardLegit))
                    {
                        F::Players->Mark(n, { false, false, retardLegit });
                    }
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::Text("Not connected to a server");
        }

        ImGui::EndTabItem();
    }
}

void CMenu::RenderConfigsTab()
{
    if (ImGui::BeginTabItem("Configs"))
    {
        static std::string strSelected = {};
        static std::string strInput = {};
        const auto& configFolder = U::Storage->GetConfigFolder();

        int nCount = 0;
        for (const auto &entry : std::filesystem::directory_iterator(configFolder))
        {
            if (std::string(std::filesystem::path(entry).filename().string()).find(".json") == std::string_view::npos)
                continue;
            nCount++;
        }

        if (nCount < 11)
        {
            if (ImGui::CollapsingHeader("Create New", ImGuiTreeNodeFlags_DefaultOpen))
            {
                char inputBuffer[256] = {};
                strncpy_s(inputBuffer, sizeof(inputBuffer), strInput.c_str(), _TRUNCATE);

                if (ImGui::InputText("Config Name", inputBuffer, sizeof(inputBuffer)))
                {
                    strInput = inputBuffer;
                }

                ImGui::SameLine();
                if (ImGui::Button("Create") && !strInput.empty())
                {
                    bool bAlreadyExists = false;
                    for (const auto &entry : std::filesystem::directory_iterator(configFolder))
                    {
                        if (std::string(std::filesystem::path(entry).filename().string()).find(".json") == std::string_view::npos)
                            continue;

                        if (!std::string(std::filesystem::path(entry).filename().string()).compare(strInput))
                        {
                            bAlreadyExists = true;
                            break;
                        }
                    }

                    if (!bAlreadyExists)
                    {
                        std::string newFile = strInput + ".json";
                        Config::Save(configFolder / newFile);
                        strInput.clear();
                    }
                }
            }
        }

        if (strSelected.empty())
        {
            if (nCount > 0)
            {
                if (ImGui::CollapsingHeader("Configs", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (const auto &entry : std::filesystem::directory_iterator(configFolder))
                    {
                        if (std::string(std::filesystem::path(entry).filename().string()).find(".json") == std::string_view::npos)
                            continue;

                        std::string s = entry.path().filename().string();
                        s.erase(s.end() - 5, s.end()); // Remove .json

                        if (ImGui::Button(s.c_str()))
                            strSelected = s;
                    }
                }
            }
        }
        else
        {
            if (ImGui::CollapsingHeader(strSelected.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Button("Load"))
                {
                    std::string fileName = strSelected + ".json";
                    Config::Load(configFolder / fileName);
                    strSelected.clear();
                }

                ImGui::SameLine();
                if (ImGui::Button("Update"))
                {
                    std::string fileName = strSelected + ".json";
                    Config::Save(configFolder / fileName);
                    strSelected.clear();
                }

                ImGui::SameLine();
                if (ImGui::Button("Delete"))
                {
                    std::string fileName = strSelected + ".json";
                    std::filesystem::remove(configFolder / fileName);
                    strSelected.clear();
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                    strSelected.clear();
            }
        }

        ImGui::EndTabItem();
    }
}

void CMenu::Run()
{
    // Handle input and menu toggle logic
    if (CFG::Misc_Clean_Screenshot && I::EngineClient->IsTakingScreenshot())
    {
        return;
    }

    if (!H::Input->IsGameFocused() && m_bOpen)
    {
        m_bOpen = false;

        // Menu closed due to focus loss - hide our cursor, show system cursor
        ImGui::GetIO().MouseDrawCursor = false;

        // Force show system cursor (handle reference counting)
        while (::ShowCursor(TRUE) < 0) {}  // Keep calling until cursor is visible

        // Reset input state when closing to prevent stuck keys
        if (I::InputSystem) {
            I::InputSystem->ResetInputState();
        }
        return;
    }

    if (H::Input->IsPressed(VK_INSERT) || H::Input->IsPressed(VK_F3))
    {
        m_bOpen = !m_bOpen;

        // Handle cursor visibility when menu state changes
        // Input system control is now handled in WndProc (like GOESP)
        if (m_bOpen) {
            // Menu opened - show our cursor, hide system cursor
            ImGui::GetIO().MouseDrawCursor = true;
            ::ShowCursor(FALSE);  // Hide Windows system cursor

            // Reset input state to clear any pressed keys (prevent stuck movement)
            if (I::InputSystem) {
                I::InputSystem->ResetInputState();
            }
        } else {
            // Menu closed - hide our cursor, show system cursor
            ImGui::GetIO().MouseDrawCursor = false;

            // Force show system cursor (handle reference counting)
            while (::ShowCursor(TRUE) < 0) {}  // Keep calling until cursor is visible

            // Reset input state when closing to prevent stuck keys
            if (I::InputSystem) {
                I::InputSystem->ResetInputState();
            }
        }
    }
}

void CMenu::RenderImguiFrame()
{
    // Multiple safety checks to prevent crashes
    if (!m_bInitialized || !m_bOpen)
        return;

    // Additional checks for ImGui context
    if (!m_bImGuiContextCreated || !ImGui::GetCurrentContext())
        return;

    // Ensure our cursor is visible when menu is open
    ImGui::GetIO().MouseDrawCursor = true;
    // Note: System cursor hiding is handled in menu toggle, not here to avoid reference count issues

    try {
        // ImGui frame already started in basicHook.cpp to avoid conflicts
        // ImGui_ImplDX9_NewFrame();
        // ImGui_ImplWin32_NewFrame();
        // ImGui::NewFrame();

        // Set window size and position
        if (m_vWindowSize && m_vWindowPos)
        {
            ImGui::SetNextWindowSize(*m_vWindowSize, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(*m_vWindowPos, ImGuiCond_FirstUseEver);
        }

        // Main window
        if (ImGui::Begin("SEOwnedDE", &m_bOpen, ImGuiWindowFlags_MenuBar))
        {
            // Update CFG values with current window position/size
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            CFG::Menu_Pos_X = static_cast<int>(pos.x);
            CFG::Menu_Pos_Y = static_cast<int>(pos.y);
            CFG::Menu_Width = static_cast<int>(size.x);
            CFG::Menu_Height = static_cast<int>(size.y);

            // Update static references
            if (m_vWindowSize)
            {
                m_vWindowSize->x = static_cast<float>(CFG::Menu_Width);
                m_vWindowSize->y = static_cast<float>(CFG::Menu_Height);
            }
            if (m_vWindowPos)
            {
                m_vWindowPos->x = static_cast<float>(CFG::Menu_Pos_X);
                m_vWindowPos->y = static_cast<float>(CFG::Menu_Pos_Y);
            }

            // Tab bar
            if (ImGui::BeginTabBar("MainTabs"))
            {
                RenderAimTab();
                RenderVisualsTab();
                RenderMiscTab();
                RenderPlayersTab();
                RenderConfigsTab();

                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // Frame ending and rendering is handled in basicHook.cpp to prevent conflicts
        // This avoids double ImGui::Render() calls that cause flickering
    }
    catch (...) {
        // If any ImGui operation fails, close the menu to prevent repeated crashes
        m_bOpen = false;
    }
}

CMenu::CMenu()
{
    m_bInitialized = false;
    m_bImGuiContextCreated = false;
    m_vWindowSize = nullptr;
    m_vWindowPos = nullptr;
    m_pDevice = nullptr;
}

CMenu::~CMenu()
{
    Shutdown();
}