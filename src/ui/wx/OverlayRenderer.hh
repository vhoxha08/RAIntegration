#ifndef RA_UI_WX_OVERLAY_RENDERER_HH
#define RA_UI_WX_OVERLAY_RENDERER_HH
#pragma once

#include "ui/wx/SurfaceRenderer.hh"

#include <wx/event.h>
#include <wx/window.h>

#include <cstdint>
#include <vector>

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// Draws the overlay (popups, scoreboards, the in-game overlay) into an image the size of the host's game area, for
/// the host to draw over the game (_RA_SetOverlayImageHandler). Like the Win32 OverlayWindow, it's drawn on the main
/// thread whenever the overlay manager asks for it (from any thread).
/// </summary>
class OverlayRenderer : public wxEvtHandler
{
public:
    /// <summary>
    /// Receives the overlay: the part of the game area with something in it, at (nX, nY) from its top left corner, as
    /// premultiplied ARGB pixels (0xAARRGGBB words, rows from top to bottom); all in pixels of the display. nullptr
    /// when there's nothing to show. Called on the main thread; the pixels are only valid during the call.
    /// </summary>
    using ImageHandler = void (*)(const uint32_t* pARGB, int nX, int nY, int nWidth, int nHeight);

    static void SetImageHandler(ImageHandler fHandler) noexcept;

    /// <summary>
    /// Draws the overlay the size of <paramref name="pHost" />'s client area (the game). Main thread only; the host
    /// must be a window of the application's wxWidgets.
    /// </summary>
    static void Attach(wxWindow* pHost);

    /// <summary>
    /// Stops drawing the overlay. Main thread only.
    /// </summary>
    static void Detach();

private:
    explicit OverlayRenderer(wxWindow* pHost);
    ~OverlayRenderer() override;

    void Render();
    void Clear();
    void OnHostSized(wxSizeEvent& event);
    void OnHostDestroyed(wxWindowDestroyEvent& event);

    static void RequestRender();
    static uint64_t Signature(const std::vector<ra::ui::drawing::RecordingSurface::Op>& vOps, const wxSize& szClient,
                              double fScale);

    wxWindow* m_pHost;
    bool m_bVisible = false;
    SurfaceRenderer m_pRenderer;
    std::vector<uint32_t> m_vPixels;

    // what the host has (see Signature)
    bool m_bHasImage = false;
    uint64_t m_nSignature = 0;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_OVERLAY_RENDERER_HH
