#ifndef RA_UI_IMGUI_SURFACE_HH
#define RA_UI_IMGUI_SURFACE_HH
#pragma once

#include "ui/drawing/ISurface.hh"

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

private:
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
        std::string sText; // UTF-8

        ImageType nImageType = ImageType::None;
        std::string sImageName;
        bool bStretch = false;

        std::shared_ptr<PixelImage> pPixels;
    };

    Op& AddOp(OpType nType, int nX, int nY, int nWidth, int nHeight);
    float GetAlpha(Color nColor) const noexcept;

    int m_nWidth;
    int m_nHeight;
    bool m_bTransparent;
    std::vector<Op> m_vOps;

    struct Font
    {
        float fSize;
        bool bBold;
    };
    std::vector<Font> m_vFonts;
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
