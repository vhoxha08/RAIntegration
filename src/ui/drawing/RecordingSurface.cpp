#include "RecordingSurface.hh"

#include "ui/IDesktop.hh"

#include "services/ServiceLocator.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <cfloat>
#include <cmath>

namespace ra {
namespace ui {
namespace drawing {

// Shared by all surfaces: a renderer keeping what it drew from a surface (by the last sequence it drew) must not
// mistake a new surface, which may reuse the old one's address, for the old one.
static uint64_t NextSequence() noexcept
{
    static std::atomic<uint64_t> s_nNextSequence{1};
    return s_nNextSequence++;
}


// Text as UTF-8. U+2BC7/U+2BC8 (the overlay's page arrows) are missing from the fonts on many systems (macOS has
// none), so use the older, near identical triangles U+25C0/U+25B6 instead.
static std::wstring ToDisplayTextW(const std::wstring& sText)
{
    std::wstring sDisplay = sText;
    for (auto& c : sDisplay)
    {
        if (c == 0x2BC7)
            c = 0x25C0;
        else if (c == 0x2BC8)
            c = 0x25B6;
    }

    return sDisplay;
}

static std::string ToDisplayText(const std::wstring& sText)
{
    return ra::util::String::Narrow(ToDisplayTextW(sText));
}

RecordingSurface::RecordingSurface(int nWidth, int nHeight, bool bTransparent) noexcept :
    m_nWidth(std::max(nWidth, 0)),
    m_nHeight(std::max(nHeight, 0)),
    m_bTransparent(bTransparent)
{
}

RecordingSurface::Op& RecordingSurface::AddOp(OpType nType, int nX, int nY, int nWidth, int nHeight)
{
    Op& pOp = m_vOps.emplace_back();
    pOp.nSequence = NextSequence();
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

void RecordingSurface::AddOp(Op&& pOp)
{
    if (pOp.nType == OpType::Rectangle && pOp.fAlpha >= 1.0f && (pOp.nColor >> 24) == 0xFF)
        RemoveCoveredOps(pOp);

    pOp.nSequence = NextSequence();
    m_vOps.push_back(std::move(pOp));
}

void RecordingSurface::DiscardOps(uint64_t nSequence) const
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

void RecordingSurface::RemoveCoveredOps(const Op& pOpaqueOp)
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

float RecordingSurface::GetAlpha(Color nColor) const noexcept
{
    // like GDI, only surfaces created as transparent honor the alpha channel
    return m_bTransparent ? nColor.Channel.A / 255.0f : 1.0f;
}

void RecordingSurface::FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor)
{
    if (nColor.ARGB == Color::Transparent.ARGB)
    {
        // Color::Transparent is a color key that erases (i.e. before part of the surface is drawn again): what's
        // entirely inside the area is dropped, or it would pile up under every repaint. (What only partly overlaps it
        // stays; a recording can't erase part of it.)
        Op pErase;
        pErase.nType = OpType::Rectangle;
        pErase.fX = static_cast<float>(nX);
        pErase.fY = static_cast<float>(nY);
        pErase.fWidth = static_cast<float>(nWidth);
        pErase.fHeight = static_cast<float>(nHeight);
        pErase.fClipLeft = 0.0f;
        pErase.fClipTop = 0.0f;
        pErase.fClipRight = static_cast<float>(m_nWidth);
        pErase.fClipBottom = static_cast<float>(m_nHeight);
        RemoveCoveredOps(pErase);
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

static RecordingSurface::TextMeasurer s_fTextMeasurer;

void RecordingSurface::SetTextMeasurer(TextMeasurer fMeasurer) { s_fTextMeasurer = std::move(fMeasurer); }

static std::mutex s_pFontsMutex;
static std::vector<RecordingSurface::Font> s_vFonts;

int RecordingSurface::LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle)
{
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

RecordingSurface::Font RecordingSurface::GetFont(int nFont)
{
    std::lock_guard<std::mutex> lock(s_pFontsMutex);
    if (nFont > 0 && nFont <= gsl::narrow_cast<int>(s_vFonts.size()))
        return s_vFonts.at(gsl::narrow_cast<size_t>(nFont) - 1);

    return {};
}

ra::ui::Size RecordingSurface::MeasureText(int nFont, const std::wstring& sText) const
{
    const Font pFont = GetFont(nFont);
    const float fSize = pFont.fSize;
    const bool bBold = pFont.bBold;

    if (s_fTextMeasurer && ra::services::ServiceLocator::Exists<ra::ui::IDesktop>() &&
        ra::services::ServiceLocator::Get<ra::ui::IDesktop>().IsOnUIThread())
    {
        return s_fTextMeasurer(pFont.sFace, fSize, bBold, ToDisplayTextW(sText));
    }

    float fWidth = sText.length() * fSize * 0.55f; // estimate

    if (bBold)
        fWidth += 1.0f;

    return {static_cast<int>(std::ceil(fWidth)), static_cast<int>(std::ceil(fSize))};
}

void RecordingSurface::WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText)
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

void RecordingSurface::RemoveOpsUnderImage(int nX, int nY, int nWidth, int nHeight)
{
    // like GDI, an opaque surface copies images over what's there (i.e. the in-game overlay starts every repaint with
    // its background over the whole surface): what they hide is dropped so it doesn't pile up under every repaint
    if (m_bTransparent)
        return;

    Op pImage;
    pImage.fX = static_cast<float>(nX);
    pImage.fY = static_cast<float>(nY);
    pImage.fWidth = static_cast<float>(nWidth);
    pImage.fHeight = static_cast<float>(nHeight);
    pImage.fClipLeft = 0.0f;
    pImage.fClipTop = 0.0f;
    pImage.fClipRight = static_cast<float>(m_nWidth);
    pImage.fClipBottom = static_cast<float>(m_nHeight);
    RemoveCoveredOps(pImage);
}

void RecordingSurface::DrawImage(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage)
{
    if (pImage.Type() == ImageType::None)
        return;

    RemoveOpsUnderImage(nX, nY, nWidth, nHeight);
    Op& pOp = AddOp(OpType::Image, nX, nY, nWidth, nHeight);
    pOp.nImageType = pImage.Type();
    pOp.sImageName = pImage.Name();
    pOp.bStretch = false;
}

void RecordingSurface::DrawImageStretched(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage)
{
    if (pImage.Type() == ImageType::None)
        return;

    RemoveOpsUnderImage(nX, nY, nWidth, nHeight);
    Op& pOp = AddOp(OpType::Image, nX, nY, nWidth, nHeight);
    pOp.nImageType = pImage.Type();
    pOp.sImageName = pImage.Name();
    pOp.bStretch = true;
}

void RecordingSurface::DrawSurface(int nX, int nY, const ISurface& pSurface)
{
    DrawSurface(nX, nY, pSurface, 0, 0, gsl::narrow_cast<int>(pSurface.GetWidth()),
                gsl::narrow_cast<int>(pSurface.GetHeight()));
}

void RecordingSurface::DrawSurface(int nX, int nY, const ISurface& pSurface, int nSurfaceX, int nSurfaceY, int nWidth,
                               int nHeight)
{
    const auto* pSource = dynamic_cast<const RecordingSurface*>(&pSurface);
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
        // only copy commands that draw something inside the requested region (i.e. one glyph of a font image)
        const float fClipLeft = std::max({pSourceOp.fClipLeft + fOffsetX, pSourceOp.fX + fOffsetX, fLeft});
        const float fClipTop = std::max({pSourceOp.fClipTop + fOffsetY, pSourceOp.fY + fOffsetY, fTop});
        const float fClipRight = std::min({pSourceOp.fClipRight + fOffsetX,
                                           pSourceOp.fX + pSourceOp.fWidth + fOffsetX, fRight});
        const float fClipBottom = std::min({pSourceOp.fClipBottom + fOffsetY,
                                            pSourceOp.fY + pSourceOp.fHeight + fOffsetY, fBottom});
        if (fClipLeft >= fClipRight || fClipTop >= fClipBottom)
            continue;

        Op pOp = pSourceOp;
        pOp.fX += fOffsetX;
        pOp.fY += fOffsetY;
        pOp.fClipLeft = std::max(pOp.fClipLeft + fOffsetX, fLeft);
        pOp.fClipTop = std::max(pOp.fClipTop + fOffsetY, fTop);
        pOp.fClipRight = std::min(pOp.fClipRight + fOffsetX, fRight);
        pOp.fClipBottom = std::min(pOp.fClipBottom + fOffsetY, fBottom);
        AddOp(std::move(pOp));
    }
}

void RecordingSurface::SetOpacity(double fAlpha)
{
    // sets the alpha of everything drawn so far (not cumulative), like the GDI implementation
    const float fNewAlpha = static_cast<float>(std::clamp(fAlpha, 0.0, 1.0));
    for (auto& pOp : m_vOps)
        pOp.fAlpha = fNewAlpha;
}

void RecordingSurface::SetPixels(int nX, int nY, int nWidth, int nHeight, uint32_t* pARGB)
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

bool RecordingSurfaceFactory::SaveImage([[maybe_unused]] const ISurface& pSurface, const std::wstring& sPath) const
{
    RA_LOG_WARN("Saving images is not supported: %s", ra::util::String::Narrow(sPath).c_str());
    return false;
}

} // namespace drawing
} // namespace ui
} // namespace ra
