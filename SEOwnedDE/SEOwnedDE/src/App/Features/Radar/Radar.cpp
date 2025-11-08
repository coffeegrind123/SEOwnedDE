#include "Radar.h"

#include "../CFG.h"
#include "../Menu/Menu.h"
#include "../VisualUtils/VisualUtils.h"

void CRadar::UpdateViewAngleCache()
{
	const int currentFrame = I::GlobalVars->framecount;
	if (m_nLastViewAngleCacheFrame == currentFrame)
		return;

	m_nLastViewAngleCacheFrame = currentFrame;
	m_cachedViewAngles = I::EngineClient->GetViewAngles();
}

void CRadar::Drag()
{
	const int nMouseX = H::Input->GetMouseX();
	const int nMouseY = H::Input->GetMouseY();
	const int nRadarSize = CFG::Radar_Size;

	bool bHovered = false;
	static bool bDragging = false;

	if (!bDragging && F::Menu->IsMenuWindowHovered())
		return;

	static int nDeltaX = 0;
	static int nDeltaY = 0;

	switch (CFG::Radar_Style)
	{
		case 0:
		{
			const int nRadarX = CFG::Radar_Pos_X;
			const int nRadarY = CFG::Radar_Pos_Y;

			bHovered = nMouseX > nRadarX && nMouseX < nRadarX + nRadarSize && nMouseY > nRadarY && nMouseY < nRadarY + nRadarSize;

			if (bHovered && H::Input->IsPressed(VK_LBUTTON))
			{
				nDeltaX = nMouseX - nRadarX;
				nDeltaY = nMouseY - nRadarY;
				bDragging = true;
			}

			break;
		}

		case 1:
		{
			const int nRadarX = CFG::Radar_Pos_X + nRadarSize / 2;
			const int nRadarY = CFG::Radar_Pos_Y + nRadarSize / 2;

			const auto vRadar = Vec2(static_cast<float>(nRadarX), static_cast<float>(nRadarY));
			const auto vMouse = Vec2(static_cast<float>(nMouseX), static_cast<float>(nMouseY));

			bHovered = static_cast<int>(vRadar.DistTo(vMouse)) < (CFG::Radar_Size / 2);

			if (bHovered && H::Input->IsPressed(VK_LBUTTON))
			{
				nDeltaX = nMouseX - CFG::Radar_Pos_X;
				nDeltaY = nMouseY - CFG::Radar_Pos_Y;
				bDragging = true;
			}

			break;
		}

		default: return;
	}

	if (!H::Input->IsPressed(VK_LBUTTON) && !H::Input->IsHeld(VK_LBUTTON))
		bDragging = false;

	if (bDragging)
	{
		CFG::Radar_Pos_X = nMouseX - nDeltaX;
		CFG::Radar_Pos_Y = nMouseY - nDeltaY;
	}
}

bool CRadar::GetDrawPosition(int& x, int& y, const Vec3& vWorld)
{
	const auto pLocal = H::Entities->GetLocal();

	if (!pLocal)
		return false;

	// Use cached data to prevent interpolation flicker when spectating
	const auto pLocalCached = H::Entities->GetCachedData(pLocal);
	if (!pLocalCached)
		return false;

	const int nRadarX = CFG::Radar_Pos_X + (CFG::Radar_Size / 2);
	const int nRadarY = CFG::Radar_Pos_Y + (CFG::Radar_Size / 2);

	const float flYaw = m_cachedViewAngles.y * (static_cast<float>(PI) / 180.0f);
	const float flRadius = CFG::Radar_Radius;
	const float flCos = std::cosf(flYaw);
	const float flSin = std::sinf(flYaw);

	const Vec3 vDelta = vWorld - pLocalCached->renderOrigin;
	Vec2 vPos = { (vDelta.y * (-flCos) + vDelta.x * flSin), (vDelta.x * (-flCos) - vDelta.y * flSin) };

	switch (CFG::Radar_Style)
	{
		// Rectangle
		case 0:
		{
			if (fabsf(vPos.x) > flRadius || fabsf(vPos.y) > flRadius)
			{
				if (vPos.y > vPos.x)
				{
					if (vPos.y > -vPos.x)
					{
						vPos.x = flRadius * vPos.x / vPos.y;
						vPos.y = flRadius;
					}

					else
					{
						vPos.y = -flRadius * vPos.y / vPos.x;
						vPos.x = -flRadius;
					}
				}

				else
				{
					if (vPos.y > -vPos.x)
					{
						vPos.y = flRadius * vPos.y / vPos.x;
						vPos.x = flRadius;
					}

					else
					{
						vPos.x = -flRadius * vPos.x / vPos.y;
						vPos.y = -flRadius;
					}
				}
			}

			x = nRadarX + static_cast<int>(vPos.x / flRadius * static_cast<float>(CFG::Radar_Size / 2));
			y = nRadarY + static_cast<int>(vPos.y / flRadius * static_cast<float>(CFG::Radar_Size / 2));

			break;
		}

		// Circle
		case 1:
		{
			const int nPosX = nRadarX + static_cast<int>(vPos.x / flRadius * static_cast<float>(CFG::Radar_Size / 2));
			const int nPosY = nRadarY + static_cast<int>(vPos.y / flRadius * static_cast<float>(CFG::Radar_Size / 2));

			const Vec2 vRadar = { static_cast<float>(nRadarX), static_cast<float>(nRadarY) };
			Vec2 vPoint = { static_cast<float>(nPosX), static_cast<float>(nPosY) };

			Vec2 vDelta = vPoint - vRadar;

			if (static_cast<int>(vDelta.Length()) > CFG::Radar_Size / 2)
			{
				vDelta *= static_cast<float>(CFG::Radar_Size / 2) / vDelta.Length();
				vPoint = vRadar + vDelta;

				x = static_cast<int>(vPoint.x);
				y = static_cast<int>(vPoint.y);
			}

			else
			{
				x = nPosX;
				y = nPosY;
			}

			break;
		}

		default: return false;
	}

	return true;
}

void CRadar::Run()
{
	if (!CFG::Radar_Active || ((I::EngineVGui->IsGameUIVisible() || SDKUtils::BInEndOfMatch()) && !F::Menu->IsOpen()))
		return;

	if (CFG::Misc_Clean_Screenshot && I::EngineClient->IsTakingScreenshot())
	{
		return;
	}

	const int nRadarSize = CFG::Radar_Size;

	const auto crossColor = F::VisualUtils->GetAlphaColor(CFG::Menu_Accent_Secondary, CFG::Radar_Cross_Alpha);
	const auto outlineColor = F::VisualUtils->GetAlphaColor(CFG::Menu_Accent_Secondary, CFG::Radar_Outline_Alpha);
	const auto bgColor = F::VisualUtils->GetAlphaColor(CFG::Menu_Background, CFG::Radar_Background_Alpha);

	if (F::Menu->IsOpen())
		Drag();

	switch (CFG::Radar_Style)
	{
		// Rectangle
		case 0:
		{
			const int nRadarX = CFG::Radar_Pos_X;
			const int nRadarY = CFG::Radar_Pos_Y;

			H::Draw->Rect(nRadarX, nRadarY, nRadarSize, nRadarSize, bgColor);
			H::Draw->OutlinedRect(nRadarX, nRadarY, nRadarSize, nRadarSize, outlineColor);

			H::Draw->Line(
				nRadarX + (nRadarSize / 8),
				nRadarY + (nRadarSize / 2),
				nRadarX + (nRadarSize - ((nRadarSize / 8))),
				nRadarY + (nRadarSize / 2),
				crossColor
			);

			H::Draw->Line(
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize / 8),
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize - ((nRadarSize / 8))),
				crossColor
			);

			break;
		}

		// Circle
		case 1:
		{
			int nRadarX = CFG::Radar_Pos_X + nRadarSize / 2;
			int nRadarY = CFG::Radar_Pos_Y + nRadarSize / 2;

			H::Draw->FilledCircle(nRadarX, nRadarY, nRadarSize / 2, 100, bgColor);
			H::Draw->OutlinedCircle(nRadarX, nRadarY, nRadarSize / 2, 100, outlineColor);

			nRadarX = CFG::Radar_Pos_X;
			nRadarY = CFG::Radar_Pos_Y;

			H::Draw->Line(
				nRadarX + (nRadarSize / 8),
				nRadarY + (nRadarSize / 2),
				nRadarX + (nRadarSize - ((nRadarSize / 8))),
				nRadarY + (nRadarSize / 2),
				crossColor
			);

			H::Draw->Line(
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize / 8),
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize - ((nRadarSize / 8))),
				crossColor
			);

			break;
		}

		default: return;
	}

	const auto pLocal = H::Entities->GetLocal();

	if (!pLocal)
		return;

	const int nIconSize = CFG::Radar_Icon_Size;

	// Draw world objects
	if (CFG::Radar_World_Active)
	{
		if (!CFG::Radar_World_Ignore_HealthPacks)
		{
			for (const auto pEntity : H::Entities->GetGroup(EEntGroup::HEALTHPACKS))
			{
				if (!pEntity)
					continue;

				const auto pCachedData = H::Entities->GetCachedData(pEntity);
				if (!pCachedData)
					continue;

				int x = 0, y = 0;

				if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
					continue;

				H::Draw->Texture(x, y, nIconSize, nIconSize, F::VisualUtils->GetHealthIconTextureId(), POS_CENTERXY);
			}
		}

		if (!CFG::Radar_World_Ignore_AmmoPacks)
		{
			for (const auto pEntity : H::Entities->GetGroup(EEntGroup::AMMOPACKS))
			{
				if (!pEntity)
					continue;

				const auto pCachedData = H::Entities->GetCachedData(pEntity);
				if (!pCachedData)
					continue;

				int x = 0, y = 0;

				if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
					continue;

				H::Draw->Texture(x, y, nIconSize, nIconSize, F::VisualUtils->GetAmmoIconTextureId(), POS_CENTERXY);
			}
		}

		if (!CFG::Radar_World_Ignore_Halloween_Gift)
		{
			const float s = Math::RemapValClamped
			(
				static_cast<float>(nIconSize),
				18.0f,
				36.0f,
				0.5f,
				1.0f
			);

			for (const auto pEntity : H::Entities->GetGroup(EEntGroup::HALLOWEEN_GIFT))
			{
				if (!pEntity || !pEntity->ShouldDraw())
					continue;

				const auto pCachedData = H::Entities->GetCachedData(pEntity);
				if (!pCachedData)
					continue;

				int x = 0, y = 0;

				if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
					continue;

				H::Draw->Texture(x, y, static_cast<int>(36.0f * s), static_cast<int>(40.0f * s), F::VisualUtils->GetHalloweenGiftTextureId(), POS_CENTERXY);
			}
		}
	}

	// Draw buildings
	if (CFG::Radar_Buildings_Active)
	{
		for (const auto pEntity : H::Entities->GetGroup(EEntGroup::BUILDINGS_ALL))
		{
			if (!pEntity)
				continue;

			const auto pBuilding = pEntity->As<C_BaseObject>();

			if (pBuilding->m_bPlacing())
				continue;

			const bool bIsLocal = F::VisualUtils->IsEntityOwnedBy(pBuilding, pLocal);

			if (CFG::Radar_Buildings_Ignore_Local && bIsLocal)
				continue;

			if (!bIsLocal)
			{
				if (CFG::Radar_Buildings_Ignore_Teammates && pBuilding->m_iTeamNum() == pLocal->m_iTeamNum())
				{
					if (CFG::Radar_Buildings_Show_Teammate_Dispensers)
					{
						if (pBuilding->GetClassId() != ETFClassIds::CObjectDispenser)
							continue;
					}

					else
					{
						continue;
					}
				}

				if (CFG::Radar_Buildings_Ignore_Enemies && pBuilding->m_iTeamNum() != pLocal->m_iTeamNum())
					continue;
			}

			const auto nTexture = F::VisualUtils->GetBuildingTextureId(pBuilding);

			if (!nTexture)
				continue;

			const auto pCachedData = H::Entities->GetCachedData(pBuilding);
			if (!pCachedData)
				continue;

			int x = 0, y = 0;

			if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
				continue;

			Color_t entColor = F::VisualUtils->GetEntityColor(pLocal, pBuilding);
			entColor.a = 100;

			H::Draw->FilledCircle(x, y, (nIconSize + 8) / 2, 20, entColor);
			H::Draw->Texture(x, y, nIconSize, nIconSize, nTexture, POS_CENTERXY);
			//H::Draw->OutlinedCircle(x, y, (nIconSize + 8) / 2, 20, CFG::Color_ESP_Outline);
		}
	}

	// Draw players
	if (CFG::Radar_Players_Active)
	{
		for (const auto pEntity : H::Entities->GetGroup(EEntGroup::PLAYERS_ALL))
		{
			if (!pEntity)
				continue;

			const auto pPlayer = pEntity->As<C_TFPlayer>();

			if (pPlayer->deadflag())
				continue;

			const bool bIsLocal = pPlayer == pLocal;
			const bool bIsFriend = pPlayer->IsPlayerOnSteamFriendsList();

			if (CFG::Radar_Players_Ignore_Local && bIsLocal)
				continue;

			if (CFG::Radar_Players_Ignore_Friends && bIsFriend)
				continue;

			if (!bIsLocal)
			{
				if (!bIsFriend)
				{
					if (CFG::Radar_Players_Ignore_Teammates && pPlayer->m_iTeamNum() == pLocal->m_iTeamNum())
					{
						if (CFG::Radar_Players_Show_Teammate_Medics)
						{
							if (pPlayer->m_iClass() != TF_CLASS_MEDIC)
								continue;
						}

						else
						{
							continue;
						}
					}

					if (CFG::Radar_Players_Ignore_Enemies && pPlayer->m_iTeamNum() != pLocal->m_iTeamNum())
						continue;
				}

				if (CFG::Radar_Players_Ignore_Invisible && pPlayer->m_flInvisibility() >= 1.0f)
					continue;
			}

			const auto pCachedData = H::Entities->GetCachedData(pPlayer);
			if (!pCachedData)
				continue;

			int x = 0, y = 0;

			if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
				continue;

			Color_t entColor = F::VisualUtils->GetEntityColor(pLocal, pPlayer);
			entColor.a = 100;

			H::Draw->FilledCircle(x, y, (nIconSize + 4) / 2, 20, entColor);
			H::Draw->Texture(x, y, nIconSize, nIconSize, F::VisualUtils->GetClassIcon(pPlayer->m_iClass()), POS_CENTERXY);
			//H::Draw->OutlinedCircle(x, y, (nIconSize + 4) / 2, 20, CFG::Color_ESP_Outline);
		}
	}

	// Draw MvM money
	if (CFG::Radar_World_Active && !CFG::Radar_World_Ignore_MVM_Money)
	{
		for (const auto pEntity : H::Entities->GetGroup(EEntGroup::MVM_MONEY))
		{
			if (!pEntity || !pEntity->ShouldDraw())
				continue;

			const auto pCachedData = H::Entities->GetCachedData(pEntity);
			if (!pCachedData)
				continue;

			int x = 0, y = 0;

			if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
				continue;

			H::Draw->String(H::Fonts->Get(EFonts::ESP_SMALL), x, y, CFG::Color_MVM_Money, POS_CENTERXY, "$");
		}
	}
}

void CRadar::RunImGui()
{
	// DON'T update spectated player cache - this causes jitter
	// The spectated player will use the standard FRAME_RENDER_START cache
	// Minor jitter is acceptable trade-off vs flicker

	if (!CFG::Radar_Active)
		return;

	// Anti screenshot?
	if (CFG::Misc_Clean_Screenshot && I::EngineClient->IsTakingScreenshot())
	{
		return;
	}

	// In menu?
	if (!F::Menu->IsOpen() && (I::EngineVGui->IsGameUIVisible() || SDKUtils::BInEndOfMatch()))
		return;

	if (F::Menu->IsOpen())
		Drag();

	// Set ImGui draw list - use background to render behind menu
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	H::DrawImGui->SetDrawList(drawList);

	// Matrices updated once per frame in Present hook to prevent conflicts

	DrawRadarImGui();
}

void CRadar::DrawRadarImGui()
{
	const int nRadarSize = CFG::Radar_Size;
	const int nRadarX = CFG::Radar_Pos_X;
	const int nRadarY = CFG::Radar_Pos_Y;

	const auto crossColor = F::VisualUtils->GetAlphaColor(CFG::Menu_Accent_Secondary, CFG::Radar_Cross_Alpha);
	const auto outlineColor = F::VisualUtils->GetAlphaColor(CFG::Menu_Accent_Secondary, CFG::Radar_Outline_Alpha);
	const auto bgColor = F::VisualUtils->GetAlphaColor(CFG::Menu_Background, CFG::Radar_Background_Alpha);

	switch (CFG::Radar_Style)
	{
		case 0:
		{
			H::DrawImGui->Rect(nRadarX, nRadarY, nRadarSize, nRadarSize, bgColor);
			H::DrawImGui->OutlinedRect(nRadarX, nRadarY, nRadarSize, nRadarSize, outlineColor);

			H::DrawImGui->Line(
				nRadarX + (nRadarSize / 8),
				nRadarY + (nRadarSize / 2),
				nRadarX + (nRadarSize - ((nRadarSize / 8))),
				nRadarY + (nRadarSize / 2),
				crossColor
			);

			H::DrawImGui->Line(
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize / 8),
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize - ((nRadarSize / 8))),
				crossColor
			);

			break;
		}

		case 1:
		{
			int nRadarCenterX = nRadarX + nRadarSize / 2;
			int nRadarCenterY = nRadarY + nRadarSize / 2;

			H::DrawImGui->FilledCircle(nRadarCenterX, nRadarCenterY, nRadarSize / 2, 100, bgColor);
			H::DrawImGui->OutlinedCircle(nRadarCenterX, nRadarCenterY, nRadarSize / 2, 100, outlineColor);

			H::DrawImGui->Line(
				nRadarX + (nRadarSize / 8),
				nRadarY + (nRadarSize / 2),
				nRadarX + (nRadarSize - ((nRadarSize / 8))),
				nRadarY + (nRadarSize / 2),
				crossColor
			);

			H::DrawImGui->Line(
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize / 8),
				nRadarX + (nRadarSize / 2),
				nRadarY + (nRadarSize - ((nRadarSize / 8))),
				crossColor
			);

			break;
		}

		default: return;
	}

	const auto pLocal = H::Entities->GetLocal();

	if (!pLocal)
		return;

	DrawWorldRadar();

	if (CFG::Radar_Buildings_Active)
	{
		for (const auto pEntity : H::Entities->GetGroup(EEntGroup::BUILDINGS_ALL))
		{
			if (!pEntity)
				continue;

			const auto pBuilding = pEntity->As<C_BaseObject>();

			if (pBuilding->m_bPlacing())
				continue;

			DrawBuildingRadar(pBuilding, pLocal);
		}
	}

	if (CFG::Radar_Players_Active)
	{
		for (const auto pEntity : H::Entities->GetGroup(EEntGroup::PLAYERS_ALL))
		{
			if (!pEntity)
				continue;

			const auto pPlayer = pEntity->As<C_TFPlayer>();

			if (pPlayer->deadflag())
				continue;

			DrawPlayerRadar(pPlayer, pLocal);
		}
	}
}

void CRadar::DrawPlayerRadar(C_TFPlayer* pPlayer, C_TFPlayer* pLocal)
{
	const bool bIsLocal = pPlayer == pLocal;
	const bool bIsFriend = pPlayer->IsPlayerOnSteamFriendsList();

	if (CFG::Radar_Players_Ignore_Local && bIsLocal)
		return;

	if (CFG::Radar_Players_Ignore_Friends && bIsFriend)
		return;

	if (!bIsLocal)
	{
		if (!bIsFriend)
		{
			if (CFG::Radar_Players_Ignore_Teammates && pPlayer->m_iTeamNum() == pLocal->m_iTeamNum())
			{
				if (CFG::Radar_Players_Show_Teammate_Medics)
				{
					if (pPlayer->m_iClass() != TF_CLASS_MEDIC)
						return;
				}
				else
				{
					return;
				}
			}

			if (CFG::Radar_Players_Ignore_Enemies && pPlayer->m_iTeamNum() != pLocal->m_iTeamNum())
				return;
		}

		if (CFG::Radar_Players_Ignore_Invisible && pPlayer->m_flInvisibility() >= 1.0f)
			return;
	}

	const auto pCachedData = H::Entities->GetCachedData(pPlayer);
	if (!pCachedData)
		return;

	int x = 0, y = 0;

	if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
		return;

	const int nIconSize = CFG::Radar_Icon_Size;

	Color_t entColor = F::VisualUtils->GetEntityColor(pLocal, pPlayer);
	entColor.a = 100;

	H::DrawImGui->FilledCircle(x, y, (nIconSize + 4) / 2, 20, entColor);
	H::DrawImGui->Texture(x, y, nIconSize, nIconSize, F::VisualUtils->GetClassIcon(pPlayer->m_iClass()), POS_CENTERXY);
}

void CRadar::DrawBuildingRadar(C_BaseObject* pBuilding, C_TFPlayer* pLocal)
{
	const bool bIsLocal = F::VisualUtils->IsEntityOwnedBy(pBuilding, pLocal);

	if (CFG::Radar_Buildings_Ignore_Local && bIsLocal)
		return;

	if (!bIsLocal)
	{
		if (CFG::Radar_Buildings_Ignore_Teammates && pBuilding->m_iTeamNum() == pLocal->m_iTeamNum())
		{
			if (CFG::Radar_Buildings_Show_Teammate_Dispensers)
			{
				if (pBuilding->GetClassId() != ETFClassIds::CObjectDispenser)
					return;
			}
			else
			{
				return;
			}
		}

		if (CFG::Radar_Buildings_Ignore_Enemies && pBuilding->m_iTeamNum() != pLocal->m_iTeamNum())
			return;
	}

	const auto nTexture = F::VisualUtils->GetBuildingTextureId(pBuilding);

	if (!nTexture)
		return;

	const auto pCachedData = H::Entities->GetCachedData(pBuilding);
	if (!pCachedData)
		return;

	int x = 0, y = 0;

	if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
		return;

	const int nIconSize = CFG::Radar_Icon_Size;

	Color_t entColor = F::VisualUtils->GetEntityColor(pLocal, pBuilding);
	entColor.a = 100;

	H::DrawImGui->FilledCircle(x, y, (nIconSize + 8) / 2, 20, entColor);
	H::DrawImGui->Texture(x, y, nIconSize, nIconSize, nTexture, POS_CENTERXY);
}

void CRadar::DrawWorldRadar()
{
	const auto pLocal = H::Entities->GetLocal();

	if (!pLocal)
		return;

	const int nIconSize = CFG::Radar_Icon_Size;

	if (CFG::Radar_World_Active)
	{
		if (!CFG::Radar_World_Ignore_HealthPacks)
		{
			for (const auto pEntity : H::Entities->GetGroup(EEntGroup::HEALTHPACKS))
			{
				if (!pEntity)
					continue;

				const auto pCachedData = H::Entities->GetCachedData(pEntity);
				if (!pCachedData)
					continue;

				int x = 0, y = 0;

				if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
					continue;

				H::DrawImGui->Texture(x, y, nIconSize, nIconSize, F::VisualUtils->GetHealthIconTextureId(), POS_CENTERXY);
			}
		}

		if (!CFG::Radar_World_Ignore_AmmoPacks)
		{
			for (const auto pEntity : H::Entities->GetGroup(EEntGroup::AMMOPACKS))
			{
				if (!pEntity)
					continue;

				const auto pCachedData = H::Entities->GetCachedData(pEntity);
				if (!pCachedData)
					continue;

				int x = 0, y = 0;

				if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
					continue;

				H::DrawImGui->Texture(x, y, nIconSize, nIconSize, F::VisualUtils->GetAmmoIconTextureId(), POS_CENTERXY);
			}
		}

		if (!CFG::Radar_World_Ignore_Halloween_Gift)
		{
			const float s = Math::RemapValClamped
			(
				static_cast<float>(nIconSize),
				18.0f,
				36.0f,
				0.5f,
				1.0f
			);

			for (const auto pEntity : H::Entities->GetGroup(EEntGroup::HALLOWEEN_GIFT))
			{
				if (!pEntity || !pEntity->ShouldDraw())
					continue;

				const auto pCachedData = H::Entities->GetCachedData(pEntity);
				if (!pCachedData)
					continue;

				int x = 0, y = 0;

				if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
					continue;

				H::DrawImGui->Texture(x, y, static_cast<int>(36.0f * s), static_cast<int>(40.0f * s), F::VisualUtils->GetHalloweenGiftTextureId(), POS_CENTERXY);
			}
		}
	}

	if (CFG::Radar_World_Active && !CFG::Radar_World_Ignore_MVM_Money)
	{
		for (const auto pEntity : H::Entities->GetGroup(EEntGroup::MVM_MONEY))
		{
			if (!pEntity || !pEntity->ShouldDraw())
				continue;

			const auto pCachedData = H::Entities->GetCachedData(pEntity);
			if (!pCachedData)
				continue;

			int x = 0, y = 0;

			if (!GetDrawPosition(x, y, pCachedData->renderOrigin))
				continue;

			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_SMALL), x, y, CFG::Color_MVM_Money, POS_CENTERXY, "$");
		}
	}
}
