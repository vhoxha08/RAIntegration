#include "HeadlessImageRepository.hh"

#include "RA_Defs.h"
#include "RA_md5factory.h"

#include "data/context/GameContext.hh"

#include "services/IFileSystem.hh"
#include "services/ServiceLocator.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

namespace ra {
namespace ui {
namespace headless {

// keep in sync with ra::ui::drawing::gdi::ImageRepository
static void RemoveRedundantFileExtension(std::wstring& sFilename, const std::string& sName)
{
    const auto nIndex = sName.rfind('.');
    if (nIndex == std::string::npos)
        return;

    auto sExtension = sName.substr(nIndex);
    ra::util::String::MakeLowercase(sExtension);

    if (sExtension == ".jpg" || sExtension == ".gif" || sExtension == ".jpeg" || sExtension == ".png")
    {
        // name contains a supported extension, remove ".png" appended by GetFilename
        sFilename.resize(sFilename.length() - 4);
    }
}

std::wstring HeadlessImageRepository::GetFilename(ImageType nType, const std::string& sName) const
{
    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    std::wstring sFilename = pFileSystem.BaseDirectory();

    switch (nType)
    {
        case ImageType::Badge:
            sFilename += RA_DIR_BADGE + ra::util::String::Widen(sName) + L".png";
            RemoveRedundantFileExtension(sFilename, sName);
            break;
        case ImageType::Icon:
            sFilename += RA_DIR_BADGE + std::wstring(L"i") + ra::util::String::Widen(sName) + L".png";
            break;
        case ImageType::UserPic:
            sFilename += RA_DIR_USERPIC + ra::util::String::Widen(sName) + L".png";
            break;
        case ImageType::Local:
            sFilename += ra::util::String::Widen(sName);
            break;
        default:
            Expects(!"Unsupported image type");
            break;
    }

    return sFilename;
}

bool HeadlessImageRepository::IsImageAvailable(ImageType nType, const std::string& sName) const
{
    if (sName.empty())
        return false;

    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    return (pFileSystem.GetFileSize(GetFilename(nType, sName)) > 0);
}

void HeadlessImageRepository::FetchImage(ImageType nType, const std::string& sName,
                                         [[maybe_unused]] const std::string& sSourceUrl,
                                         [[maybe_unused]] time_t tLastUpdated)
{
    RA_LOG_INFO("[headless] not fetching image %d:%s", ra::etoi(nType), sName.c_str());
}

std::string HeadlessImageRepository::StoreImage(ImageType nType, const std::wstring& sPath)
{
    // same as ra::ui::drawing::gdi::ImageRepository::StoreImage
    auto sFileMD5 = RAGenerateFileMD5(sPath);
    if (sFileMD5.empty())
        return "";

    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    const auto sDirectory = pFileSystem.BaseDirectory() + RA_DIR_BADGE + L"local\\";
    if (!pFileSystem.DirectoryExists(sDirectory))
        pFileSystem.CreateDirectory(sDirectory);

    auto sExtension = pFileSystem.GetExtension(sPath);
    ra::util::String::MakeLowercase(sExtension);

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    sFileMD5 = ra::util::String::Printf("local\\%u-%s.%s", pGameContext.GameId(), sFileMD5, sExtension);

    const std::wstring sFilename = GetFilename(nType, sFileMD5);
    if (pFileSystem.GetFileSize(sFilename) < 0)
    {
        if (!pFileSystem.CopyFile(sPath, sFilename))
            return "";
    }

    return sFileMD5;
}

} // namespace headless
} // namespace ui
} // namespace ra
