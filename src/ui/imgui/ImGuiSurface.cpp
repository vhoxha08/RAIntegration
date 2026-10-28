#include "ImGuiSurface.hh"

#include "ui/IDesktop.hh"
#include "ui/imgui/ImGuiImageRepository.hh"
#include "ui/imgui/ImGuiTextures.hh"

#include "services/ServiceLocator.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <mutex>
#include <cfloat>
#include <cmath>

namespace ra {
namespace ui {
namespace imgui {

static bool CanUseImGui()
{
    // ImGui is not thread safe; only measure text with it on the UI thread, and only while the host is building a
    // frame (surfaces may also be measured by other UI code, i.e. native windows painting outside of a frame)
    if (ImGui::GetCurrentContext() == nullptr || !ImGui::GetCurrentContext()->WithinFrameScope ||
        !ra::services::ServiceLocator::Exists<ra::ui::IDesktop>())
        return false;

    return ra::services::ServiceLocator::Get<ra::ui::IDesktop>().IsOnUIThread();
}

// Text as UTF-8 for ImGui. U+2BC7/U+2BC8 (the overlay's page arrows) are missing from the fonts on many systems
// (macOS has none), so use the older, near identical triangles U+25C0/U+25B6 instead.
static std::string ToDisplayText(const std::wstring& sText)
{
    std::wstring sDisplay = sText;
    for (auto& c : sDisplay)
    {
        if (c == 0x2BC7)
            c = 0x25C0;
        else if (c == 0x2BC8)
            c = 0x25B6;
    }

    return ra::util::String::Narrow(sDisplay);
}

struct ImGuiSurface::PixelImage
{
    std::vector<uint32_t> vPixels;
    int nWidth = 0;
    int nHeight = 0;
    mutable uintptr_t nTexture = 0;

    PixelImage() noexcept = default;
    PixelImage(const PixelImage&) = delete;
    PixelImage& operator=(const PixelImage&) = delete;
    PixelImage(PixelImage&&) = delete;
    PixelImage& operator=(PixelImage&&) = delete;

    ~PixelImage() noexcept
    {
        if (nTexture != 0)
            ImGuiImageRepository::DestroyTexture(nTexture);
    }
};

ImGuiSurface::ImGuiSurface(int nWidth, int nHeight, bool bTransparent) noexcept :
    m_nWidth(std::max(nWidth, 0)),
    m_nHeight(std::max(nHeight, 0)),
    m_bTransparent(bTransparent)
{
}

ImGuiSurface::Op& ImGuiSurface::AddOp(OpType nType, int nX, int nY, int nWidth, int nHeight)
{
    Op& pOp = m_vOps.emplace_back();
    pOp.nSequence = m_nNextSequence++;
    pOp.nType = nType;
    pOp.fX = static_cast<float>(nX);
    pOp.fY = static_cast<float>(nY);
    pOp.fWidth = static_cast<float>(nWidth);
    pOp.fHeight = static_cast<float>(nHeight);
    pOp.fClipLeft = 0.0f;
    pOp.fClipTop = 0.0f;
    pOp.fClipRight = static_cast<float>(m_nWidth);
    pOp.fClipBottom = static_cast<float>(m_nHeight);
    return pOp;
}

void ImGuiSurface::AddOp(Op&& pOp)
{
    if (pOp.nType == OpType::Rectangle && pOp.fAlpha >= 1.0f && (pOp.nColor >> 24) == 0xFF)
        RemoveCoveredOps(pOp);

    pOp.nSequence = m_nNextSequence++;
    m_vOps.push_back(std::move(pOp));
}

void ImGuiSurface::DiscardOps(uint64_t nSequence) const
{
    if (m_vOps.empty() || m_vOps.front().nSequence > nSequence)
        return;

    if (m_vOps.back().nSequence <= nSequence)
    {
        m_vOps.clear();
        return;
    }

    const auto pFirstKept = std::find_if(m_vOps.begin(), m_vOps.end(),
                                         [nSequence](const Op& pOp) { return pOp.nSequence > nSequence; });
    m_vOps.erase(m_vOps.begin(), pFirstKept);
}

void ImGuiSurface::RemoveCoveredOps(const Op& pOpaqueOp)
{
    // Views like the memory viewer draw once and then only repaint what changed, each repaint starting with an
    // opaque background. Drop what such a fill hides so the recording doesn't grow with every repaint.
    const float fLeft = std::max(pOpaqueOp.fX, pOpaqueOp.fClipLeft);
    const float fTop = std::max(pOpaqueOp.fY, pOpaqueOp.fClipTop);
    const float fRight = std::min(pOpaqueOp.fX + pOpaqueOp.fWidth, pOpaqueOp.fClipRight);
    const float fBottom = std::min(pOpaqueOp.fY + pOpaqueOp.fHeight, pOpaqueOp.fClipBottom);
    if (fLeft >= fRight || fTop >= fBottom)
        return;

    m_vOps.erase(std::remove_if(m_vOps.begin(), m_vOps.end(),
                                [fLeft, fTop, fRight, fBottom](const Op& pOp) {
                                    const float fOpLeft = std::max(pOp.fX, pOp.fClipLeft);
                                    const float fOpTop = std::max(pOp.fY, pOp.fClipTop);
                                    const float fOpRight = std::min(pOp.fX + pOp.fWidth, pOp.fClipRight);
                                    const float fOpBottom = std::min(pOp.fY + pOp.fHeight, pOp.fClipBottom);
                                    return fOpLeft >= fLeft && fOpTop >= fTop && fOpRight <= fRight &&
                                           fOpBottom <= fBottom;
                                }),
                 m_vOps.end());
}

float ImGuiSurface::GetAlpha(Color nColor) const noexcept
{
    // like GDI, only surfaces created as transparent honor the alpha channel
    return m_bTransparent ? nColor.Channel.A / 255.0f : 1.0f;
}

void ImGuiSurface::FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor)
{
    if (nColor.ARGB == Color::Transparent.ARGB)
    {
        // Color::Transparent is a color key that erases. Erasing everything is common (start over); partial
        // erases can't be expressed in a recording, but the whole frame is redrawn each time anyway.
        if (nX <= 0 && nY <= 0 && nX + nWidth >= m_nWidth && nY + nHeight >= m_nHeight)
            m_vOps.clear();
        return;
    }

    Op pOp;
    pOp.nType = OpType::Rectangle;
    pOp.fX = static_cast<float>(nX);
    pOp.fY = static_cast<float>(nY);
    pOp.fWidth = static_cast<float>(nWidth);
    pOp.fHeight = static_cast<float>(nHeight);
    pOp.fClipLeft = 0.0f;
    pOp.fClipTop = 0.0f;
    pOp.fClipRight = static_cast<float>(m_nWidth);
    pOp.fClipBottom = static_cast<float>(m_nHeight);
    pOp.nColor = nColor.ARGB;
    pOp.fAlpha = GetAlpha(nColor);
    if (!m_bTransparent)
        pOp.nColor |= 0xFF000000; // opaque surfaces ignore the alpha channel
    AddOp(std::move(pOp));
}

static ImGuiSurface::TextMeasurer s_fTextMeasurer;

void ImGuiSurface::SetTextMeasurer(TextMeasurer fMeasurer) { s_fTextMeasurer = std::move(fMeasurer); }

static std::mutex s_pFontsMutex;
static std::vector<ImGuiSurface::Font> s_vFonts;

int ImGuiSurface::LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle)
{
    // ImGui draws everything with its own font at the requested size; the face is kept for other renderers
    const bool bBold = (static_cast<int>(nStyle) & static_cast<int>(FontStyles::Bold)) != 0;

    std::lock_guard<std::mutex> lock(s_pFontsMutex);
    for (size_t i = 0; i < s_vFonts.size(); ++i)
    {
        const auto& pFont = s_vFonts.at(i);
        if (pFont.fSize == static_cast<float>(nFontSize) && pFont.bBold == bBold && pFont.sFace == sFont)
            return gsl::narrow_cast<int>(i) + 1;
    }

    s_vFonts.push_back({sFont, static_cast<float>(nFontSize), bBold});
    return gsl::narrow_cast<int>(s_vFonts.size());
}

ImGuiSurface::Font ImGuiSurface::GetFont(int nFont)
{
    std::lock_guard<std::mutex> lock(s_pFontsMutex);
    if (nFont > 0 && nFont <= gsl::narrow_cast<int>(s_vFonts.size()))
        return s_vFonts.at(gsl::narrow_cast<size_t>(nFont) - 1);

    return {};
}

ra::ui::Size ImGuiSurface::MeasureText(int nFont, const std::wstring& sText) const
{
    const Font pFont = GetFont(nFont);
    const float fSize = pFont.fSize;
    const bool bBold = pFont.bBold;

    float fWidth = 0.0f;
    if (CanUseImGui())
    {
        const std::string sUtf8 = ToDisplayText(sText);
        fWidth = ImGui::GetFont()->CalcTextSizeA(fSize, FLT_MAX, 0.0f, sUtf8.c_str()).x;
    }
    else if (s_fTextMeasurer && ra::services::ServiceLocator::Exists<ra::ui::IDesktop>() &&
             ra::services::ServiceLocator::Get<ra::ui::IDesktop>().IsOnUIThread())
    {
        return s_fTextMeasurer(pFont.sFace, fSize, bBold, sText);
    }
    else
    {
        fWidth = sText.length() * fSize * 0.55f; // estimate
    }

    if (bBold)
        fWidth += 1.0f;

    return {static_cast<int>(std::ceil(fWidth)), static_cast<int>(std::ceil(fSize))};
}

void ImGuiSurface::WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText)
{
    if (sText.empty())
        return;

    const auto pSize = MeasureText(nFont, sText);
    Op& pOp = AddOp(OpType::Text, nX, nY, pSize.Width, pSize.Height);
    pOp.nColor = nColor.ARGB;
    pOp.fAlpha = GetAlpha(nColor);
    const Font pFont = GetFont(nFont);
    pOp.fFontSize = pFont.fSize;
    pOp.bBold = pFont.bBold;
    pOp.sFontFace = pFont.sFace;
    pOp.sText = ToDisplayText(sText);
}

void ImGuiSurface::DrawImage(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage)
{
    if (pImage.Type() == ImageType::None)
        return;

    Op& pOp = AddOp(OpType::Image, nX, nY, nWidth, nHeight);
    pOp.nImageType = pImage.Type();
    pOp.sImageName = pImage.Name();
    pOp.bStretch = false;
}

void ImGuiSurface::DrawImageStretched(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage)
{
    if (pImage.Type() == ImageType::None)
        return;

    Op& pOp = AddOp(OpType::Image, nX, nY, nWidth, nHeight);
    pOp.nImageType = pImage.Type();
    pOp.sImageName = pImage.Name();
    pOp.bStretch = true;
}

void ImGuiSurface::DrawSurface(int nX, int nY, const ISurface& pSurface)
{
    DrawSurface(nX, nY, pSurface, 0, 0, gsl::narrow_cast<int>(pSurface.GetWidth()),
                gsl::narrow_cast<int>(pSurface.GetHeight()));
}

void ImGuiSurface::DrawSurface(int nX, int nY, const ISurface& pSurface, int nSurfaceX, int nSurfaceY, int nWidth,
                               int nHeight)
{
    const auto* pSource = dynamic_cast<const ImGuiSurface*>(&pSurface);
    if (pSource == nullptr || pSource == this)
        return;

    // copy the source's commands so they draw the requested region at (nX, nY), clipped to it
    const float fOffsetX = static_cast<float>(nX - nSurfaceX);
    const float fOffsetY = static_cast<float>(nY - nSurfaceY);
    const float fLeft = static_cast<float>(nX);
    const float fTop = static_cast<float>(nY);
    const float fRight = static_cast<float>(nX + nWidth);
    const float fBottom = static_cast<float>(nY + nHeight);

    for (const auto& pSourceOp : pSource->m_vOps)
    {
        Op pOp = pSourceOp;
        pOp.fX += fOffsetX;
        pOp.fY += fOffsetY;
        pOp.fClipLeft = std::max(pOp.fClipLeft + fOffsetX, fLeft);
        pOp.fClipTop = std::max(pOp.fClipTop + fOffsetY, fTop);
        pOp.fClipRight = std::min(pOp.fClipRight + fOffsetX, fRight);
        pOp.fClipBottom = std::min(pOp.fClipBottom + fOffsetY, fBottom);
        if (pOp.fClipLeft < pOp.fClipRight && pOp.fClipTop < pOp.fClipBottom)
            AddOp(std::move(pOp));
    }
}

void ImGuiSurface::SetOpacity(double fAlpha)
{
    // sets the alpha of everything drawn so far (not cumulative), like the GDI implementation
    const float fNewAlpha = static_cast<float>(std::clamp(fAlpha, 0.0, 1.0));
    for (auto& pOp : m_vOps)
        pOp.fAlpha = fNewAlpha;
}

void ImGuiSurface::SetPixels(int nX, int nY, int nWidth, int nHeight, uint32_t* pARGB)
{
    if (pARGB == nullptr || nWidth <= 0 || nHeight <= 0)
        return;

    auto pPixels = std::make_shared<PixelImage>();
    pPixels->nWidth = nWidth;
    pPixels->nHeight = nHeight;
    pPixels->vPixels.assign(pARGB, pARGB + static_cast<size_t>(nWidth) * nHeight);

    Op& pOp = AddOp(OpType::Pixels, nX, nY, nWidth, nHeight);
    pOp.pPixels = std::move(pPixels);
}

static ImU32 ToImColor(uint32_t nARGB, float fAlpha)
{
    const unsigned nAlpha = static_cast<unsigned>(std::clamp(fAlpha, 0.0f, 1.0f) * 255.0f + 0.5f);
    return IM_COL32((nARGB >> 16) & 0xFF, (nARGB >> 8) & 0xFF, nARGB & 0xFF, nAlpha);
}

void ImGuiSurface::Replay(ImDrawList& pDrawList, float fX, float fY) const
{
    auto* pRepository = ImGuiImageRepository::Get();
    const auto& pTextures = GetTextureHandlers();

    for (const auto& pOp : m_vOps)
    {
        if (pOp.fAlpha <= 0.0f)
            continue;

        const ImVec2 pMin(fX + pOp.fX, fY + pOp.fY);
        const ImVec2 pMax(pMin.x + pOp.fWidth, pMin.y + pOp.fHeight);
        pDrawList.PushClipRect(ImVec2(fX + pOp.fClipLeft, fY + pOp.fClipTop),
                               ImVec2(fX + pOp.fClipRight, fY + pOp.fClipBottom), true);

        switch (pOp.nType)
        {
            case OpType::Rectangle:
                pDrawList.AddRectFilled(pMin, pMax, ToImColor(pOp.nColor, pOp.fAlpha));
                break;

            case OpType::Text:
            {
                const ImU32 nColor = ToImColor(pOp.nColor, pOp.fAlpha);
                pDrawList.AddText(ImGui::GetFont(), pOp.fFontSize, pMin, nColor, pOp.sText.c_str());
                if (pOp.bBold) // the default font has no bold face
                    pDrawList.AddText(ImGui::GetFont(), pOp.fFontSize, ImVec2(pMin.x + 1.0f, pMin.y), nColor,
                                      pOp.sText.c_str());
                break;
            }

            case OpType::Image:
            {
                int nImageWidth = 0, nImageHeight = 0;
                const uintptr_t nTexture = pRepository ?
                    pRepository->GetTexture(pOp.nImageType, pOp.sImageName, &nImageWidth, &nImageHeight) : 0;
                if (nTexture == 0)
                    break;

                const ImVec2 pImageMax = pOp.bStretch ? pMax :
                    ImVec2(pMin.x + static_cast<float>(nImageWidth), pMin.y + static_cast<float>(nImageHeight));
                if (!pOp.bStretch) // unstretched images are cropped to the requested area
                    pDrawList.PushClipRect(pMin, pMax, true);
                pDrawList.AddImage(static_cast<ImTextureID>(nTexture), pMin, pImageMax, ImVec2(0, 0), ImVec2(1, 1),
                                   IM_COL32(255, 255, 255, static_cast<int>(pOp.fAlpha * 255.0f)));
                if (!pOp.bStretch)
                    pDrawList.PopClipRect();
                break;
            }

            case OpType::Pixels:
            {
                auto& pPixels = *pOp.pPixels;
                if (pPixels.nTexture == 0 && pTextures.fpCreateFromPixels)
                    pPixels.nTexture = pTextures.fpCreateFromPixels(pPixels.vPixels.data(), pPixels.nWidth, pPixels.nHeight);

                if (pPixels.nTexture != 0)
                    pDrawList.AddImage(static_cast<ImTextureID>(pPixels.nTexture), pMin, pMax, ImVec2(0, 0),
                                       ImVec2(1, 1), IM_COL32(255, 255, 255, static_cast<int>(pOp.fAlpha * 255.0f)));
                break;
            }
        }

        pDrawList.PopClipRect();
    }
}

bool ImGuiSurfaceFactory::SaveImage([[maybe_unused]] const ra::ui::drawing::ISurface& pSurface,
                                    const std::wstring& sPath) const
{
    RA_LOG_WARN("Saving images is not supported by the ImGui backend: %s", ra::util::String::Narrow(sPath).c_str());
    return false;
}

} // namespace imgui
} // namespace ui
} // namespace ra
