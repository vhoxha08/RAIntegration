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

#include <atomic>
#include <cstring>
#include <memory>

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
    if (s_fImageHandler)
        s_fImageHandler(nullptr, 0, 0);
}

void OverlayRenderer::Render()
{
    if (m_pHost == nullptr)
        return;

    const wxSize szClient = m_pHost->GetClientSize();
    if (szClient.GetWidth() <= 0 || szClient.GetHeight() <= 0)
        return;

    // a transparent image at the display's resolution
    const double fScale = m_pHost->GetDPIScaleFactor();
    wxImage pImage(std::max(1, static_cast<int>(szClient.GetWidth() * fScale + 0.5)),
                   std::max(1, static_cast<int>(szClient.GetHeight() * fScale + 0.5)));
    pImage.InitAlpha();
    memset(pImage.GetAlpha(), 0, static_cast<size_t>(pImage.GetWidth()) * pImage.GetHeight());
    wxBitmap pBitmap(pImage, 32, fScale);

    // the overlay manager animates the popups as it renders them; it asks for the next render if they move
    ra::ui::drawing::RecordingSurface pSurface(szClient.GetWidth(), szClient.GetHeight(), true);
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.Render(pSurface, true);

    {
        wxMemoryDC dc(pBitmap);
        std::unique_ptr<wxGraphicsContext> pContext(wxGraphicsContext::Create(dc));
        if (!pContext)
            return;

        // like the Win32 overlay, a transparent theme draws everything at 90% opacity
        const bool bTranslucent = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>().Transparent();
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

    const int nWidth = pData.GetWidth();
    const int nHeight = pData.GetHeight();
    m_vPixels.resize(static_cast<size_t>(nWidth) * nHeight);
    auto* pPixel = m_vPixels.data();
    wxAlphaPixelData::Iterator pRow(pData);
    for (int y = 0; y < nHeight; ++y)
    {
        wxAlphaPixelData::Iterator pColumn = pRow;
        for (int x = 0; x < nWidth; ++x, ++pColumn)
        {
            *pPixel++ = (static_cast<uint32_t>(pColumn.Alpha()) << 24) | (static_cast<uint32_t>(pColumn.Red()) << 16) |
                        (static_cast<uint32_t>(pColumn.Green()) << 8) | pColumn.Blue();
        }
        pRow.OffsetY(pData, 1);
    }

    if (s_fImageHandler)
        s_fImageHandler(m_vPixels.data(), nWidth, nHeight);
}

} // namespace wx
} // namespace ui
} // namespace ra
