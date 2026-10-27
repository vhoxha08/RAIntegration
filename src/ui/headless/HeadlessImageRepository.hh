#ifndef RA_UI_HEADLESS_IMAGE_REPOSITORY_HH
#define RA_UI_HEADLESS_IMAGE_REPOSITORY_HH
#pragma once

#include "ui/IImageRepository.hh"

namespace ra {
namespace ui {
namespace headless {

/// <summary>
/// IImageRepository that knows where images live on disk (same layout as the Windows ImageRepository) but
/// does not download or decode them.
/// </summary>
class HeadlessImageRepository : public IImageRepository
{
public:
    bool IsImageAvailable(ImageType nType, const std::string& sName) const override;
    void FetchImage(ImageType nType, const std::string& sName, const std::string& sSourceUrl, time_t tLastUpdated = 0) override;
    std::string StoreImage(ImageType nType, const std::wstring& sPath) override;
    std::wstring GetFilename(ImageType nType, const std::string& sName) const override;

    void AddReference(const ImageReference&) noexcept override {}
    void ReleaseReference(ImageReference&) noexcept(false) override {}
    bool HasReferencedImageChanged(ImageReference&) const override { return false; }
};

} // namespace headless
} // namespace ui
} // namespace ra

#endif // !RA_UI_HEADLESS_IMAGE_REPOSITORY_HH
