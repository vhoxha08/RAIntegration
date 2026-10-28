#include "MemoryViewerControl.hh"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#include "ui/EditorTheme.hh"
#include "ui/drawing/RecordingSurface.hh"
#include "ui/wx/WxApp.hh"

#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/settings.h>
#include <wx/thread.h>

#include <algorithm>
#include <cstring>
#include <memory>

namespace ra {
namespace ui {
namespace wx {

constexpr int MEMVIEW_MARGIN = 4;

static wxColour ToColour(uint32_t nARGB)
{
    return wxColour((nARGB >> 16) & 0xFF, (nARGB >> 8) & 0xFF, nARGB & 0xFF);
}

MemoryViewerControl::MemoryViewerControl(wxWindow* pParent, ra::ui::viewmodels::MemoryViewerViewModel& vmViewer) :
    wxWindow(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_THEME),
    m_vmViewer(vmViewer)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_vmViewer.AddRepaintNotifyTarget(*this);

    Bind(wxEVT_PAINT, &MemoryViewerControl::OnPaint, this);
    Bind(wxEVT_SYS_COLOUR_CHANGED, [this](wxSysColourChangedEvent& event) {
        // the dialog applied the colors for the new appearance first
        m_vmViewer.OnThemeChanged();
        Refresh(false);
        event.Skip();
    });
    Bind(wxEVT_SIZE, &MemoryViewerControl::OnSize, this);
    Bind(wxEVT_LEFT_DOWN, &MemoryViewerControl::OnLeftDown, this);
    Bind(wxEVT_MOUSEWHEEL, &MemoryViewerControl::OnMouseWheel, this);
    Bind(wxEVT_KEY_DOWN, &MemoryViewerControl::OnKeyDown, this);
    Bind(wxEVT_CHAR, &MemoryViewerControl::OnChar, this);
    Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent& event) {
        m_vmViewer.OnGotFocus();
        Refresh(false);
        event.Skip();
    });
    Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& event) {
        m_vmViewer.OnLostFocus();
        Refresh(false);
        event.Skip();
    });
}

void MemoryViewerControl::OnRepaintMemoryViewer()
{
    // the view model may be updated on the emulator's thread; windows may only be changed on the main thread
    if (wxIsMainThread())
        Refresh(false);
    else
        CallAfter([this]() { Refresh(false); });
}

MemoryViewerControl::~MemoryViewerControl() { m_vmViewer.RemoveRepaintNotifyTarget(*this); }

void MemoryViewerControl::UpdateBackingImage()
{
    m_vmViewer.UpdateRenderImage();
    const auto* pSurface = dynamic_cast<const ra::ui::drawing::RecordingSurface*>(&m_vmViewer.GetRenderImage());
    if (pSurface == nullptr)
        return;

    // a new surface (i.e. after resizing) starts over
    const wxSize szSurface(gsl::narrow_cast<int>(pSurface->GetWidth()), gsl::narrow_cast<int>(pSurface->GetHeight()));
    if (pSurface != m_pBackingSurface || !m_pBacking.IsOk() || m_pBacking.GetLogicalSize() != szSurface)
    {
        // (the view model renders everything into a new surface, so discarding drawn commands below is safe)
        m_pBacking.CreateWithDIPSize(szSurface, GetDPIScaleFactor());
        m_pBackingSurface = pSurface;
        m_nBackingSequence = 0;
    }

    const auto& vOps = pSurface->GetOps();
    if (vOps.empty() || vOps.back().nSequence <= m_nBackingSequence)
        return;

    // drawn with a graphics context: wxDC::DrawBitmap converts each bitmap to an NSImage, which is slow
    wxMemoryDC dc(m_pBacking);
    std::unique_ptr<wxGraphicsContext> pContext(wxGraphicsContext::Create(dc));
    if (!pContext)
        return;
    m_pRenderer.Draw(*pContext, vOps, GetDPIScaleFactor(), m_nBackingSequence);
    pContext.reset();

    m_nBackingSequence = vOps.back().nSequence;

    // everything recorded so far is in the backing image now
    pSurface->DiscardOps(m_nBackingSequence);
}

void MemoryViewerControl::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);

    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    dc.SetBackground(wxBrush(ToColour(pEditorTheme.ColorBackground().ARGB)));
    dc.Clear();

    UpdateBackingImage();
    if (m_pBacking.IsOk())
        dc.DrawBitmap(m_pBacking, MEMVIEW_MARGIN, MEMVIEW_MARGIN);

    if (HasFocus())
    {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
        dc.DrawRectangle(GetClientRect());
    }
}

void MemoryViewerControl::OnSize(wxSizeEvent& event)
{
    const wxSize szClient = GetClientSize();
    m_vmViewer.OnResized(szClient.GetWidth() - MEMVIEW_MARGIN * 2, szClient.GetHeight() - MEMVIEW_MARGIN * 2);
    Refresh(false);
    event.Skip();
}

void MemoryViewerControl::OnLeftDown(wxMouseEvent& event)
{
    SetFocus();

    if (!m_vmViewer.IsAddressFixed())
    {
        m_vmViewer.OnClick(event.GetX() - MEMVIEW_MARGIN, event.GetY() - MEMVIEW_MARGIN);
        Refresh(false);
    }
}

void MemoryViewerControl::OnMouseWheel(wxMouseEvent& event)
{
    if (m_vmViewer.IsAddressFixed())
        return;

    if (event.GetWheelRotation() > 0)
        m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() - 32);
    else if (event.GetWheelRotation() < 0)
        m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() + 32);

    Refresh(false);
}

void MemoryViewerControl::OnKeyDown(wxKeyEvent& event)
{
    bool bHandled = false;
    if (!m_vmViewer.IsAddressFixed())
        bHandled = HandleNavigation(event);
    if (!bHandled)
        bHandled = HandleShortcut(event);

    if (bHandled)
        Refresh(false);
    else
        event.Skip(); // generates wxEVT_CHAR, and lets Tab move the focus
}

void MemoryViewerControl::OnChar(wxKeyEvent& event)
{
    const auto c = event.GetUnicodeKey();
    if (c != WXK_NONE && c < 0x80 && m_vmViewer.OnChar(static_cast<char>(c)))
        Refresh(false);
    else
        event.Skip();
}

bool MemoryViewerControl::HandleNavigation(const wxKeyEvent& event)
{
    // Ctrl on Windows and Linux, Cmd on macOS
    const bool bShiftHeld = event.ShiftDown();
    const bool bControlHeld = event.ControlDown();

    switch (event.GetKeyCode())
    {
        case WXK_RIGHT:
            if (bShiftHeld || bControlHeld)
                m_vmViewer.AdvanceCursorWord();
            else
                m_vmViewer.AdvanceCursor();
            return true;

        case WXK_LEFT:
            if (bShiftHeld || bControlHeld)
                m_vmViewer.RetreatCursorWord();
            else
                m_vmViewer.RetreatCursor();
            return true;

        case WXK_DOWN:
            if (bControlHeld)
                m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() + 0x10);
            else
                m_vmViewer.AdvanceCursorLine();
            return true;

        case WXK_UP:
            if (bControlHeld)
                m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() - 0x10);
            else
                m_vmViewer.RetreatCursorLine();
            return true;

        case WXK_PAGEUP:
            m_vmViewer.RetreatCursorPage();
            return true;

        case WXK_PAGEDOWN:
            m_vmViewer.AdvanceCursorPage();
            return true;

        case WXK_HOME:
            if (bControlHeld)
            {
                m_vmViewer.SetFirstAddress(0);
                m_vmViewer.SetAddress(0);
            }
            else
            {
                m_vmViewer.SetAddress(m_vmViewer.GetAddress() & ~0x0F);
            }
            return true;

        case WXK_END:
            if (bControlHeld)
            {
                const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
                const auto nTotalBytes = gsl::narrow<ra::data::ByteAddress>(pMemoryContext.TotalMemorySize());

                m_vmViewer.SetFirstAddress(nTotalBytes & ~0x0F);
                m_vmViewer.SetAddress(nTotalBytes - 1);
            }
            else
            {
                switch (m_vmViewer.GetSize())
                {
                    case ra::data::Memory::Size::ThirtyTwoBit:
                        m_vmViewer.SetAddress((m_vmViewer.GetAddress() & ~0x0F) | 0x0C);
                        break;

                    case ra::data::Memory::Size::SixteenBit:
                        m_vmViewer.SetAddress((m_vmViewer.GetAddress() & ~0x0F) | 0x0E);
                        break;

                    default:
                        m_vmViewer.SetAddress(m_vmViewer.GetAddress() | 0x0F);
                        break;
                }
            }
            return true;

        default:
            return false;
    }
}

bool MemoryViewerControl::HandleShortcut(const wxKeyEvent& event)
{
    const int nKey = event.GetKeyCode();
    if (nKey != WXK_NUMPAD_ADD && nKey != WXK_NUMPAD_SUBTRACT)
        return false;

    // Increase/decrease by 1 or 2 on the high and lower nibble depending on the modifiers
    uint32_t nModifier = 1;
    if (event.ControlDown())
        nModifier *= 2;
    if (event.ShiftDown())
        nModifier *= 16;

    if (nKey == WXK_NUMPAD_ADD)
        m_vmViewer.IncreaseCurrentValue(nModifier);
    else
        m_vmViewer.DecreaseCurrentValue(nModifier);

    return true;
}

} // namespace wx
} // namespace ui
} // namespace ra
