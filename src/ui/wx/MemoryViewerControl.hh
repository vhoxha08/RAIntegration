#ifndef RA_UI_WX_MEMORY_VIEWER_CONTROL_HH
#define RA_UI_WX_MEMORY_VIEWER_CONTROL_HH
#pragma once

#include "ui/viewmodels/MemoryViewerViewModel.hh"

#include <wx/bitmap.h>
#include <wx/window.h>


namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 MemoryViewerControl (and its MemoryViewerControlBinding). The view model renders
/// the memory onto a surface; this control paints that surface and forwards input to the view model.
/// </summary>
class MemoryViewerControl : public wxWindow,
                            protected ra::ui::viewmodels::MemoryViewerViewModel::RepaintNotifyTarget
{
public:
    MemoryViewerControl(wxWindow* pParent, ra::ui::viewmodels::MemoryViewerViewModel& vmViewer);
    ~MemoryViewerControl() override;
    MemoryViewerControl(const MemoryViewerControl&) noexcept = delete;
    MemoryViewerControl& operator=(const MemoryViewerControl&) noexcept = delete;
    MemoryViewerControl(MemoryViewerControl&&) noexcept = delete;
    MemoryViewerControl& operator=(MemoryViewerControl&&) noexcept = delete;

    bool AcceptsFocus() const override { return true; }

protected:
    void OnRepaintMemoryViewer() override { Refresh(false); }

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnChar(wxKeyEvent& event);

    bool HandleNavigation(const wxKeyEvent& event);
    bool HandleShortcut(const wxKeyEvent& event);

    void UpdateBackingImage();

    ra::ui::viewmodels::MemoryViewerViewModel& m_vmViewer;

    // what has been drawn so far (the view model only repaints what changed, like into a GDI bitmap)
    wxBitmap m_pBacking;
    const void* m_pBackingSurface = nullptr;
    uint64_t m_nBackingSequence = 0;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_MEMORY_VIEWER_CONTROL_HH
