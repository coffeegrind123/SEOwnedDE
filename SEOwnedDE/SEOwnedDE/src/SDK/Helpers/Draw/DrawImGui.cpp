#include "DrawImGui.h"
#include "../../TF2/cdll_int.h"
#include "../../TF2/ivrenderview.h"
#include "../../../App/Features/Menu/Menu.h"
#include "../../../App/Features/VisualUtils/VisualUtils.h"
#include "../../../Utils/VTFLoader/VTFLoader.h"
#include <cstdarg>
#include <cstdio>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <d3d9.h>
#include <sstream>

#pragma warning (disable : 6385)
#pragma warning (disable : 4996) // Disable deprecated function warnings

void CDrawImGui::UpdateScreenSize()
{
    const auto& displaySize = ImGui::GetIO().DisplaySize;
    m_nScreenW = static_cast<int>(displaySize.x);
    m_nScreenH = static_cast<int>(displaySize.y);
}

void CDrawImGui::UpdateW2SMatrix()
{
    const int currentFrame = I::GlobalVars->framecount;
    if (m_nLastW2SUpdateFrame == currentFrame)
        return;

    m_nLastW2SUpdateFrame = currentFrame;

    m_WorldToProjection = I::EngineClient->WorldToScreenMatrix();
}

bool CDrawImGui::W2S(const Vec3 &vOrigin, Vec3 &vScreen)
{
    const matrix3x4_t &w2s = m_WorldToProjection.As3x4();

    float w = w2s[3][0] * vOrigin[0] + w2s[3][1] * vOrigin[1] + w2s[3][2] * vOrigin[2] + w2s[3][3];

    if (w > 0.001f)
    {
        float flsw = static_cast<float>(m_nScreenW);
        float flsh = static_cast<float>(m_nScreenH);
        float fl1dbw = 1.0f / w;

        vScreen.x = (flsw / 2.0f) + (0.5f * ((w2s[0][0] * vOrigin[0] + w2s[0][1] * vOrigin[1] + w2s[0][2] * vOrigin[2] + w2s[0][3]) * fl1dbw) * flsw + 0.5f);
        vScreen.y = (flsh / 2.0f) - (0.5f * ((w2s[1][0] * vOrigin[0] + w2s[1][1] * vOrigin[1] + w2s[1][2] * vOrigin[2] + w2s[1][3]) * fl1dbw) * flsh + 0.5f);

        return true;
    }

    return false;
}

bool CDrawImGui::ClipTransformWithProjection(const matrix3x4_t &worldToScreen, const Vec3 &point, Vec3 *pClip)
{
    pClip->x = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
    pClip->y = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
    pClip->z = 0.0f;

    float w = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];

    if (w < 0.001f)
        return false;

    pClip->x = pClip->x / w;
    pClip->y = pClip->y / w;

    if (pClip->x < -1.0f || pClip->x > 1.0f || pClip->y < -1.0f || pClip->y > 1.0f)
        return false;

    pClip->x = (pClip->x * 0.5f + 0.5f) * m_nScreenW;
    pClip->y = ((1.0f - pClip->y) * 0.5f) * m_nScreenH;
    pClip->z = 1.0f / w;

    return true;
}

bool CDrawImGui::ClipTransform(const Vector &point, Vector *pClip)
{
    return ClipTransformWithProjection(m_WorldToProjection.As3x4(), point, pClip);
}

bool CDrawImGui::ScreenPosition(const Vec3 &vPoint, Vec3 &vScreen)
{
    return W2S(vPoint, vScreen);
}

ImU32 CDrawImGui::ColorToImU32(Color_t clr)
{
    return IM_COL32(clr.r, clr.g, clr.b, clr.a);
}

void CDrawImGui::String(const CFont &font, int x, int y, Color_t clr, short pos, const char *str, ...)
{
    if (!m_pDrawList) return;

    char buffer[1024];
    va_list args;
    va_start(args, str);
    vsnprintf(buffer, sizeof(buffer), str, args);
    va_end(args);

    ImVec2 textPos = ImVec2(static_cast<float>(x), static_cast<float>(y));
    ImVec2 textSize = ImGui::CalcTextSize(buffer);

    if (pos & POS_CENTERX)
        textPos.x -= textSize.x * 0.5f;
    if (pos & POS_CENTERY)
        textPos.y -= textSize.y * 0.5f;

    m_pDrawList->AddText(textPos, ColorToImU32(clr), buffer);
}

void CDrawImGui::String(const CFont &font, int x, int y, Color_t clr, short pos, const wchar_t *str, ...)
{
    if (!m_pDrawList) return;

    wchar_t buffer[1024];
    va_list args;
    va_start(args, str);
    vswprintf(buffer, sizeof(buffer) / sizeof(wchar_t), str, args);
    va_end(args);

    char utf8Buffer[1024];
    int i;
    for (i = 0; i < 1023 && buffer[i] != L'\0'; i++)
    {
        if (buffer[i] < 128)
            utf8Buffer[i] = static_cast<char>(buffer[i]);
        else
            utf8Buffer[i] = '?';
    }
    utf8Buffer[i] = '\0';

    ImVec2 textPos = ImVec2(static_cast<float>(x), static_cast<float>(y));
    ImVec2 textSize = ImGui::CalcTextSize(utf8Buffer);

    if (pos & POS_CENTERX)
        textPos.x -= textSize.x * 0.5f;
    if (pos & POS_CENTERY)
        textPos.y -= textSize.y * 0.5f;

    m_pDrawList->AddText(textPos, ColorToImU32(clr), utf8Buffer);
}

void CDrawImGui::Line(int x, int y, int x1, int y1, Color_t clr)
{
    if (!m_pDrawList) return;
    m_pDrawList->AddLine(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                        ImVec2(static_cast<float>(x1), static_cast<float>(y1)),
                        ColorToImU32(clr));
}

void CDrawImGui::Rect(int x, int y, int w, int h, Color_t clr)
{
    if (!m_pDrawList) return;
    m_pDrawList->AddRectFilled(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                              ImVec2(static_cast<float>(x + w), static_cast<float>(y + h)),
                              ColorToImU32(clr));
}

void CDrawImGui::OutlinedRect(int x, int y, int w, int h, Color_t clr)
{
    if (!m_pDrawList) return;
    m_pDrawList->AddRect(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                        ImVec2(static_cast<float>(x + w), static_cast<float>(y + h)),
                        ColorToImU32(clr));
}

void CDrawImGui::GradientRect(int x, int y, int w, int h, Color_t top_clr, Color_t bottom_clr, bool horizontal)
{
    if (!m_pDrawList) return;

    if (horizontal)
    {
        ImVec2 p1 = ImVec2(static_cast<float>(x), static_cast<float>(y));
        ImVec2 p2 = ImVec2(static_cast<float>(x + w), static_cast<float>(y));
        ImVec2 p3 = ImVec2(static_cast<float>(x + w), static_cast<float>(y + h));
        ImVec2 p4 = ImVec2(static_cast<float>(x), static_cast<float>(y + h));

        m_pDrawList->AddQuadFilled(p1, p2, p3, p4, ColorToImU32(top_clr));
        // Note: ImGui doesn't support gradient rectangles directly, so we use top color
    }
    else
    {
        ImVec2 p1 = ImVec2(static_cast<float>(x), static_cast<float>(y));
        ImVec2 p2 = ImVec2(static_cast<float>(x + w), static_cast<float>(y));
        ImVec2 p3 = ImVec2(static_cast<float>(x + w), static_cast<float>(y + h));
        ImVec2 p4 = ImVec2(static_cast<float>(x), static_cast<float>(y + h));

        m_pDrawList->AddQuadFilled(p1, p2, p3, p4, ColorToImU32(top_clr));
        // Note: ImGui doesn't support gradient rectangles directly, so we use top color
    }
}

void CDrawImGui::OutlinedCircle(int x, int y, int radius, int segments, Color_t clr)
{
    if (!m_pDrawList) return;
    m_pDrawList->AddCircle(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                          static_cast<float>(radius),
                          ColorToImU32(clr),
                          segments);
}

void CDrawImGui::FilledCircle(int x, int y, int radius, int segments, Color_t clr)
{
    if (!m_pDrawList) return;
    m_pDrawList->AddCircleFilled(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                                static_cast<float>(radius),
                                ColorToImU32(clr),
                                segments);
}

void CDrawImGui::Texture(int x, int y, int w, int h, int id, short pos)
{
    if (!m_pDrawList) return;

    const char* textureName = F::VisualUtils->GetTextureNameFromID(id);
    if (!textureName) return;

    IDirect3DTexture9* pD3DTexture = GetOrCreateD3D9Texture(id, textureName);
    if (!pD3DTexture) return;

    if (pos & POS_LEFT) x -= w;
    if (pos & POS_TOP) y -= h;
    if (pos & POS_CENTERX) x -= (w / 2);
    if (pos & POS_CENTERY) y -= (h / 2);

    m_pDrawList->AddImage(
        reinterpret_cast<ImTextureID>(pD3DTexture),
        ImVec2(static_cast<float>(x), static_cast<float>(y)),
        ImVec2(static_cast<float>(x + w), static_cast<float>(y + h)),
        ImVec2(0, 0),
        ImVec2(1, 1),
        IM_COL32_WHITE
    );
}

void CDrawImGui::Polygon(int count, Vertex_t *vertices, Color_t clr)
{
    if (!m_pDrawList || !vertices) return;

    std::vector<ImVec2> points;
    for (int i = 0; i < count; ++i)
    {
        points.push_back(ImVec2(vertices[i].m_Position.x, vertices[i].m_Position.y));
    }

    m_pDrawList->AddConvexPolyFilled(points.data(), count, ColorToImU32(clr));
}

void CDrawImGui::FilledTriangle(const std::array<Vec2, 3> &points, Color_t clr)
{
    if (!m_pDrawList) return;

    ImVec2 trianglePoints[3] = {
        ImVec2(points[0].x, points[0].y),
        ImVec2(points[1].x, points[1].y),
        ImVec2(points[2].x, points[2].y)
    };

    m_pDrawList->AddConvexPolyFilled(trianglePoints, 3, ColorToImU32(clr));
}

void CDrawImGui::Arc(int x, int y, int radius, float thickness, float start, float end, Color_t col)
{
    if (!m_pDrawList) return;

    ImVec2 center = ImVec2(static_cast<float>(x), static_cast<float>(y));
    ImU32 color = ColorToImU32(col);

    // Draw arc as a series of line segments
    const int segments = 32;
    ImVec2 prevPoint;
    bool firstPoint = true;

    for (int i = 0; i <= segments; ++i)
    {
        float angle = start + (end - start) * (static_cast<float>(i) / segments);
        ImVec2 currentPoint = ImVec2(
            center.x + radius * cosf(angle),
            center.y + radius * sinf(angle)
        );

        if (!firstPoint)
        {
            m_pDrawList->AddLine(prevPoint, currentPoint, color, thickness);
        }
        else
        {
            firstPoint = false;
        }

        prevPoint = currentPoint;
    }
}

void CDrawImGui::StartClipping(int x, int y, int w, int h)
{
    if (!m_pDrawList) return;
    m_pDrawList->PushClipRect(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                             ImVec2(static_cast<float>(x + w), static_cast<float>(y + h)));
}

void CDrawImGui::EndClipping()
{
    if (!m_pDrawList) return;
    m_pDrawList->PopClipRect();
}

void CDrawImGui::FillRectRounded(int x, int y, int w, int h, int radius, Color_t col)
{
    if (!m_pDrawList) return;
    m_pDrawList->AddRectFilled(ImVec2(static_cast<float>(x), static_cast<float>(y)),
                              ImVec2(static_cast<float>(x + w), static_cast<float>(y + h)),
                              ColorToImU32(col),
                              static_cast<float>(radius),
                              ImDrawFlags_RoundCornersAll);
}

CDrawImGui::~CDrawImGui()
{
    ClearTextureCache();
}

void CDrawImGui::ClearTextureCache()
{
    for (auto& pair : m_D3D9TextureCache)
    {
        if (pair.second)
        {
            pair.second->Release();
            pair.second = nullptr;
        }
    }
    m_D3D9TextureCache.clear();

    I::CVar->ConsoleColorPrintf({100, 255, 100, 255}, "[DrawImGui] Texture cache cleared\n");
}

IDirect3DTexture9* CDrawImGui::GetOrCreateD3D9Texture(int matSurfaceID, const char* textureName)
{
    auto it = m_D3D9TextureCache.find(matSurfaceID);
    if (it != m_D3D9TextureCache.end())
    {
        if (it->second)
        {
            D3DSURFACE_DESC desc;
            if (SUCCEEDED(it->second->GetLevelDesc(0, &desc)))
                return it->second;

            it->second->Release();
        }

        return nullptr;
    }

    if (!F::Menu->m_pDevice || !textureName)
        return nullptr;

    // I::CVar->ConsoleColorPrintf({100, 100, 255, 255}, "[VTFLoader] Attempting to load: %s\n", textureName);

    int width = 0, height = 0;
    unsigned char* pImageData = VTFLoader::ReadVTFFromVPK(textureName, &width, &height);

    if (!pImageData)
    {
        I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Failed to load %s from VPK\n", textureName);
        m_D3D9TextureCache[matSurfaceID] = nullptr;
        return nullptr;
    }

    IDirect3DTexture9* pD3DTexture = nullptr;
    if (!VTFLoader::CreateTexture(pImageData, width, height, &pD3DTexture, F::Menu->m_pDevice))
    {
        I::CVar->ConsoleColorPrintf({255, 100, 100, 255}, "[VTFLoader] Failed to create D3D9 texture for %s\n", textureName);
        VTFLoader::FreeImage(pImageData);
        m_D3D9TextureCache[matSurfaceID] = nullptr;
        return nullptr;
    }

    VTFLoader::FreeImage(pImageData);

    // I::CVar->ConsoleColorPrintf({100, 255, 100, 255}, "[VTFLoader] Successfully loaded %s (%dx%d)\n", textureName, width, height);

    m_D3D9TextureCache[matSurfaceID] = pD3DTexture;
    return pD3DTexture;
}