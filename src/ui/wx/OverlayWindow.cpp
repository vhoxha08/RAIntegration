#include "OverlayWindow.hh"

#include "services/ServiceLocator.hh"

#include "ui/OverlayTheme.hh"
#include "ui/drawing/RecordingSurface.hh"
#include "ui/viewmodels/OverlayManager.hh"
#include "ui/wx/WxApp.hh"

#include <wx/app.h>
#include <wx/dcclient.h>

#include <atomic>
#include <memory>

namespace ra {
namespace ui {
namespace wx {

#ifdef __WXOSX__
// OverlayWindowMac.mm
void PrepareOverlayNSWindow(WXWindow pWindow);
#endif

static OverlayWindow* s_pOverlay = nullptr; // main thread only
static std::atomic<bool> s_bRepaintPending{false};

void OverlayWindow::Attach(wxWindow* pHost)
{
    if (s_pOverlay != nullptr && s_pOverlay->m_pHost == pHost)
        return;

    Detach();

    if (pHost == nullptr || !EnsureWxInitialized())
        return;

    s_pOverlay = new OverlayWindow(pHost);

    // the overlay manager may ask from any thread (i.e. the emulator's); the window lives on the main thread
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.SetRenderRequestHandler([]() { RequestRepaint(); });
    pOverlayManager.SetShowRequestHandler([]() {
        wxTheApp->CallAfter([]() {
            if (s_pOverlay != nullptr)
            {
                s_pOverlay->UpdatePosition();
                s_pOverlay->ShowWithoutActivating();
            }
        });
    });
    pOverlayManager.SetHideRequestHandler([]() {
        wxTheApp->CallAfter([]() {
            if (s_pOverlay != nullptr)
                s_pOverlay->Hide();
        });
    });
}

void OverlayWindow::Detach()
{
    if (s_pOverlay != nullptr)
        delete s_pOverlay; // (the destructor stops the overlay manager's requests)
}

void OverlayWindow::RequestRepaint()
{
    // once, until the repaint happens
    if (s_bRepaintPending.exchange(true))
        return;

    wxTheApp->CallAfter([]() {
        s_bRepaintPending = false;
        if (s_pOverlay != nullptr)
            s_pOverlay->Refresh(false);
    });
}

OverlayWindow::OverlayWindow(wxWindow* pHost) :
    m_pHost(pHost),
    m_pHostFrame(wxGetTopLevelParent(pHost))
{
    // a child of the host's window: it stays above it, moves into full screen with it, and goes away with it
    SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
    Create(m_pHostFrame, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
           wxBORDER_NONE | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT);
#ifdef __WXOSX__
    PrepareOverlayNSWindow(GetWXWindow());
#endif

    if (ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>().Transparent())
        SetTransparent(255 * 90 / 100); // 90% opacity, like the Win32 overlay

    Bind(wxEVT_PAINT, &OverlayWindow::OnPaint, this);

    // (bound with this window as the handler, so they're unbound when it's destroyed)
    m_pHostFrame->Bind(wxEVT_MOVE, &OverlayWindow::OnHostChanged, this);
    m_pHost->Bind(wxEVT_SIZE, &OverlayWindow::OnHostChanged, this);

    UpdatePosition();
}

OverlayWindow::~OverlayWindow()
{
    // the host's window destroys this one with it (it's a child)
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
}

void OverlayWindow::OnHostChanged(wxEvent& event)
{
    UpdatePosition();
    event.Skip();
}

void OverlayWindow::UpdatePosition()
{
    // over the host's client area (the game)
    const wxPoint pOrigin = m_pHost->ClientToScreen(wxPoint(0, 0));
    const wxSize szClient = m_pHost->GetClientSize();
    if (GetPosition() != pOrigin || GetSize() != szClient)
    {
        SetSize(pOrigin.x, pOrigin.y, szClient.GetWidth(), szClient.GetHeight());
        Refresh(false);
    }
}

void OverlayWindow::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    std::unique_ptr<wxGraphicsContext> pContext(wxGraphicsContext::Create(dc));
    if (!pContext)
        return;

    // start over from a fully transparent window
    const wxSize szClient = GetClientSize();
    pContext->SetCompositionMode(wxCOMPOSITION_CLEAR);
    pContext->DrawRectangle(0, 0, szClient.GetWidth(), szClient.GetHeight());
    pContext->SetCompositionMode(wxCOMPOSITION_OVER);

    // the overlay manager animates the popups as it renders them; it asks for the next repaint if they move
    ra::ui::drawing::RecordingSurface pSurface(szClient.GetWidth(), szClient.GetHeight(), true);
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.Render(pSurface, true);

    m_pRenderer.Draw(*pContext, pSurface.GetOps(), GetDPIScaleFactor());
}

} // namespace wx
} // namespace ui
} // namespace ra
