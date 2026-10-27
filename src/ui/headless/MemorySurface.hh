#ifndef RA_UI_HEADLESS_MEMORY_SURFACE_HH
#define RA_UI_HEADLESS_MEMORY_SURFACE_HH
#pragma once

#include "ui/drawing/ISurface.hh"

#include "util/Log.hh"

#include <algorithm>
#include <vector>

namespace ra {
namespace ui {
namespace headless {

/// <summary>
/// ISurface backed by a 32-bit ARGB pixel buffer. Rectangles, pixel blocks and other memory surfaces are
/// drawn; text and images are not rendered (text is measured with an approximate fixed-width metric).
/// A rendering backend can upload <see cref="GetPixels" /> as a texture.
/// </summary>
class MemorySurface : public ra::ui::drawing::ISurface
{
public:
    MemorySurface(int nWidth, int nHeight)
        : m_nWidth(std::max(nWidth, 0)), m_nHeight(std::max(nHeight, 0)),
          m_vPixels(gsl::narrow_cast<size_t>(m_nWidth) * gsl::narrow_cast<size_t>(m_nHeight), 0U)
    {
    }

    unsigned int GetWidth() const noexcept override { return gsl::narrow_cast<unsigned int>(m_nWidth); }
    unsigned int GetHeight() const noexcept override { return gsl::narrow_cast<unsigned int>(m_nHeight); }

    const std::vector<uint32_t>& GetPixels() const noexcept { return m_vPixels; }

    void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) override
    {
        // Color::Transparent is a color key, not a real color
        const uint32_t nARGB = (nColor.ARGB == Color::Transparent.ARGB) ? 0U : nColor.ARGB;
        ForEachPixel(nX, nY, nWidth, nHeight, [nARGB](uint32_t& nPixel, int, int) { nPixel = nARGB; });
    }

    int LoadFont(const std::string&, int nFontSize, FontStyles) override
    {
        m_vFontSizes.push_back(nFontSize);
        return gsl::narrow_cast<int>(m_vFontSizes.size());
    }

    ra::ui::Size MeasureText(int nFont, const std::wstring& sText) const override
    {
        const int nFontSize = (nFont > 0 && nFont <= gsl::narrow_cast<int>(m_vFontSizes.size()))
            ? m_vFontSizes.at(gsl::narrow_cast<size_t>(nFont) - 1) : 12;
        return {gsl::narrow_cast<int>(sText.length()) * nFontSize * 6 / 10, nFontSize};
    }

    void WriteText(int, int, int, Color, const std::wstring&) override {}

    void DrawImage(int, int, int, int, const ImageReference&) override {}

    void DrawImageStretched(int, int, int, int, const ImageReference&) override {}

    void DrawSurface(int nX, int nY, const ISurface& pSurface) override
    {
        DrawSurface(nX, nY, pSurface, 0, 0, gsl::narrow_cast<int>(pSurface.GetWidth()),
                    gsl::narrow_cast<int>(pSurface.GetHeight()));
    }

    void DrawSurface(int nX, int nY, const ISurface& pSurface, int nSurfaceX, int nSurfaceY, int nWidth,
                     int nHeight) override
    {
        const auto* pSource = dynamic_cast<const MemorySurface*>(&pSurface);
        if (pSource == nullptr)
            return;

        ForEachPixel(nX, nY, nWidth, nHeight, [&](uint32_t& nPixel, int nDX, int nDY) {
            const int nSX = nSurfaceX + nDX;
            const int nSY = nSurfaceY + nDY;
            if (nSX >= 0 && nSY >= 0 && nSX < pSource->m_nWidth && nSY < pSource->m_nHeight)
            {
                const auto nSourcePixel = pSource->m_vPixels.at(gsl::narrow_cast<size_t>(nSY * pSource->m_nWidth + nSX));
                if (nSourcePixel >> 24) // skip fully transparent pixels
                    nPixel = nSourcePixel;
            }
        });
    }

    void SetOpacity(double fAlpha) override
    {
        const auto nAlpha = gsl::narrow_cast<uint32_t>(std::clamp(fAlpha, 0.0, 1.0) * 255.0) << 24;
        for (auto& nPixel : m_vPixels)
        {
            if (nPixel >> 24)
                nPixel = (nPixel & 0x00FFFFFF) | nAlpha;
        }
    }

    void SetPixels(int nX, int nY, int nWidth, int nHeight, uint32_t* pARGB) override
    {
        if (pARGB == nullptr)
            return;

        ForEachPixel(nX, nY, nWidth, nHeight, [&](uint32_t& nPixel, int nDX, int nDY) {
            nPixel = pARGB[nDY * nWidth + nDX];
        });
    }

private:
    template<typename TFunc>
    void ForEachPixel(int nX, int nY, int nWidth, int nHeight, TFunc&& fFunc)
    {
        const int nStartY = std::max(nY, 0);
        const int nStopY = std::min(nY + nHeight, m_nHeight);
        const int nStartX = std::max(nX, 0);
        const int nStopX = std::min(nX + nWidth, m_nWidth);
        for (int y = nStartY; y < nStopY; ++y)
        {
            for (int x = nStartX; x < nStopX; ++x)
                fFunc(m_vPixels.at(gsl::narrow_cast<size_t>(y * m_nWidth + x)), x - nX, y - nY);
        }
    }

    int m_nWidth;
    int m_nHeight;
    std::vector<uint32_t> m_vPixels;
    std::vector<int> m_vFontSizes;
};

class MemorySurfaceFactory : public ra::ui::drawing::ISurfaceFactory
{
public:
    std::unique_ptr<ra::ui::drawing::ISurface> CreateSurface(int nWidth, int nHeight) const override
    {
        return std::make_unique<MemorySurface>(nWidth, nHeight);
    }

    std::unique_ptr<ra::ui::drawing::ISurface> CreateTransparentSurface(int nWidth, int nHeight) const override
    {
        return std::make_unique<MemorySurface>(nWidth, nHeight);
    }

    bool SaveImage(const ra::ui::drawing::ISurface&, const std::wstring& sPath) const override
    {
        RA_LOG_WARN("[headless] SaveImage not supported: %s", ra::util::String::Narrow(sPath).c_str());
        return false;
    }
};

} // namespace headless
} // namespace ui
} // namespace ra

#endif // !RA_UI_HEADLESS_MEMORY_SURFACE_HH
