#ifndef RA_UI_IMGUI_IMAGE_REPOSITORY_HH
#define RA_UI_IMGUI_IMAGE_REPOSITORY_HH
#pragma once

#include "ui/headless/HeadlessImageRepository.hh"

#include <map>
#include <mutex>
#include <set>

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// IImageRepository that downloads images like the Windows ImageRepository and turns them into textures
/// through the host's texture handlers.
/// </summary>
class ImGuiImageRepository : public ra::ui::headless::HeadlessImageRepository
{
public:
    ~ImGuiImageRepository() noexcept override;

    /// <summary>
    /// Gets the registered repository, if it's an ImGuiImageRepository.
    /// </summary>
    static ImGuiImageRepository* Get();

    bool IsImageAvailable(ImageType nType, const std::string& sName) const override;
    void FetchImage(ImageType nType, const std::string& sName, const std::string& sSourceUrl, time_t tLastUpdated = 0) override;
    void ReleaseReference(ImageReference& pImage) noexcept override { pImage.SetData(0); }
    bool HasReferencedImageChanged(ImageReference& pImage) const override;

    /// <summary>
    /// Gets the texture for an image, loading it if it's on disk and requesting it if it isn't.
    /// Returns 0 if the image is not available (yet). UI thread only.
    /// </summary>
    uintptr_t GetTexture(ImageType nType, const std::string& sName, int* pWidth, int* pHeight);

    /// <summary>
    /// Releases a texture created through the host's handlers, on the UI thread.
    /// </summary>
    static void DestroyTexture(uintptr_t nTexture);

private:
    void OnImageDownloaded(ImageType nType, const std::string& sName);

    struct Texture
    {
        uintptr_t nTexture = 0;
        int nWidth = 0;
        int nHeight = 0;
    };

    std::map<std::string, Texture> m_mTextures; // UI thread only

    mutable std::mutex m_pMutex;
    std::set<std::wstring> m_vRequestedImages;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_IMAGE_REPOSITORY_HH
