#include "ESP.h"

#include "../CFG.h"
#include "../SpyCamera/SpyCamera.h"
#include "../VisualUtils/VisualUtils.h"
#include <cmath>
#include <float.h>

constexpr int SPACING_X = 2;
constexpr int SPACING_Y = 2;
constexpr Color_t WHITE = {220, 220, 220, 255};

// TODO: These utility functions should be moved somewhere else

const char* GetBuildingName(C_BaseObject* pBuilding)
{
	switch (pBuilding->GetClassId())
	{
	case ETFClassIds::CObjectSentrygun: return pBuilding->m_bMiniBuilding() ? "Mini Sentrygun" : "Sentrygun";
	case ETFClassIds::CObjectDispenser: return "Dispenser";
	case ETFClassIds::CObjectTeleporter: return pBuilding->m_iObjectMode() == MODE_TELEPORTER_ENTRANCE ? "Teleporter In" : "Teleporter Out";
	default: return "Unknown Building Name";
	}
}

const char* GetProjectileName(C_BaseEntity* pEntity)
{
	switch (pEntity->GetClassId())
	{
	case ETFClassIds::CTFProjectile_Rocket:
	case ETFClassIds::CTFProjectile_SentryRocket: return "Rocket";
	case ETFClassIds::CTFProjectile_Jar: return "Jarate";
	case ETFClassIds::CTFProjectile_JarGas: return "Gas";
	case ETFClassIds::CTFProjectile_JarMilk: return "Milk";
	case ETFClassIds::CTFProjectile_Arrow: return "Arrow";
	case ETFClassIds::CTFProjectile_Flare: return "Flare";
	case ETFClassIds::CTFProjectile_Cleaver: return "Cleaver";
	case ETFClassIds::CTFProjectile_HealingBolt: return "Healing Arrow";
	case ETFClassIds::CTFGrenadePipebombProjectile:
		{
			const auto pPipebomb = pEntity->As<C_TFGrenadePipebombProjectile>();

			return pPipebomb->HasStickyEffects() ? "Sticky" : pPipebomb->m_iType() == TF_GL_MODE_CANNONBALL ? "Cannonball" : "Pipe";
		}

	default: return "Projectile";
	}
}

const char* GetPlayerClassName(C_TFPlayer* pPlayer)
{
	switch (pPlayer->m_iClass())
	{
	case TF_CLASS_SCOUT: return "Scout";
	case TF_CLASS_SOLDIER: return "Soldier";
	case TF_CLASS_PYRO: return "Pyro";
	case TF_CLASS_DEMOMAN: return "Demoman";
	case TF_CLASS_HEAVYWEAPONS: return "Heavy";
	case TF_CLASS_ENGINEER: return "Engineer";
	case TF_CLASS_MEDIC: return "Medic";
	case TF_CLASS_SNIPER: return "Sniper";
	case TF_CLASS_SPY: return "Spy";
	default: return "Unknown Class";
	}
}

bool CESP::GetDrawBounds(C_BaseEntity* pEntity, int& x, int& y, int& w, int& h)
{
	if (!pEntity)
		return false;

	const auto pCachedData = H::Entities->GetCachedData(pEntity);
	if (!pCachedData)
		return false;

	// Only skip cached data if it's very old (more than 30 frames = ~0.5 seconds)
	if (I::GlobalVars->framecount - pCachedData->frameNumber > 30) {
		return false; // Cached data is too old
	}

	bool bIsPlayer = false;
	const Vec3& vMins = pCachedData->mins;
	const Vec3& vMaxs = pCachedData->maxs;

	// Use reference for common case, only copy if we need to modify (local player)
	const matrix3x4_t* pTransform = &pCachedData->transform;

	if (pEntity->GetClassId() == ETFClassIds::CTFPlayer)
	{
		const auto pPlayer = pEntity->As<C_TFPlayer>();
		bIsPlayer = true;
	}

	const matrix3x4_t& transform = *pTransform;

	const Vec3 vPoints[] =
	{
		Vec3(vMins.x, vMins.y, vMins.z),
		Vec3(vMins.x, vMaxs.y, vMins.z),
		Vec3(vMaxs.x, vMaxs.y, vMins.z),
		Vec3(vMaxs.x, vMins.y, vMins.z),
		Vec3(vMaxs.x, vMaxs.y, vMaxs.z),
		Vec3(vMins.x, vMaxs.y, vMaxs.z),
		Vec3(vMins.x, vMins.y, vMaxs.z),
		Vec3(vMaxs.x, vMins.y, vMaxs.z)
	};

	Vec3 vTransformed[8] = {};

	for (int n = 0; n < 8; n++)
	{
		Math::VectorTransform(vPoints[n], transform, vTransformed[n]);
	}

	Vec3 flb = {}, brt = {}, blb = {}, frt = {}, frb = {}, brb = {}, blt = {}, flt = {};

	// Enhanced W2S validation with coordinate bounds checking
	if (H::DrawImGui->W2S(vTransformed[3], flb) && H::DrawImGui->W2S(vTransformed[5], brt)
		&& H::DrawImGui->W2S(vTransformed[0], blb) && H::DrawImGui->W2S(vTransformed[4], frt)
		&& H::DrawImGui->W2S(vTransformed[2], frb) && H::DrawImGui->W2S(vTransformed[1], brb)
		&& H::DrawImGui->W2S(vTransformed[6], blt) && H::DrawImGui->W2S(vTransformed[7], flt))
	{
		// Validate that all screen coordinates are reasonable
		const Vec3 coordsArray[] = {flb, brt, blb, frt, frb, brb, blt, flt};
		bool bValidCoords = true;

		for (int n = 0; n < 8; n++) {
			// Check for NaN, infinity, or extreme values that indicate bad W2S projection
			// Use _finite() from float.h instead of isfinite() for better compatibility
			if (!_finite(static_cast<double>(coordsArray[n].x)) || !_finite(static_cast<double>(coordsArray[n].y)) ||
				std::abs(coordsArray[n].x) > 50000.0f || std::abs(coordsArray[n].y) > 50000.0f) {
				bValidCoords = false;
				break;
			}
		}

		if (!bValidCoords) {
			return false; // Skip this entity due to invalid screen coordinates
		}

		const Vec3 arr[] = {flb, brt, blb, frt, frb, brb, blt, flt};

		float left = flb.x;
		float top = flb.y;
		float righ = flb.x;
		float bottom = flb.y;

		for (int n = 1; n < 8; n++)
		{
			if (left > arr[n].x)
				left = arr[n].x;

			if (top < arr[n].y)
				top = arr[n].y;

			if (righ < arr[n].x)
				righ = arr[n].x;

			if (bottom > arr[n].y)
				bottom = arr[n].y;
		}

		float x_ = left;
		float y_ = bottom;
		float w_ = (righ - left);
		float h_ = (top - bottom);

		if (bIsPlayer)
		{
			x_ += ((righ - left) / 8.0f);
			w_ -= (((righ - left) / 8.0f) * 2.0f);
		}

		x = static_cast<int>(x_);
		y = static_cast<int>(y_);
		w = static_cast<int>(w_);
		h = static_cast<int>(h_);

		return x <= H::DrawImGui->GetScreenW() && (x + w) >= 0 && y <= H::DrawImGui->GetScreenH() && (y + h) >= 0;
	}

	return false;
}

void CESP::DrawBones(C_TFPlayer* pPlayer, Color_t color)
{
	// Use cached bone data to prevent flickering/warping
	const auto pCachedData = H::Entities->GetCachedData(pPlayer);
	if (!pCachedData || !pCachedData->bBonesValid)
		return;

	// Only skip bone data if it's very old (more than 30 frames = ~0.5 seconds)
	if (I::GlobalVars->framecount - pCachedData->frameNumber > 30)
		return;

	auto MatrixPosition = [](const matrix3x4_t& matrix, Vector& position)
	{
		position[0] = matrix[0][3];
		position[1] = matrix[1][3];
		position[2] = matrix[2][3];
	};

	const model_t* pModel = pPlayer->GetModel();

	if (!pModel)
		return;

	const studiohdr_t* pStudioHdr = I::ModelInfoClient->GetStudiomodel(pModel);

	if (!pStudioHdr)
		return;

	// Use cached bone matrices instead of calling SetupBones() directly
	const matrix3x4_t* boneMatrix = pCachedData->boneMatrix;

	Vec3 p1 = {}, p2 = {};
	Vec3 p1s = {}, p2s = {};

	for (int n = 0; n < pStudioHdr->numbones; n++)
	{
		const mstudiobone_t* pBone = pStudioHdr->pBone(n);

		if (!pBone || pBone->parent == -1 || !(pBone->flags & BONE_USED_BY_HITBOX))
			continue;

		MatrixPosition(boneMatrix[n], p1);

		if (!H::DrawImGui->W2S(p1, p1s))
			continue;

		MatrixPosition(boneMatrix[pBone->parent], p2);

		if (!H::DrawImGui->W2S(p2, p2s))
			continue;

		// Validate screen coordinates before drawing bones
		if (!_finite(static_cast<double>(p1s.x)) || !_finite(static_cast<double>(p1s.y)) ||
			!_finite(static_cast<double>(p2s.x)) || !_finite(static_cast<double>(p2s.y)))
			continue;

		H::DrawImGui->Line(static_cast<int>(p1s.x), static_cast<int>(p1s.y), static_cast<int>(p2s.x), static_cast<int>(p2s.y), color);
	}
}

void CESP::Run()
{
	// DISABLED: Use ImGui rendering instead to prevent conflicts and flicker
	// This old MatSystemSurface-based rendering conflicts with ImGui ESP rendering
	return;
}

void CESP::DrawPlayerESP(C_TFPlayer* pPlayer, C_TFPlayer* pLocal, int x, int y, int w, int h)
{
	// Apply alpha to all colors
	Color_t entColor = ApplyAlpha(F::VisualUtils->GetEntityColor(pLocal, pPlayer), CFG::ESP_Players_Alpha);
	Color_t healthColor = ApplyAlpha(F::VisualUtils->GetHealthColor(pPlayer->m_iHealth(), pPlayer->GetMaxHealth()), CFG::ESP_Players_Alpha);
	Color_t textColor = ApplyAlpha(CFG::ESP_Text_Color == 0 ? F::VisualUtils->GetEntityColor(pLocal, pPlayer) : CFG::Color_ESP_Text, CFG::ESP_Players_Alpha);

	bool bIsLocal = pPlayer == pLocal;
	int nTextOffsetY = 0;

	// Tracer
	if (CFG::ESP_Players_Tracer && !bIsLocal)
	{
		int nFromY = 0;
		switch (CFG::ESP_Tracer_From)
		{
		case 0: nFromY = 0; break;
		case 1: nFromY = H::DrawImGui->GetScreenH() / 2; break;
		case 2: nFromY = H::DrawImGui->GetScreenH(); break;
		}

		int nToY = 0;
		switch (CFG::ESP_Tracer_To)
		{
		case 0: nToY = y; break;
		case 1: nToY = y + (h / 2); break;
		case 2: nToY = y + h; break;
		}

		H::DrawImGui->Line(H::DrawImGui->GetScreenW() / 2, nFromY, x + (w / 2), nToY, entColor);
	}

	// Basic box
	if (CFG::ESP_Players_Box)
	{
		H::DrawImGui->OutlinedRect(x, y, w, h, entColor);
		H::DrawImGui->OutlinedRect(x - 1, y - 1, w + 2, h + 2, CFG::Color_ESP_Outline);
	}

	// Health bar
	if (CFG::ESP_Players_HealthBar && !pPlayer->InCond(TF_COND_HALLOWEEN_GHOST_MODE))
	{
		auto flHealth = static_cast<float>(pPlayer->m_iHealth());
		auto flMaxHealth = static_cast<float>(pPlayer->GetMaxHealth());

		if (flHealth > 0.f && flMaxHealth > 0.f)
		{
			if (flHealth > flMaxHealth)
				flMaxHealth = flHealth;

			static constexpr int BAR_WIDTH = 2;
			int nBarX = x - ((BAR_WIDTH * 2) + 1);
			int nFillH = static_cast<int>(Math::RemapValClamped(flHealth, 0.0f, flMaxHealth, 0.0f, static_cast<float>(h)));

			H::DrawImGui->OutlinedRect(nBarX - 1, (y + h - nFillH) - 1, BAR_WIDTH + 2, nFillH + 2, CFG::Color_ESP_Outline);
			H::DrawImGui->Rect(nBarX, y + h - nFillH, BAR_WIDTH, nFillH, healthColor);
		}
	}

	// Bones
	if (CFG::ESP_Players_Bones)
	{
		DrawBones(pPlayer, CFG::ESP_Players_Bones_Color == 0 ? entColor : WHITE);
	}

	// Name (rendered above the box, centered)
	if (CFG::ESP_Players_Name)
	{
		player_info_t playerInfo = {};
		if (I::EngineClient->GetPlayerInfo(pPlayer->entindex(), &playerInfo))
		{
			H::DrawImGui->String(
				H::Fonts->Get(EFonts::ESP_SMALL),
				x + (w / 2),
				(y - (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall - 1)) - SPACING_Y,
				textColor,
				POS_CENTERX,
				"%hs",
				playerInfo.name
			);
		}
	}

	// Class text (right side, stacked with offset)
	if (CFG::ESP_Players_Class)
	{
		H::DrawImGui->String(
			H::Fonts->Get(EFonts::ESP_SMALL),
			x + w + SPACING_X,
			y + (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall * nTextOffsetY++),
			textColor,
			POS_DEFAULT,
			"%hs",
			GetPlayerClassName(pPlayer)
		);
	}

	// Class icon
	if (CFG::ESP_Players_Class_Icon)
	{
		static constexpr int CLASS_ICON_SIZE = 18;

		H::DrawImGui->Texture(
			x + (w / 2),
			CFG::ESP_Players_Name ? ((y - (H::Fonts->Get(EFonts::ESP).m_nTall - 1)) - SPACING_Y) - CLASS_ICON_SIZE : y - (CLASS_ICON_SIZE + SPACING_Y),
			CLASS_ICON_SIZE,
			CLASS_ICON_SIZE,
			F::VisualUtils->GetClassIcon(pPlayer->m_iClass()),
			POS_CENTERX
		);
	}

	// Health text
	if (CFG::ESP_Players_Health && !pPlayer->InCond(TF_COND_HALLOWEEN_GHOST_MODE))
	{
		H::DrawImGui->String(
			H::Fonts->Get(EFonts::ESP_SMALL),
			x + w + SPACING_X,
			y + (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall * nTextOffsetY++),
			healthColor,
			POS_DEFAULT,
			"%d",
			pPlayer->m_iHealth()
		);
	}

	// Uber text
	if (CFG::ESP_Players_Uber)
	{
		if (pPlayer->m_iClass() == TF_CLASS_MEDIC)
		{
			if (auto pWeapon = pPlayer->GetWeaponFromSlot(1))
			{
				H::DrawImGui->String(
					H::Fonts->Get(EFonts::ESP_SMALL),
					x + w + SPACING_X,
					y + (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall * nTextOffsetY++),
					ApplyAlpha(CFG::Color_Uber, CFG::ESP_Players_Alpha),
					POS_DEFAULT,
					"%d%%", static_cast<int>(pWeapon->As<C_WeaponMedigun>()->m_flChargeLevel() * 100.0f)
				);
			}
		}
	}

	// Uber bar
	if (CFG::ESP_Players_UberBar)
	{
		if (pPlayer->m_iClass() == TF_CLASS_MEDIC)
		{
			if (auto pWeapon = pPlayer->GetWeaponFromSlot(1))
			{
				auto pMedigun = pWeapon->As<C_WeaponMedigun>();

				if (auto flCharge = pMedigun->m_flChargeLevel())
				{
					int nBarH = 2;
					int nDrawY = y + h + nBarH + 1;
					float flFillW = Math::RemapValClamped(flCharge, 0.0f, 1.0f, 0.0f, static_cast<float>(w));

					H::DrawImGui->OutlinedRect(x - 1, nDrawY - 1, static_cast<int>(flFillW) + 2, nBarH + 2, CFG::Color_ESP_Outline);
					H::DrawImGui->Rect(x, nDrawY, static_cast<int>(flFillW), nBarH, ApplyAlpha(CFG::Color_Uber, CFG::ESP_Players_Alpha));

					if (pMedigun->m_iItemDefinitionIndex() == Medic_s_TheVaccinator)
					{
						if (flCharge >= 0.25f)
							H::DrawImGui->Rect(x + static_cast<int>(static_cast<float>(w) * 0.25f) - 1, nDrawY, 2, nBarH, CFG::Color_ESP_Outline);

						if (flCharge >= 0.5f)
							H::DrawImGui->Rect(x + static_cast<int>(static_cast<float>(w) * 0.5f) - 1, nDrawY, 2, nBarH, CFG::Color_ESP_Outline);

						if (flCharge >= 0.75f)
							H::DrawImGui->Rect(x + static_cast<int>(static_cast<float>(w) * 0.75f) - 1, nDrawY, 2, nBarH, CFG::Color_ESP_Outline);
					}
				}
			}
		}
	}

	// Conditions
	if (CFG::ESP_Players_Conds)
	{
		if (nTextOffsetY > 0)
			nTextOffsetY += 1;

		int drawX = x + w + SPACING_X;
		int tall = H::Fonts->Get(EFonts::ESP_CONDS).m_nTall;

		Color_t color = ApplyAlpha(CFG::Color_Conds, CFG::ESP_Players_Alpha);

		if (pPlayer->IsZoomed())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "ZOOM");

		if (pPlayer->IsInvisible())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "INVIS");

		if (pPlayer->m_bFeignDeathReady())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "DEADRINGER");

		if (pPlayer->IsInvulnerable())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "INVULN");

		if (pPlayer->IsCritBoosted())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "CRIT");

		if (pPlayer->IsMiniCritBoosted())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "MINICRIT");

		if (pPlayer->IsMarked())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "MARKED");

		if (pPlayer->InCond(TF_COND_MAD_MILK))
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "MILK");

		if (pPlayer->InCond(TF_COND_TAUNTING))
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "TAUNT");

		if (pPlayer->InCond(TF_COND_DISGUISED))
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "DISGUISE");

		if (pPlayer->InCond(TF_COND_BURNING) || pPlayer->InCond(TF_COND_BURNING_PYRO))
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "BURNING");

		if (pPlayer->InCond(TF_COND_OFFENSEBUFF))
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "BANNER");

		if (pPlayer->InCond(TF_COND_DEFENSEBUFF))
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "BACKUP");

		if (pPlayer->InCond(TF_COND_REGENONDAMAGEBUFF))
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "CONCH");

		if (!pPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST))
		{
			if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BULLET_RESIST))
				H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "BULLET(RES)");
		}
		else
		{
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "BULLET(UBER)");
		}

		if (!pPlayer->InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST))
		{
			if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BLAST_RESIST))
				H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "EXPLOSION(RES)");
		}
		else
		{
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "EXPLOSION(UBER)");
		}

		if (!pPlayer->InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST))
		{
			if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_FIRE_RESIST))
				H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "FIRE(RES)");
		}
		else
		{
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "FIRE(UBER)");
		}
	}
}

void CESP::DrawBuildingESP(C_BaseObject* pBuilding, C_TFPlayer* pLocal)
{
	// Check if building is placed (not being built or carried) - do this FIRST
	if (!pBuilding || pBuilding->m_bPlacing() || pBuilding->m_bCarried())
		return;

	int x, y, w, h;
	if (!GetDrawBounds(pBuilding, x, y, w, h))
		return;

	// Apply alpha to all colors (safe to access properties now)
	Color_t entColor = ApplyAlpha(F::VisualUtils->GetEntityColor(pLocal, pBuilding), CFG::ESP_Buildings_Alpha);
	Color_t healthColor = ApplyAlpha(F::VisualUtils->GetHealthColor(pBuilding->m_iHealth(), pBuilding->m_iMaxHealth()), CFG::ESP_Buildings_Alpha);
	Color_t textColor = ApplyAlpha(CFG::ESP_Text_Color == 0 ? F::VisualUtils->GetEntityColor(pLocal, pBuilding) : CFG::Color_ESP_Text, CFG::ESP_Buildings_Alpha);

	int nTextOffsetY = 0;

	// Tracer
	if (CFG::ESP_Buildings_Tracer)
	{
		int nFromY = 0;
		switch (CFG::ESP_Tracer_From)
		{
		case 0: nFromY = 0; break;
		case 1: nFromY = H::DrawImGui->GetScreenH() / 2; break;
		case 2: nFromY = H::DrawImGui->GetScreenH(); break;
		}

		int nToY = 0;
		switch (CFG::ESP_Tracer_To)
		{
		case 0: nToY = y; break;
		case 1: nToY = y + (h / 2); break;
		case 2: nToY = y + h; break;
		}

		H::DrawImGui->Line(H::DrawImGui->GetScreenW() / 2, nFromY, x + (w / 2), nToY, entColor);
	}

	// Basic box
	if (CFG::ESP_Buildings_Box)
	{
		H::DrawImGui->OutlinedRect(x, y, w, h, entColor);
		H::DrawImGui->OutlinedRect(x - 1, y - 1, w + 2, h + 2, CFG::Color_ESP_Outline);
	}

	// Building name (centered above box)
	if (CFG::ESP_Buildings_Name)
	{
		H::DrawImGui->String(
			H::Fonts->Get(EFonts::ESP_SMALL),
			x + (w / 2),
			(y - (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall - 1)) - SPACING_Y,
			textColor,
			POS_CENTERX,
			"%hs",
			GetBuildingName(pBuilding)
		);
	}

	// Health text
	if (CFG::ESP_Buildings_Health)
	{
		int health = pBuilding->m_iHealth();
		if (health > 0)
		{
			H::DrawImGui->String(
				H::Fonts->Get(EFonts::ESP_SMALL),
				x + w + SPACING_X,
				y + (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall * nTextOffsetY++),
				healthColor,
				POS_DEFAULT,
				"%d",
				health
			);
		}
	}

	// Health bar
	if (CFG::ESP_Buildings_HealthBar)
	{
		float flHealth = static_cast<float>(pBuilding->m_iHealth());
		float flMaxHealth = static_cast<float>(pBuilding->m_iMaxHealth());

		if (flHealth > 0.f && flMaxHealth > 0.f)
		{
			if (flHealth > flMaxHealth)
				flMaxHealth = flHealth;

			static constexpr int BAR_WIDTH = 2;
			int nBarX = x - ((BAR_WIDTH * 2) + 1);
			int nFillH = static_cast<int>(Math::RemapValClamped(flHealth, 0.0f, flMaxHealth, 0.0f, static_cast<float>(h)));

			H::DrawImGui->OutlinedRect(nBarX - 1, (y + h - nFillH) - 1, BAR_WIDTH + 2, nFillH + 2, CFG::Color_ESP_Outline);
			H::DrawImGui->Rect(nBarX, y + h - nFillH, BAR_WIDTH, nFillH, healthColor);
		}
	}

	// Level text
	if (CFG::ESP_Buildings_Level)
	{
		int level = pBuilding->m_iUpgradeLevel();
		if (level > 0)
		{
			H::DrawImGui->String(
				H::Fonts->Get(EFonts::ESP_SMALL),
				x + w + SPACING_X,
				y + (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall * nTextOffsetY++),
				textColor,
				POS_DEFAULT,
				"%d",
				level
			);
		}
	}

	// Level bar
	if (CFG::ESP_Buildings_LevelBar)
	{
		int level = pBuilding->m_iUpgradeLevel();
		if (level > 0)
		{
			int nBarH = 2;
			int nDrawY = y + h + nBarH + 1;
			float flLevel = static_cast<float>(level);
			float flFillW = Math::RemapValClamped(flLevel, 1.0f, 3.0f, w / 3.0f, static_cast<float>(w));

			if (pBuilding->m_bMiniBuilding())
				flFillW = static_cast<float>(w);

			H::DrawImGui->OutlinedRect(x - 1, nDrawY - 1, static_cast<int>(flFillW) + 2, nBarH + 2, CFG::Color_ESP_Outline);
			H::DrawImGui->Rect(x, nDrawY, static_cast<int>(flFillW), nBarH, WHITE);
		}
	}

	// Conditions
	if (CFG::ESP_Buildings_Conds)
	{
		if (nTextOffsetY > 0)
			nTextOffsetY += 1;

		int drawX = x + w + SPACING_X;
		int tall = H::Fonts->Get(EFonts::ESP_CONDS).m_nTall;

		Color_t color = ApplyAlpha(CFG::Color_Conds, CFG::ESP_Buildings_Alpha);

		if (pBuilding->m_bBuilding())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "BUILDING");

		if (pBuilding->m_bHasSapper())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "SAPPED");

		if (pBuilding->m_bDisabled())
			H::DrawImGui->String(H::Fonts->Get(EFonts::ESP_CONDS), drawX, y + (tall * nTextOffsetY++), color, POS_DEFAULT, "DISABLED");
	}
}

void CESP::DrawProjectileESP(C_BaseEntity* pProjectile, C_TFPlayer* pLocal)
{
	if (!pProjectile || !pProjectile->ShouldDraw())
		return;

	bool bIsLocal = F::VisualUtils->IsEntityOwnedBy(pProjectile, pLocal);

	// Check ignore filters
	if (CFG::ESP_World_Ignore_LocalProjectiles && bIsLocal)
		return;

	if (!bIsLocal)
	{
		if (CFG::ESP_World_Ignore_EnemyProjectiles && pProjectile->m_iTeamNum() != pLocal->m_iTeamNum())
			return;

		if (CFG::ESP_World_Ignore_TeammateProjectiles && pProjectile->m_iTeamNum() == pLocal->m_iTeamNum())
			return;
	}

	// Use GetDrawBounds to prevent flickering (don't call interpolated GetAbsOrigin directly!)
	int x = 0, y = 0, w = 0, h = 0;
	if (!GetDrawBounds(pProjectile, x, y, w, h))
		return;

	// Apply alpha to colors
	Color_t entColor = ApplyAlpha(F::VisualUtils->GetEntityColor(pLocal, pProjectile), CFG::ESP_World_Alpha);
	Color_t textColor = ApplyAlpha(CFG::ESP_Text_Color == 0 ? F::VisualUtils->GetEntityColor(pLocal, pProjectile) : CFG::Color_ESP_Text, CFG::ESP_World_Alpha);

	// Tracer
	if (CFG::ESP_World_Tracer)
	{
		int nFromY = 0;
		switch (CFG::ESP_Tracer_From)
		{
		case 0: nFromY = 0; break;
		case 1: nFromY = H::DrawImGui->GetScreenH() / 2; break;
		case 2: nFromY = H::DrawImGui->GetScreenH(); break;
		}

		int nToY = 0;
		switch (CFG::ESP_Tracer_To)
		{
		case 0: nToY = y; break;
		case 1: nToY = y + (h / 2); break;
		case 2: nToY = y + h; break;
		}

		H::DrawImGui->Line(H::DrawImGui->GetScreenW() / 2, nFromY, x + (w / 2), nToY, entColor);
	}

	// Name
	if (CFG::ESP_World_Name)
	{
		H::DrawImGui->String(
			H::Fonts->Get(EFonts::ESP_SMALL),
			x + (w / 2),
			(y - (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall - 1)) - SPACING_Y,
			textColor,
			POS_CENTERX,
			"%hs",
			GetProjectileName(pProjectile)
		);
	}

	// Box
	if (CFG::ESP_World_Box)
	{
		H::DrawImGui->OutlinedRect(x, y, w, h, entColor);
		H::DrawImGui->OutlinedRect(x - 1, y - 1, w + 2, h + 2, CFG::Color_ESP_Outline);
	}
}

void CESP::DrawWorldESP()
{
	// Handle world items like health packs, ammo packs, etc.
	if (CFG::ESP_World_Active)
	{
		int x = 0, y = 0, w = 0, h = 0;

		// Draw health packs
		if (!CFG::ESP_World_Ignore_HealthPacks)
		{
			Color_t color = ApplyAlpha(CFG::Color_HealthPack, CFG::ESP_World_Alpha);
			Color_t textColor = ApplyAlpha(CFG::ESP_Text_Color == 0 ? CFG::Color_HealthPack : CFG::Color_ESP_Text, CFG::ESP_World_Alpha);

			for (auto pEntity : H::Entities->GetGroup(EEntGroup::HEALTHPACKS))
			{
				if (!pEntity || !GetDrawBounds(pEntity, x, y, w, h))
					continue;

				// Tracer
				if (CFG::ESP_World_Tracer)
				{
					int nFromY = 0;
					switch (CFG::ESP_Tracer_From)
					{
					case 0: nFromY = 0; break;
					case 1: nFromY = H::DrawImGui->GetScreenH() / 2; break;
					case 2: nFromY = H::DrawImGui->GetScreenH(); break;
					}

					int nToY = 0;
					switch (CFG::ESP_Tracer_To)
					{
					case 0: nToY = y; break;
					case 1: nToY = y + (h / 2); break;
					case 2: nToY = y + h; break;
					}

					H::DrawImGui->Line(H::DrawImGui->GetScreenW() / 2, nFromY, x + (w / 2), nToY, color);
				}

				// Name
				if (CFG::ESP_World_Name)
				{
					H::DrawImGui->String(
						H::Fonts->Get(EFonts::ESP_SMALL),
						x + (w / 2),
						(y - (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall - 1)) - SPACING_Y,
						textColor,
						POS_CENTERX,
						"health"
					);
				}

				// Box
				if (CFG::ESP_World_Box)
				{
					H::DrawImGui->OutlinedRect(x, y, w, h, color);
					H::DrawImGui->OutlinedRect(x - 1, y - 1, w + 2, h + 2, CFG::Color_ESP_Outline);
				}
			}
		}

		// Draw ammo packs
		if (!CFG::ESP_World_Ignore_AmmoPacks)
		{
			Color_t color = ApplyAlpha(CFG::Color_AmmoPack, CFG::ESP_World_Alpha);
			Color_t textColor = ApplyAlpha(CFG::ESP_Text_Color == 0 ? CFG::Color_AmmoPack : CFG::Color_ESP_Text, CFG::ESP_World_Alpha);

			for (auto pEntity : H::Entities->GetGroup(EEntGroup::AMMOPACKS))
			{
				if (!pEntity || !GetDrawBounds(pEntity, x, y, w, h))
					continue;

				// Tracer
				if (CFG::ESP_World_Tracer)
				{
					int nFromY = 0;
					switch (CFG::ESP_Tracer_From)
					{
					case 0: nFromY = 0; break;
					case 1: nFromY = H::DrawImGui->GetScreenH() / 2; break;
					case 2: nFromY = H::DrawImGui->GetScreenH(); break;
					}

					int nToY = 0;
					switch (CFG::ESP_Tracer_To)
					{
					case 0: nToY = y; break;
					case 1: nToY = y + (h / 2); break;
					case 2: nToY = y + h; break;
					}

					H::DrawImGui->Line(H::DrawImGui->GetScreenW() / 2, nFromY, x + (w / 2), nToY, color);
				}

				// Name
				if (CFG::ESP_World_Name)
				{
					H::DrawImGui->String(
						H::Fonts->Get(EFonts::ESP_SMALL),
						x + (w / 2),
						(y - (H::Fonts->Get(EFonts::ESP_SMALL).m_nTall - 1)) - SPACING_Y,
						textColor,
						POS_CENTERX,
						"ammo"
					);
				}

				// Box
				if (CFG::ESP_World_Box)
				{
					H::DrawImGui->OutlinedRect(x, y, w, h, color);
					H::DrawImGui->OutlinedRect(x - 1, y - 1, w + 2, h + 2, CFG::Color_ESP_Outline);
				}
			}
		}
	}
}

void CESP::RunImGui()
{
	// DON'T update spectated player cache - this causes jitter
	// The spectated player will use the standard FRAME_RENDER_START cache
	// Minor jitter is acceptable trade-off vs flicker

	// Additional safety check during unload
	if (!CFG::ESP_Active || I::EngineVGui->IsGameUIVisible() || SDKUtils::BInEndOfMatch() || F::SpyCamera->IsRendering())
		return;

	// Safety check: ensure interfaces are still valid during unloading
	if (!I::EngineClient || !I::GlobalVars || !H::Entities || !H::DrawImGui)
		return;

	if (CFG::Misc_Clean_Screenshot && I::EngineClient->IsTakingScreenshot())
	{
		return;
	}

	// Basic connection state validation
	if (!I::EngineClient->IsInGame() || !I::EngineClient->IsConnected())
		return;

	auto pLocal = H::Entities->GetLocal();

	if (!pLocal)
		return;

	// Relaxed pre-match validation - allow ESP in most spectator modes except pure spectator
	if (pLocal->m_iObserverMode() == OBS_MODE_FREEZECAM)
		return; // Skip frozen camera (likely transition state)

	// Allow ESP in: deathcam, fixed, in-eye, chase, poi, roaming - useful spectator modes

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	H::DrawImGui->SetDrawList(drawList);

	DrawWorldESP();

	// Draw players
	if (CFG::ESP_Players_Active)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::PLAYERS_ALL))
		{
			if (!pEntity) continue;

			auto pPlayer = pEntity->As<C_TFPlayer>();
			if (pPlayer->deadflag()) continue;

			bool bIsLocal = pPlayer == pLocal;
			bool bIsFriend = pPlayer->IsPlayerOnSteamFriendsList();

			if ((CFG::ESP_Players_Ignore_Local && bIsLocal) || (!I::Input->CAM_IsThirdPerson() && bIsLocal))
				continue;

			if (CFG::ESP_Players_Ignore_Friends && bIsFriend)
				continue;

			if (!bIsLocal)
			{
				if (!bIsFriend)
				{
					if (CFG::ESP_Players_Ignore_Teammates && pPlayer->m_iTeamNum() == pLocal->m_iTeamNum())
					{
						if (CFG::ESP_Players_Show_Teammate_Medics)
						{
							if (pPlayer->m_iClass() != TF_CLASS_MEDIC)
								continue;
						}
						else
							continue;
					}

					if (CFG::ESP_Players_Ignore_Enemies && pPlayer->m_iTeamNum() != pLocal->m_iTeamNum())
						continue;
				}

				if (CFG::ESP_Players_Ignore_Invisible && pPlayer->m_flInvisibility() >= 1.0f)
					continue;
			}

			int x = 0, y = 0, w = 0, h = 0;

			if (!GetDrawBounds(pPlayer, x, y, w, h))
				continue;

			DrawPlayerESP(pPlayer, pLocal, x, y, w, h);
		}
	}

	// Draw buildings
	if (CFG::ESP_Buildings_Active)
	{
		for (auto pEntity : H::Entities->GetGroup(EEntGroup::BUILDINGS_ALL))
		{
			if (!pEntity) continue;

			auto pBuilding = pEntity->As<C_BaseObject>();

			// Check if building is being built, placed, or carried
			if (pBuilding->m_bBuilding() || pBuilding->m_bPlacing() || pBuilding->m_bCarried())
				continue;

			if (CFG::ESP_Buildings_Ignore_Teammates && pBuilding->m_iTeamNum() == pLocal->m_iTeamNum())
				continue;

			DrawBuildingESP(pBuilding, pLocal);
		}
	}

	// Draw projectiles (part of world ESP)
	if (CFG::ESP_World_Active)
	{
		bool bIgnoringAllProjectiles = CFG::ESP_World_Ignore_LocalProjectiles
			&& CFG::ESP_World_Ignore_EnemyProjectiles
			&& CFG::ESP_World_Ignore_TeammateProjectiles;

		if (!bIgnoringAllProjectiles)
		{
			for (auto pEntity : H::Entities->GetGroup(EEntGroup::PROJECTILES_ALL))
			{
				if (!pEntity) continue;

				DrawProjectileESP(pEntity, pLocal);
			}
		}
	}
}
