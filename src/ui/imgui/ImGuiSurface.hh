#ifndef RA_UI_IMGUI_SURFACE_HH
#define RA_UI_IMGUI_SURFACE_HH
#pragma once

#include "ui/drawing/ISurface.hh"

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct ImDrawList;

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ISurface that records what is drawn on it and replays it into a Dear ImGui draw list.
/// </summary>
/// <remarks>
/// The toolkit renders popups into their own surfaces and then copies those onto the overlay surface
/// (DrawSurface), possibly faded (SetOpacity). Copying a recording surface copies its commands, translated and
/// clipped, so the final surface holds everything needed to draw a frame.
/// </remarks>
class ImGuiSurface : public ra::ui::drawing::ISurface
{
public:
    ImGuiSurface(int nWidth, int nHeight, bool bTransparent) noexcept;

    unsigned int GetWidth() const noexcept override { return m_nWidth; }
    unsigned int GetHeight() const noexcept override { return m_nHeight; }

    void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) override;
    int LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle) override;
    ra::ui::Size MeasureText(int nFont, const std::wstring& sText) const override;
    void WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText) override;
    void DrawImage(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage) override;
    void DrawImageStretched(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage) override;
    void DrawSurface(int nX, int nY, const ISurface& pSurface) override;
    void DrawSurface(int nX, int nY, const ISurface& pSurface, int nSurfaceX, int nSurfaceY, int nWidth, int nHeight) override;
    void SetOpacity(double fAlpha) override;
    void SetPixels(int nX, int nY, int nWidth, int nHeight, uint32_t* pARGB) override;

    /// <summary>
    /// Draws the recorded commands with the surface's upper left corner at (fX, fY). UI thread only.
    /// </summary>
    void Replay(ImDrawList& pDrawList, float fX, float fY) const;

    struct PixelImage;

    enum class OpType
    {
        Rectangle,
        Text,
        Image,
        Pixels,
    };

    struct Op
    {
        OpType nType;
        float fX, fY, fWidth, fHeight;
        float fClipLeft, fClipTop, fClipRight, fClipBottom;
        uint32_t nColor = 0; // ARGB
        float fAlpha = 1.0f;

        float fFontSize = 0.0f;
        bool bBold = false;
        std::string sFontFace;
        std::string sText; // UTF-8

        ImageType nImageType = ImageType::None;
        std::string sImageName;
        bool bStretch = false;

        std::shared_ptr<PixelImage> pPixels;

        uint64_t nSequence = 0; // increases with every recorded command (see GetOps)
    };

    /// <summary>
    /// The recorded drawing commands, in order (for drawing the surface with something other than ImGui). Commands
    /// hidden by later opaque fills are dropped; a renderer that keeps its own image can draw just the commands
    /// with a higher nSequence than the last one it drew.
    /// </summary>
    const std::vector<Op>& GetOps() const noexcept { return m_vOps; }

    /// <summary>
    /// Drops the recorded commands up to and including <paramref name="nSequence" />. For renderers that keep
    /// their own image and only draw what was added since (see GetOps), so the recording doesn't have to.
    /// </summary>
    void DiscardOps(uint64_t nSequence) const;

    /// <summary>
    /// Measures text for surfaces that are not drawn by ImGui (i.e. by native windows). Used on the UI thread
    /// outside of ImGui frames; elsewhere the size is estimated.
    /// </summary>
    using TextMeasurer = std::function<ra::ui::Size(const std::string& sFace, float fSize, bool bBold,
                                                    const std::wstring& sText)>;
    static void SetTextMeasurer(TextMeasurer fMeasurer);

    // Like GDI fonts, fonts are shared by all surfaces: callers load a font on one surface and use it on others
    struct Font
    {
        std::string sFace;
        float fSize = 13.0f;
        bool bBold = false;
    };

private:
    Op& AddOp(OpType nType, int nX, int nY, int nWidth, int nHeight);
    void AddOp(Op&& pOp);
    void RemoveCoveredOps(const Op& pOpaqueOp);
    float GetAlpha(Color nColor) const noexcept;

    int m_nWidth;
    int m_nHeight;
    bool m_bTransparent;
    mutable std::vector<Op> m_vOps; // DiscardOps is const: it doesn't change what the surface shows
    uint64_t m_nNextSequence = 1;

    static Font GetFont(int nFont);
};

class ImGuiSurfaceFactory : public ra::ui::drawing::ISurfaceFactory
{
public:
    std::unique_ptr<ra::ui::drawing::ISurface> CreateSurface(int nWidth, int nHeight) const override
    {
        return std::make_unique<ImGuiSurface>(nWidth, nHeight, false);
    }

    std::unique_ptr<ra::ui::drawing::ISurface> CreateTransparentSurface(int nWidth, int nHeight) const override
    {
        return std::make_unique<ImGuiSurface>(nWidth, nHeight, true);
    }

    bool SaveImage(const ra::ui::drawing::ISurface& pSurface, const std::wstring& sPath) const override;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_SURFACE_HH
