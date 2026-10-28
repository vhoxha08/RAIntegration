#include "OverlayRenderer.hh"

#include "services/ServiceLocator.hh"

#include "ui/OverlayTheme.hh"
#include "ui/drawing/RecordingSurface.hh"
#include "ui/viewmodels/OverlayManager.hh"
#include "ui/wx/WxApp.hh"

#include <wx/app.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/rawbmp.h>

#ifdef __WXOSX__
#include <CoreGraphics/CoreGraphics.h>
#endif

#include <algorithm>

#include <atomic>
#include <cstring>
#include <memory>
#include <unordered_map>

namespace ra {
namespace ui {
namespace wx {

static OverlayRenderer* s_pOverlay = nullptr; // main thread only
static OverlayRenderer::ImageHandler s_fImageHandler = nullptr;
static std::atomic<bool> s_bRenderPending{false};

void OverlayRenderer::SetImageHandler(ImageHandler fHandler) noexcept { s_fImageHandler = fHandler; }

void OverlayRenderer::Attach(wxWindow* pHost)
{
    if (s_pOverlay != nullptr && s_pOverlay->m_pHost == pHost)
        return;

    Detach();

    if (pHost == nullptr || !EnsureWxInitialized())
        return;

    s_pOverlay = new OverlayRenderer(pHost);

    // the overlay manager may ask from any thread (i.e. the emulator's); the overlay is drawn on the main thread
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.SetRenderRequestHandler([]() { RequestRender(); });
    pOverlayManager.SetShowRequestHandler([]() {
        wxTheApp->CallAfter([]() {
            if (s_pOverlay != nullptr)
            {
                s_pOverlay->m_bVisible = true;
                s_pOverlay->Render();
            }
        });
    });
    pOverlayManager.SetHideRequestHandler([]() {
        wxTheApp->CallAfter([]() {
            if (s_pOverlay != nullptr)
            {
                s_pOverlay->m_bVisible = false;
                s_pOverlay->Clear();
            }
        });
    });
}

void OverlayRenderer::Detach()
{
    if (s_pOverlay != nullptr)
        delete s_pOverlay; // (the destructor stops the overlay manager's requests)
}

void OverlayRenderer::RequestRender()
{
    // once, until the overlay is drawn
    if (s_bRenderPending.exchange(true))
        return;

    wxTheApp->CallAfter([]() {
        s_bRenderPending = false;
        if (s_pOverlay != nullptr && s_pOverlay->m_bVisible)
            s_pOverlay->Render();
    });
}

OverlayRenderer::OverlayRenderer(wxWindow* pHost) : m_pHost(pHost)
{
    // (bound with this as the handler, so they're unbound when it's destroyed)
    m_pHost->Bind(wxEVT_SIZE, &OverlayRenderer::OnHostSized, this);
    m_pHost->Bind(wxEVT_DESTROY, &OverlayRenderer::OnHostDestroyed, this);
}

OverlayRenderer::~OverlayRenderer()
{
    if (s_pOverlay != this)
        return;

    s_pOverlay = nullptr;
    if (ra::services::ServiceLocator::Exists<ra::ui::viewmodels::OverlayManager>())
    {
        auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
        pOverlayManager.SetRenderRequestHandler(nullptr);
        pOverlayManager.SetShowRequestHandler(nullptr);
        pOverlayManager.SetHideRequestHandler(nullptr);
    }

    if (m_bVisible)
        Clear();
}

void OverlayRenderer::OnHostSized(wxSizeEvent& event)
{
    if (m_bVisible)
        Render();

    event.Skip();
}

void OverlayRenderer::OnHostDestroyed(wxWindowDestroyEvent& event)
{
    if (event.GetEventObject() == m_pHost)
    {
        // it's going away: don't draw for it anymore
        m_pHost = nullptr;
        m_bVisible = false;
        CallAfter([]() { Detach(); });
    }

    event.Skip();
}

void OverlayRenderer::Clear()
{
    m_bHasImage = false;
    m_vDrawn.clear();
    if (s_fImageHandler)
        s_fImageHandler(nullptr, 0, 0, 0, 0, 0, 0, 0);
}

void OverlayRenderer::Render()
{
    if (m_pHost == nullptr)
        return;

    const wxSize szClient = m_pHost->GetClientSize();
    if (szClient.GetWidth() <= 0 || szClient.GetHeight() <= 0)
        return;

    // the overlay manager animates the popups as it renders them; it asks for the next render if they move
    ra::ui::drawing::RecordingSurface pSurface(szClient.GetWidth(), szClient.GetHeight(), true);
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.Render(pSurface, true);

    // what's drawn, and where
    std::vector<DrawnOp> vDrawn;
    for (const auto& pOp : pSurface.GetOps())
    {
        DrawnOp pDrawn;
        pDrawn.fLeft = std::max(pOp.fX, pOp.fClipLeft);
        pDrawn.fTop = std::max(pOp.fY, pOp.fClipTop);
        pDrawn.fRight = std::min(pOp.fX + pOp.fWidth, pOp.fClipRight);
        pDrawn.fBottom = std::min(pOp.fY + pOp.fHeight, pOp.fClipBottom);
        if (pOp.fAlpha <= 0.0f || pDrawn.fRight <= pDrawn.fLeft || pDrawn.fBottom <= pDrawn.fTop)
            continue;

        pDrawn.nHash = Hash(pOp);
        vDrawn.push_back(pDrawn);
    }
    if (vDrawn.empty())
    {
        Clear();
        return;
    }

    // Like the Win32 overlay, only what changed is drawn again (the overlay manager asks again and again while the
    // in-game overlay is shown, which gets the controller's input every frame): the parts covered by what was drawn
    // last time and isn't anymore, or what's new.
    const double fScale = m_pHost->GetDPIScaleFactor();
    const int nCanvasWidth = static_cast<int>(szClient.GetWidth() * fScale + 0.5);
    const int nCanvasHeight = static_cast<int>(szClient.GetHeight() * fScale + 0.5);
    float fLeft = 0.0f, fTop = 0.0f, fRight = 0.0f, fBottom = 0.0f;
    if (!m_bHasImage || nCanvasWidth != m_nCanvasWidth || nCanvasHeight != m_nCanvasHeight)
    {
        m_nCanvasWidth = nCanvasWidth;
        m_nCanvasHeight = nCanvasHeight;
        m_vPixels.resize(static_cast<size_t>(nCanvasWidth) * nCanvasHeight);
        fRight = static_cast<float>(szClient.GetWidth());
        fBottom = static_cast<float>(szClient.GetHeight());
    }
    else
    {
        std::unordered_map<uint64_t, int> mCounts;
        for (const auto& pDrawn : m_vDrawn)
            ++mCounts[pDrawn.nHash];
        for (const auto& pDrawn : vDrawn)
            --mCounts[pDrawn.nHash];

        const auto fAddChanged = [&mCounts, &fLeft, &fTop, &fRight, &fBottom](const DrawnOp& pDrawn) {
            if (mCounts[pDrawn.nHash] == 0)
                return;
            if (fRight <= fLeft)
            {
                fLeft = pDrawn.fLeft;
                fTop = pDrawn.fTop;
                fRight = pDrawn.fRight;
                fBottom = pDrawn.fBottom;
            }
            else
            {
                fLeft = std::min(fLeft, pDrawn.fLeft);
                fTop = std::min(fTop, pDrawn.fTop);
                fRight = std::max(fRight, pDrawn.fRight);
                fBottom = std::max(fBottom, pDrawn.fBottom);
            }
        };
        for (const auto& pDrawn : m_vDrawn)
            fAddChanged(pDrawn);
        for (const auto& pDrawn : vDrawn)
            fAddChanged(pDrawn);

        if (fRight <= fLeft)
            return; // the same picture
    }
    m_vDrawn = std::move(vDrawn);

    // in pixels of the display, with a little room for antialiasing
    const int nX = std::max(0, static_cast<int>(fLeft * fScale) - 2);
    const int nY = std::max(0, static_cast<int>(fTop * fScale) - 2);
    const int nWidth = std::min(nCanvasWidth, static_cast<int>(fRight * fScale + 0.5) + 2) - nX;
    const int nHeight = std::min(nCanvasHeight, static_cast<int>(fBottom * fScale + 0.5) + 2) - nY;
    if (nWidth <= 0 || nHeight <= 0)
        return;

    // the part is drawn again from scratch
    for (int y = nY; y < nY + nHeight; ++y)
        memset(&m_vPixels[static_cast<size_t>(y) * nCanvasWidth + nX], 0, static_cast<size_t>(nWidth) * 4);

    // like the Win32 overlay, a transparent theme draws everything at 90% opacity
    const bool bTranslucent = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>().Transparent();

#ifdef __WXOSX__
    // drawn straight into the pixels handed to the host: premultiplied 0xAARRGGBB words are what Core Graphics
    // draws as "premultiplied first, 32 bit little endian"
    CGColorSpaceRef pColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef pBitmapContext = CGBitmapContextCreate(m_vPixels.data(), nCanvasWidth, nCanvasHeight, 8,
                                                        static_cast<size_t>(nCanvasWidth) * 4, pColorSpace,
                                                        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
    CGColorSpaceRelease(pColorSpace);
    if (pBitmapContext == nullptr)
        return;

    // only the part (Core Graphics counts from the bottom left), then in points of the game area from its top left
    CGContextClipToRect(pBitmapContext, CGRectMake(nX, nCanvasHeight - nY - nHeight, nWidth, nHeight));
    CGContextTranslateCTM(pBitmapContext, 0, nCanvasHeight);
    CGContextScaleCTM(pBitmapContext, fScale, -fScale);
    if (bTranslucent) // (applied to everything drawn: a layer is slow)
        CGContextSetAlpha(pBitmapContext, 0.9);
    {
        std::unique_ptr<wxGraphicsContext> pContext(
            wxGraphicsRenderer::GetDefaultRenderer()->CreateContextFromNativeContext(pBitmapContext));
        if (pContext)
            m_pRenderer.Draw(*pContext, pSurface.GetOps(), fScale);
    }
    CGContextRelease(pBitmapContext);
#else
    // a transparent image of the part, at the display's resolution
    wxImage pImage(nWidth, nHeight);
    pImage.InitAlpha();
    memset(pImage.GetAlpha(), 0, static_cast<size_t>(nWidth) * nHeight);
    wxBitmap pBitmap(pImage, 32, fScale);
    {
        wxMemoryDC dc(pBitmap);
        std::unique_ptr<wxGraphicsContext> pContext(wxGraphicsContext::Create(dc));
        if (!pContext)
            return;
        pContext->Translate(-nX / fScale, -nY / fScale);
        if (bTranslucent)
            pContext->BeginLayer(0.9);
        m_pRenderer.Draw(*pContext, pSurface.GetOps(), fScale);
        if (bTranslucent)
            pContext->EndLayer();
    }

    // the bitmap holds premultiplied pixels
    wxAlphaPixelData pData(pBitmap);
    if (!pData)
        return;

    wxAlphaPixelData::Iterator pRow(pData);
    for (int y = 0; y < nHeight; ++y)
    {
        auto* pPixel = &m_vPixels[static_cast<size_t>(nY + y) * nCanvasWidth + nX];
        wxAlphaPixelData::Iterator pColumn = pRow;
        for (int x = 0; x < nWidth; ++x, ++pColumn)
        {
            *pPixel++ = (static_cast<uint32_t>(pColumn.Alpha()) << 24) | (static_cast<uint32_t>(pColumn.Red()) << 16) |
                        (static_cast<uint32_t>(pColumn.Green()) << 8) | pColumn.Blue();
        }
        pRow.OffsetY(pData, 1);
    }
#endif

    m_bHasImage = true;

    if (s_fImageHandler)
    {
        s_fImageHandler(&m_vPixels[static_cast<size_t>(nY) * nCanvasWidth + nX], nCanvasWidth, nCanvasWidth,
                        nCanvasHeight, nX, nY, nWidth, nHeight);
    }
}

uint64_t OverlayRenderer::Hash(const ra::ui::drawing::RecordingSurface::Op& pOp)
{
    // FNV-1a over everything that changes what's drawn
    uint64_t nHash = 14695981039346656037ULL;
    const auto fAdd = [&nHash](const void* pData, size_t nSize) {
        const auto* pBytes = static_cast<const uint8_t*>(pData);
        for (size_t i = 0; i < nSize; ++i)
            nHash = (nHash ^ pBytes[i]) * 1099511628211ULL;
    };
    const auto fAddValue = [&fAdd](const auto& tValue) { fAdd(&tValue, sizeof(tValue)); };

    fAddValue(pOp.nType);
    fAddValue(pOp.fX);
    fAddValue(pOp.fY);
    fAddValue(pOp.fWidth);
    fAddValue(pOp.fHeight);
    fAddValue(pOp.fClipLeft);
    fAddValue(pOp.fClipTop);
    fAddValue(pOp.fClipRight);
    fAddValue(pOp.fClipBottom);
    fAddValue(pOp.nColor);
    fAddValue(pOp.fAlpha);
    fAddValue(pOp.fFontSize);
    fAddValue(pOp.bBold);
    fAdd(pOp.sFontFace.data(), pOp.sFontFace.size());
    fAdd(pOp.sText.data(), pOp.sText.size());
    fAddValue(pOp.nImageType);
    fAdd(pOp.sImageName.data(), pOp.sImageName.size());
    fAddValue(pOp.bStretch);
    if (pOp.pPixels)
    {
        fAddValue(pOp.pPixels->nWidth);
        fAddValue(pOp.pPixels->nHeight);
        fAdd(pOp.pPixels->vPixels.data(), pOp.pPixels->vPixels.size() * sizeof(uint32_t));
    }

    return nHash;
}

} // namespace wx
} // namespace ui
} // namespace ra
