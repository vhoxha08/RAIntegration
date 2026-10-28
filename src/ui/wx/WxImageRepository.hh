#ifndef RA_UI_WX_IMAGE_REPOSITORY_HH
#define RA_UI_WX_IMAGE_REPOSITORY_HH
#pragma once

#include "ui/headless/HeadlessImageRepository.hh"

#include <wx/graphics.h>

#include <map>
#include <mutex>
#include <set>

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// IImageRepository that downloads images like the Windows ImageRepository and loads them as bitmaps for drawing
/// surfaces (see SurfaceRenderer).
/// </summary>
class WxImageRepository : public ra::ui::headless::HeadlessImageRepository
{
public:
    /// <summary>
    /// Gets the registered repository, if it's a WxImageRepository.
    /// </summary>
    static WxImageRepository* Get();

    bool IsImageAvailable(ImageType nType, const std::string& sName) const override;
    void FetchImage(ImageType nType, const std::string& sName, const std::string& sSourceUrl, time_t tLastUpdated = 0) override;
    void ReleaseReference(ImageReference& pImage) noexcept override { pImage.SetData(0); }
    bool HasReferencedImageChanged(ImageReference& pImage) const override;

    struct Image
    {
        wxGraphicsBitmap pBitmap;
        int nWidth = 0;
        int nHeight = 0;
    };

    /// <summary>
    /// Gets an image, loading it if it's on disk and requesting it if it isn't. Returns nullptr if the image is not
    /// available (yet). UI thread only.
    /// </summary>
    const Image* GetImage(ImageType nType, const std::string& sName);

    /// <summary>
    /// Releases the loaded images (before wxWidgets shuts down). UI thread only.
    /// </summary>
    void ReleaseImages() noexcept;

private:
    void OnImageDownloaded(ImageType nType, const std::string& sName);

    std::map<std::string, Image> m_mImages; // UI thread only; images that failed to load have no bitmap

    mutable std::mutex m_pMutex;
    std::set<std::wstring> m_vRequestedImages;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_IMAGE_REPOSITORY_HH
