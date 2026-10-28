#ifndef RA_UI_WX_OVERLAY_WINDOW_HH
#define RA_UI_WX_OVERLAY_WINDOW_HH
#pragma once

#include "ui/wx/SurfaceRenderer.hh"

#include <wx/frame.h>

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// The overlay (popups, scoreboards, the in-game overlay), drawn in a transparent window over the host's game
/// area, like the Win32 OverlayWindow. Clicks go through it to the game. It follows the host's window, and is
/// painted on the main thread whenever the overlay manager asks for it (from any thread).
/// </summary>
class OverlayWindow : public wxFrame
{
public:
    /// <summary>
    /// Shows the overlay over <paramref name="pHost" />'s client area, instead of over the previous host. Main
    /// thread only; the host must be a window of the application's wxWidgets.
    /// </summary>
    static void Attach(wxWindow* pHost);

    /// <summary>
    /// Removes the overlay. Main thread only.
    /// </summary>
    static void Detach();

private:
    explicit OverlayWindow(wxWindow* pHost);
    ~OverlayWindow() override;

    void UpdatePosition();
    void OnPaint(wxPaintEvent& event);
    void OnHostChanged(wxEvent& event);

    static void RequestRepaint();

    wxWindow* m_pHost;
    wxWindow* m_pHostFrame;
    SurfaceRenderer m_pRenderer;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_OVERLAY_WINDOW_HH
