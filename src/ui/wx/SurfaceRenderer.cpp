#include "SurfaceRenderer.hh"

#include "ui/wx/WxApp.hh"
#include "ui/wx/WxImageRepository.hh"

#include <wx/dcmemory.h>
#include <wx/image.h>

#include <algorithm>
#include <cstring>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::drawing::RecordingSurface;

static wxColour ToColour(uint32_t nARGB, float fAlpha)
{
    return wxColour((nARGB >> 16) & 0xFF, (nARGB >> 8) & 0xFF, nARGB & 0xFF,
                    static_cast<unsigned char>(std::clamp(fAlpha, 0.0f, 1.0f) * 255.0f));
}

const SurfaceRenderer::TextBitmap& SurfaceRenderer::GetTextBitmap(const std::string& sText,
                                                                  const std::string& sFontFace, float fFontSize,
                                                                  bool bBold, uint32_t nColor, double fScale)
{
    std::string sKey = sFontFace;
    sKey.append(1, '\x1F').append(std::to_string(static_cast<int>(fFontSize * 4.0f)));
    sKey.append(1, bBold ? 'B' : 'N').append(std::to_string(nColor));
    sKey.append(1, '@').append(std::to_string(static_cast<int>(fScale * 100.0))).append(1, '\x1F').append(sText);

    const auto pIter = m_mTextBitmaps.find(sKey);
    if (pIter != m_mTextBitmaps.end())
        return pIter->second;

    if (m_mTextBitmaps.size() > 4096)
        m_mTextBitmaps.clear();

    const wxFont& pFont = GetSurfaceFont(sFontFace, fFontSize, bBold);
    const wxString sWxText = wxString::FromUTF8(sText);
    wxCoord nWidth = 0, nHeight = 0;
    {
        wxMemoryDC dcMeasure;
        dcMeasure.SetFont(pFont);
        dcMeasure.GetTextExtent(sWxText, &nWidth, &nHeight);
    }

    // a transparent bitmap at the display's resolution with the text on it
    wxImage pImage(std::max(1, static_cast<int>(nWidth * fScale + 0.5)), std::max(1, static_cast<int>(nHeight * fScale + 0.5)));
    pImage.InitAlpha();
    memset(pImage.GetAlpha(), 0, static_cast<size_t>(pImage.GetWidth()) * pImage.GetHeight());
    wxBitmap pBitmap(pImage, 32, fScale);
    {
        wxMemoryDC dc(pBitmap);
        dc.SetFont(pFont);
        dc.SetTextForeground(ToColour(nColor, 1.0f));
        dc.SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);
        dc.DrawText(sWxText, 0, 0);
    }

    TextBitmap pTextBitmap;
    pTextBitmap.pBitmap = wxGraphicsRenderer::GetDefaultRenderer()->CreateBitmap(pBitmap);
    pTextBitmap.fWidth = nWidth;
    pTextBitmap.fHeight = nHeight;
    return m_mTextBitmaps.emplace(std::move(sKey), std::move(pTextBitmap)).first->second;
}

static const wxGraphicsBitmap& GetPixelsBitmap(const RecordingSurface::PixelImage& pPixels)
{
    if (!pPixels.pRendered)
    {
        wxImage pImage(pPixels.nWidth, pPixels.nHeight);
        pImage.InitAlpha();
        unsigned char* pRGB = pImage.GetData();
        unsigned char* pAlpha = pImage.GetAlpha();
        for (const auto nARGB : pPixels.vPixels)
        {
            *pRGB++ = (nARGB >> 16) & 0xFF;
            *pRGB++ = (nARGB >> 8) & 0xFF;
            *pRGB++ = nARGB & 0xFF;
            *pAlpha++ = (nARGB >> 24) & 0xFF;
        }

        pPixels.pRendered = std::make_shared<wxGraphicsBitmap>(
            wxGraphicsRenderer::GetDefaultRenderer()->CreateBitmapFromImage(pImage));
    }

    return *static_cast<const wxGraphicsBitmap*>(pPixels.pRendered.get());
}

void SurfaceRenderer::Draw(wxGraphicsContext& pContext, const std::vector<RecordingSurface::Op>& vOps,
                           double fScale, uint64_t nAfterSequence)
{
    using OpType = RecordingSurface::OpType;
    auto* pRepository = WxImageRepository::Get();
    pContext.SetPen(wxNullPen);

    for (const auto& pOp : vOps)
    {
        if (pOp.nSequence <= nAfterSequence || pOp.fAlpha <= 0.0f)
            continue;

        const double fX = pOp.fX;
        const double fY = pOp.fY;
        const double fWidth = pOp.fWidth;
        const double fHeight = pOp.fHeight;
        const double fClipLeft = pOp.fClipLeft;
        const double fClipTop = pOp.fClipTop;
        const double fClipRight = pOp.fClipRight;
        const double fClipBottom = pOp.fClipBottom;
        if (fClipRight <= fClipLeft || fClipBottom <= fClipTop)
            continue;

        if (pOp.nType == OpType::Rectangle)
        {
            // fills are clipped by intersecting
            const double fLeft = std::max(fX, fClipLeft);
            const double fTop = std::max(fY, fClipTop);
            const double fRight = std::min(fX + fWidth, fClipRight);
            const double fBottom = std::min(fY + fHeight, fClipBottom);
            if (fRight > fLeft && fBottom > fTop)
            {
                pContext.SetBrush(wxBrush(ToColour(pOp.nColor, pOp.fAlpha)));
                pContext.DrawRectangle(fLeft, fTop, fRight - fLeft, fBottom - fTop);
            }
            continue;
        }

        // bitmaps are clipped when they stick out of their clip rectangle (i.e. a character copied from a font
        // surface), and faded with a layer
        const bool bClip = fX < fClipLeft || fY < fClipTop || fX + fWidth > fClipRight || fY + fHeight > fClipBottom;
        const bool bFade = pOp.fAlpha < 1.0f;
        if (bClip)
        {
            pContext.PushState();
            pContext.Clip(fClipLeft, fClipTop, fClipRight - fClipLeft, fClipBottom - fClipTop);
        }
        if (bFade)
            pContext.BeginLayer(pOp.fAlpha);

        switch (pOp.nType)
        {
            case OpType::Text:
            {
                const auto& pText = GetTextBitmap(pOp.sText, pOp.sFontFace, pOp.fFontSize, pOp.bBold,
                                                  pOp.nColor | 0xFF000000, fScale);
                pContext.DrawBitmap(pText.pBitmap, fX, fY, pText.fWidth, pText.fHeight);
                break;
            }

            case OpType::Image:
            {
                const auto* pImage = pRepository ? pRepository->GetImage(pOp.nImageType, pOp.sImageName) : nullptr;
                if (pImage == nullptr)
                    break;

                if (pOp.bStretch)
                {
                    pContext.DrawBitmap(pImage->pBitmap, fX, fY, fWidth, fHeight);
                    break;
                }

                // like the Win32 image repository, which resizes larger images (i.e. user pictures) when loading
                // them, unstretched images that don't fit the requested area are scaled down to it; they are
                // cropped to the area
                double fImageWidth = pImage->nWidth;
                double fImageHeight = pImage->nHeight;
                if (fImageWidth > fWidth || fImageHeight > fHeight)
                {
                    const double fFit = std::min(fWidth / fImageWidth, fHeight / fImageHeight);
                    fImageWidth *= fFit;
                    fImageHeight *= fFit;
                }
                pContext.PushState();
                pContext.Clip(fX, fY, fWidth, fHeight);
                pContext.DrawBitmap(pImage->pBitmap, fX, fY, fImageWidth, fImageHeight);
                pContext.PopState();
                break;
            }

            case OpType::Pixels:
                if (pOp.pPixels)
                    pContext.DrawBitmap(GetPixelsBitmap(*pOp.pPixels), fX, fY, fWidth, fHeight);
                break;

            default:
                break;
        }

        if (bFade)
            pContext.EndLayer();
        if (bClip)
            pContext.PopState();
    }
}

} // namespace wx
} // namespace ui
} // namespace ra
