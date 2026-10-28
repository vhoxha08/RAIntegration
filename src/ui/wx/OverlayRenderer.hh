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
    /// Receives the overlay: a picture the size of the game area at the display's resolution (the canvas), of which
    /// the rectangle at (nX, nY) changed: premultiplied ARGB pixels (0xAARRGGBB words, rows from top to bottom,
    /// nStride words apart) starting with the rectangle's top left one. nullptr when there's nothing to show (the
    /// next picture then comes in full). Called on the main thread; the pixels are only valid during the call.
    /// </summary>
    using ImageHandler = void (*)(const uint32_t* pARGB, int nStride, int nCanvasWidth, int nCanvasHeight, int nX,
                                  int nY, int nWidth, int nHeight);

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
    static uint64_t Hash(const ra::ui::drawing::RecordingSurface::Op& pOp);

    wxWindow* m_pHost;
    bool m_bVisible = false;
    SurfaceRenderer m_pRenderer;
    // the canvas, as the host has it
    std::vector<uint32_t> m_vPixels;
    int m_nCanvasWidth = 0;
    int m_nCanvasHeight = 0;
    bool m_bHasImage = false;

    // what's on it (in points of the game area)
    struct DrawnOp
    {
        uint64_t nHash;
        float fLeft, fTop, fRight, fBottom;
    };
    std::vector<DrawnOp> m_vDrawn;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_OVERLAY_RENDERER_HH
