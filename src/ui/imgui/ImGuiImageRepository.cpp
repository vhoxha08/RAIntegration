#include "ImGuiImageRepository.hh"

#include "context/IRcClient.hh"

#include "services/Http.hh"
#include "services/IConfiguration.hh"
#include "services/IFileSystem.hh"
#include "services/ServiceLocator.hh"

#include "ui/headless/HeadlessDesktop.hh"
#include "ui/imgui/ImGuiTextures.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <rcheevos/include/rc_api_runtime.h>

#include <algorithm>

namespace ra {
namespace ui {
namespace imgui {

static std::string GetKey(ImageType nType, const std::string& sName)
{
    return std::to_string(static_cast<int>(nType)) + ':' + sName;
}

static const ra::ui::headless::HeadlessDesktop* GetDesktop()
{
    if (!ra::services::ServiceLocator::Exists<ra::ui::IDesktop>())
        return nullptr;

    return dynamic_cast<const ra::ui::headless::HeadlessDesktop*>(&ra::services::ServiceLocator::Get<ra::ui::IDesktop>());
}

ImGuiImageRepository* ImGuiImageRepository::Get()
{
    if (!ra::services::ServiceLocator::Exists<IImageRepository>())
        return nullptr;

    return dynamic_cast<ImGuiImageRepository*>(&ra::services::ServiceLocator::GetMutable<IImageRepository>());
}

ImGuiImageRepository::~ImGuiImageRepository() noexcept
{
    // the textures belong to the host's rendering context; only release them if we're on its thread
    const auto* pDesktop = GetDesktop();
    if (pDesktop != nullptr && pDesktop->IsOnUIThread() && GetTextureHandlers().fpDestroy != nullptr)
    {
        for (const auto& pPair : m_mTextures)
        {
            if (pPair.second.nTexture != 0)
                GetTextureHandlers().fpDestroy(pPair.second.nTexture);
        }
    }
}

void ImGuiImageRepository::DestroyTexture(uintptr_t nTexture)
{
    const auto fpDestroy = GetTextureHandlers().fpDestroy;
    if (nTexture == 0 || fpDestroy == nullptr)
        return;

    const auto* pDesktop = GetDesktop();
    if (pDesktop == nullptr)
        return;

    if (pDesktop->IsOnUIThread())
        fpDestroy(nTexture);
    else
        pDesktop->PostToUIThread([fpDestroy, nTexture]() { fpDestroy(nTexture); });
}

bool ImGuiImageRepository::IsImageAvailable(ImageType nType, const std::string& sName) const
{
    if (sName.empty())
        return false;

    const std::wstring sFilename = GetFilename(nType, sName);
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        if (m_vRequestedImages.find(sFilename) != m_vRequestedImages.end())
            return false; // still downloading
    }

    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    return (pFileSystem.GetFileSize(sFilename) > 0);
}

// same as ra::ui::drawing::gdi::ImageRepository::FetchImage
void ImGuiImageRepository::FetchImage(ImageType nType, const std::string& sName, const std::string& sSourceUrl, time_t tLastUpdated)
{
    if (sName.empty() || nType == ImageType::Local)
        return;

    std::wstring sFilename = GetFilename(nType, sName);
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    if (pFileSystem.GetFileSize(sFilename) > 0)
    {
        if (tLastUpdated == 0 || tLastUpdated < std::chrono::system_clock::to_time_t(pFileSystem.GetLastModified(sFilename)))
            return;
    }

    // check to see if it's already queued
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        if (m_vRequestedImages.find(sFilename) != m_vRequestedImages.end())
            return;

        m_vRequestedImages.emplace(sFilename);
    }

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::Offline))
        return;

    std::string sUrl = sSourceUrl;
    if (sSourceUrl.empty())
    {
        const auto& pRcClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>();
        rc_api_fetch_image_request_t api_params;
        memset(&api_params, 0, sizeof(api_params));
        api_params.image_name = sName.c_str();
        switch (nType)
        {
            case ImageType::Badge:
                api_params.image_type = RC_IMAGE_TYPE_ACHIEVEMENT;
                break;

            case ImageType::UserPic:
                api_params.image_type = RC_IMAGE_TYPE_USER;
                break;

            case ImageType::Icon:
                api_params.image_type = RC_IMAGE_TYPE_GAME;
                break;

            default:
                break;
        }

        rc_api_request_t api_request;
        if (rc_api_init_fetch_image_request_hosted(&api_request, &api_params, pRcClient.GetHost()) == RC_OK)
            sUrl = api_request.url;

        rc_api_destroy_request(&api_request);
        if (sUrl.empty())
            return;
    }

    RA_LOG_INFO("Downloading %s", sUrl.c_str());

    ra::services::Http::Request request(sUrl);
    request.DownloadAsync(sFilename, [this, sFilename, sUrl, nType, sName](const ra::services::Http::Response& response) {
        if (response.StatusCode() == ra::services::Http::StatusCode::OK)
        {
            auto nFileSize = ra::services::ServiceLocator::Get<ra::services::IFileSystem>().GetFileSize(sFilename);
            RA_LOG_INFO("Wrote %lu bytes to %s", nFileSize, ra::util::String::Narrow(sFilename).c_str());

            // only remove the image from the request queue if successful. prevents repeated requests
            std::lock_guard<std::mutex> lock(m_pMutex);
            m_vRequestedImages.erase(sFilename);
        }
        else
        {
            RA_LOG_WARN("Error %u fetching %s", response.StatusCode(), sUrl.c_str());
            ra::services::ServiceLocator::Get<ra::services::IFileSystem>().DeleteFile(sFilename);
        }

        OnImageDownloaded(nType, sName);
    });
}

void ImGuiImageRepository::OnImageDownloaded(ImageType nType, const std::string& sName)
{
    // drop a texture made from an older copy of the file (on the UI thread, which owns the textures)
    const auto* pDesktop = GetDesktop();
    if (pDesktop != nullptr)
    {
        const std::string sKey = GetKey(nType, sName);
        pDesktop->PostToUIThread([this, sKey]() {
            const auto pIter = m_mTextures.find(sKey);
            if (pIter != m_mTextures.end())
            {
                DestroyTexture(pIter->second.nTexture);
                m_mTextures.erase(pIter);
            }
        });
    }

    OnImageChanged(nType, sName);
}

bool ImGuiImageRepository::HasReferencedImageChanged(ImageReference& pImage) const
{
    // the reference's data is 1 once the image has been available; report the change once
    if (pImage.Type() == ImageType::None || pImage.GetData() != 0)
        return false;

    if (!IsImageAvailable(pImage.Type(), pImage.Name()))
        return false;

    pImage.SetData(1);
    return true;
}

uintptr_t ImGuiImageRepository::GetTexture(ImageType nType, const std::string& sName, int* pWidth, int* pHeight)
{
    if (sName.empty())
        return 0;

    const std::string sKey = GetKey(nType, sName);
    const auto pIter = m_mTextures.find(sKey);
    if (pIter != m_mTextures.end())
    {
        *pWidth = pIter->second.nWidth;
        *pHeight = pIter->second.nHeight;
        return pIter->second.nTexture;
    }

    if (!IsImageAvailable(nType, sName))
    {
        FetchImage(nType, sName, "");
        return 0;
    }

    const auto fpCreate = GetTextureHandlers().fpCreateFromFile;
    if (fpCreate == nullptr)
        return 0;

    // remember failures too, so a broken file isn't decoded every frame
    Texture pTexture;
    std::string sPath = ra::util::String::Narrow(GetFilename(nType, sName));
    std::replace(sPath.begin(), sPath.end(), '\\', '/'); // paths are built with '\\' separators
    pTexture.nTexture = fpCreate(sPath.c_str(), &pTexture.nWidth, &pTexture.nHeight);
    if (pTexture.nTexture == 0)
        RA_LOG_WARN("Could not load image %s", sPath.c_str());

    m_mTextures.emplace(sKey, pTexture);
    *pWidth = pTexture.nWidth;
    *pHeight = pTexture.nHeight;
    return pTexture.nTexture;
}

} // namespace imgui
} // namespace ui
} // namespace ra
