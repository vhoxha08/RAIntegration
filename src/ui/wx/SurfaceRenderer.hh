#ifndef RA_UI_WX_SURFACE_RENDERER_HH
#define RA_UI_WX_SURFACE_RENDERER_HH
#pragma once

#include "ui/drawing/RecordingSurface.hh"

#include <wx/graphics.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// Draws what was recorded on a RecordingSurface (the memory viewer, the overlay) with a wxGraphicsContext.
/// UI thread only.
/// </summary>
class SurfaceRenderer
{
public:
    /// <summary>
    /// Draws the recorded commands that come after <paramref name="nAfterSequence" /> (all of them by default).
    /// </summary>
    /// <param name="fScale">The display's pixels per point (text is drawn at the display's resolution).</param>
    void Draw(wxGraphicsContext& pContext, const std::vector<ra::ui::drawing::RecordingSurface::Op>& vOps,
              double fScale, uint64_t nAfterSequence = 0);

private:
    struct TextBitmap
    {
        wxGraphicsBitmap pBitmap;
        double fWidth = 0.0;
        double fHeight = 0.0;
    };
    const wxGraphicsBitmap& GetSnapshotPart(const ra::ui::drawing::RecordingSurface::Snapshot& pSnapshot,
                                            double fScale, double fX, double fY, double fWidth, double fHeight);

    const TextBitmap& GetTextBitmap(const std::string& sText, const std::string& sFontFace, float fFontSize,
                                    bool bBold, uint32_t nColor, double fScale);

    // laying text out is expensive and the same strings are drawn over and over
    std::unordered_map<std::string, TextBitmap> m_mTextBitmaps;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_SURFACE_RENDERER_HH
