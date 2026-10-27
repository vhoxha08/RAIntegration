#include "MemoryViewerControl.hh"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#include "ui/EditorTheme.hh"
#include "ui/imgui/ImGuiSurface.hh"
#include "ui/wx/WxApp.hh"

#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>

#include <algorithm>

namespace ra {
namespace ui {
namespace wx {

constexpr int MEMVIEW_MARGIN = 4;

static wxColour ToColour(uint32_t nARGB, float fAlpha)
{
    return wxColour((nARGB >> 16) & 0xFF, (nARGB >> 8) & 0xFF, nARGB & 0xFF,
                    static_cast<unsigned char>(std::clamp(fAlpha, 0.0f, 1.0f) * 255.0f));
}

MemoryViewerControl::MemoryViewerControl(wxWindow* pParent, ra::ui::viewmodels::MemoryViewerViewModel& vmViewer) :
    wxWindow(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxBORDER_THEME),
    m_vmViewer(vmViewer)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_vmViewer.AddRepaintNotifyTarget(*this);

    Bind(wxEVT_PAINT, &MemoryViewerControl::OnPaint, this);
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

MemoryViewerControl::~MemoryViewerControl() { m_vmViewer.RemoveRepaintNotifyTarget(*this); }

void MemoryViewerControl::UpdateBackingImage()
{
    m_vmViewer.UpdateRenderImage();
    const auto* pSurface = dynamic_cast<const ra::ui::imgui::ImGuiSurface*>(&m_vmViewer.GetRenderImage());
    if (pSurface == nullptr)
        return;

    // a new surface (i.e. after resizing) starts over
    const wxSize szSurface(gsl::narrow_cast<int>(pSurface->GetWidth()), gsl::narrow_cast<int>(pSurface->GetHeight()));
    if (pSurface != m_pBackingSurface || !m_pBacking.IsOk() || m_pBacking.GetLogicalSize() != szSurface)
    {
        m_pBacking.CreateWithDIPSize(szSurface, GetDPIScaleFactor());
        m_pBackingSurface = pSurface;
        m_nBackingSequence = 0;
    }

    const auto& vOps = pSurface->GetOps();
    if (vOps.empty() || vOps.back().nSequence <= m_nBackingSequence)
        return;

    wxMemoryDC dc(m_pBacking);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBackgroundMode(wxBRUSHSTYLE_TRANSPARENT);

    using OpType = ra::ui::imgui::ImGuiSurface::OpType;
    for (const auto& pOp : vOps)
    {
        if (pOp.nSequence <= m_nBackingSequence)
            continue;

        const int nX = static_cast<int>(pOp.fX);
        const int nY = static_cast<int>(pOp.fY);
        const int nWidth = static_cast<int>(pOp.fWidth);
        const int nHeight = static_cast<int>(pOp.fHeight);
        const int nClipLeft = static_cast<int>(pOp.fClipLeft);
        const int nClipTop = static_cast<int>(pOp.fClipTop);
        const int nClipRight = static_cast<int>(pOp.fClipRight);
        const int nClipBottom = static_cast<int>(pOp.fClipBottom);
        if (nClipRight <= nClipLeft || nClipBottom <= nClipTop)
            continue;

        switch (pOp.nType)
        {
            case OpType::Rectangle:
            {
                // fills are clipped by intersecting
                const int nLeft = std::max(nX, nClipLeft);
                const int nTop = std::max(nY, nClipTop);
                const int nRight = std::min(nX + nWidth, nClipRight);
                const int nBottom = std::min(nY + nHeight, nClipBottom);
                if (nRight > nLeft && nBottom > nTop)
                {
                    dc.SetBrush(wxBrush(ToColour(pOp.nColor, pOp.fAlpha)));
                    dc.DrawRectangle(nLeft, nTop, nRight - nLeft, nBottom - nTop);
                }
                break;
            }

            case OpType::Text:
            {
                // the view model measured the text with the same font (see GetSurfaceFont); only clip when the
                // text sticks out of its clip rectangle (a character copied from the font surface)
                const bool bClip = nX < nClipLeft || nY < nClipTop || nX + nWidth > nClipRight ||
                                   nY + nHeight > nClipBottom;
                if (bClip)
                    dc.SetClippingRegion(nClipLeft, nClipTop, nClipRight - nClipLeft, nClipBottom - nClipTop);
                dc.SetFont(GetSurfaceFont(pOp.sFontFace, pOp.fFontSize, pOp.bBold));
                dc.SetTextForeground(ToColour(pOp.nColor, pOp.fAlpha));
                dc.DrawText(wxString::FromUTF8(pOp.sText), nX, nY);
                if (bClip)
                    dc.DestroyClippingRegion();
                break;
            }

            default: // the memory viewer only draws rectangles and text
                break;
        }
    }

    m_nBackingSequence = vOps.back().nSequence;
}

void MemoryViewerControl::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);

    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    dc.SetBackground(wxBrush(ToColour(pEditorTheme.ColorBackground().ARGB, 1.0f)));
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
