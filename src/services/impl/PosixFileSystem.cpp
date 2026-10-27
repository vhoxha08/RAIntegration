#include "PosixFileSystem.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include "services/impl/FileTextReader.hh"
#include "services/impl/FileTextWriter.hh"
#include "services/ServiceLocator.hh"

#include <cstdlib>
#include <filesystem>
#include <system_error>

#include <pwd.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace ra {
namespace services {
namespace impl {

static void NormalizeSeparators(std::wstring& sPath)
{
    for (auto& c : sPath)
    {
        if (c == L'\\')
            c = L'/';
    }
}

static fs::path ToPath(const std::wstring& sPath)
{
    // std::filesystem::path converts wide strings to the native (UTF-8) encoding
    return fs::path(sPath);
}

static std::wstring GetHomeDirectory()
{
    const char* sHome = std::getenv("HOME");
    if (sHome == nullptr || *sHome == '\0')
    {
        const struct passwd* pw = getpwuid(getuid());
        if (pw != nullptr)
            sHome = pw->pw_dir;
    }

    return (sHome != nullptr) ? ra::util::String::Widen(sHome) : std::wstring(L"/tmp");
}

std::wstring PosixFileSystem::GetDefaultBaseDirectory()
{
#ifdef __APPLE__
    return GetHomeDirectory() + L"/Library/Application Support/RAIntegration/";
#else
    const char* sDataHome = std::getenv("XDG_DATA_HOME");
    if (sDataHome != nullptr && *sDataHome == '/')
        return ra::util::String::Widen(sDataHome) + L"/RAIntegration/";

    return GetHomeDirectory() + L"/.local/share/RAIntegration/";
#endif
}

PosixFileSystem::PosixFileSystem(const std::wstring& sBaseDirectory)
    : m_sBaseDirectory(sBaseDirectory.empty() ? GetDefaultBaseDirectory() : sBaseDirectory)
{
    NormalizeSeparators(m_sBaseDirectory);
    if (m_sBaseDirectory.back() != L'/')
        m_sBaseDirectory.push_back(L'/');

    // the Windows build lives next to the emulator executable, which always exists. make sure ours does too.
    std::error_code ec;
    fs::create_directories(ToPath(m_sBaseDirectory), ec);
}

std::wstring PosixFileSystem::MakeAbsolute(const std::wstring& sPath) const
{
    std::wstring sAbsolutePath;
    if (sPath.empty() || (sPath.front() != L'/' && sPath.front() != L'\\'))
    {
        sAbsolutePath.reserve(m_sBaseDirectory.length() + sPath.length());
        sAbsolutePath.assign(m_sBaseDirectory);
    }

    sAbsolutePath.append(sPath);
    NormalizeSeparators(sAbsolutePath);
    return sAbsolutePath;
}

std::wstring PosixFileSystem::GetDirectory(const std::wstring& sPath) const
{
    const auto nIndex = sPath.find_last_of(L"/\\");
    if (nIndex != std::string::npos)
        return std::wstring(sPath, 0, nIndex);

    return sPath;
}

std::wstring PosixFileSystem::GetFileName(const std::wstring& sPath) const
{
    const auto nIndex = sPath.find_last_of(L"/\\");
    if (nIndex != std::string::npos)
        return std::wstring(sPath, nIndex + 1);

    return sPath;
}

std::wstring PosixFileSystem::GetExtension(const std::wstring& sPath) const
{
    const auto nIndex = sPath.find_last_of(L"/\\.");
    if (nIndex != std::string::npos && sPath.at(nIndex) == '.')
        return std::wstring(sPath, nIndex + 1);

    return L"";
}

std::wstring PosixFileSystem::RemoveExtension(const std::wstring& sPath) const
{
    const auto nIndex = sPath.find_last_of(L"/\\.");
    if (nIndex != std::string::npos && sPath.at(nIndex) == '.')
        return std::wstring(sPath, 0, nIndex);

    return sPath;
}

bool PosixFileSystem::DirectoryExists(const std::wstring& sDirectory) const
{
    std::error_code ec;
    return fs::is_directory(ToPath(MakeAbsolute(sDirectory)), ec);
}

bool PosixFileSystem::CreateDirectory(const std::wstring& sDirectory) const
{
    // like CreateDirectoryW: creates a single level and fails if it already exists
    std::error_code ec;
    return fs::create_directory(ToPath(MakeAbsolute(sDirectory)), ec);
}

size_t PosixFileSystem::GetFilesInDirectory(const std::wstring& sDirectory,
                                            _Inout_ std::vector<std::wstring>& vResults) const
{
    std::error_code ec;
    fs::directory_iterator pIter(ToPath(MakeAbsolute(sDirectory)), ec);
    if (ec)
        return 0U;

    const size_t nInitialSize = vResults.size();
    for (const auto& pEntry : pIter)
    {
        std::error_code ecEntry;
        if (!pEntry.is_directory(ecEntry))
            vResults.emplace_back(pEntry.path().filename().wstring());
    }

    return vResults.size() - nInitialSize;
}

bool PosixFileSystem::DeleteFile(const std::wstring& sPath) const
{
    const auto pPath = ToPath(MakeAbsolute(sPath));

    std::error_code ec;
    if (fs::is_directory(pPath, ec))
        return false;

    return fs::remove(pPath, ec);
}

bool PosixFileSystem::MoveFile(const std::wstring& sOldPath, const std::wstring& sNewPath) const
{
    // like MoveFileW: fails if the destination already exists
    const auto pNewPath = ToPath(MakeAbsolute(sNewPath));

    std::error_code ec;
    if (fs::exists(pNewPath, ec))
        return false;

    fs::rename(ToPath(MakeAbsolute(sOldPath)), pNewPath, ec);
    return !ec;
}

bool PosixFileSystem::CopyFile(const std::wstring& sSourcePath, const std::wstring& sNewPath) const
{
    // like CopyFileW(..., FALSE): overwrites the destination
    std::error_code ec;
    return fs::copy_file(ToPath(MakeAbsolute(sSourcePath)), ToPath(MakeAbsolute(sNewPath)),
                         fs::copy_options::overwrite_existing, ec);
}

int64_t PosixFileSystem::GetFileSize(const std::wstring& sPath) const
{
    const auto pPath = ToPath(MakeAbsolute(sPath));

    std::error_code ec;
    const auto nStatus = fs::status(pPath, ec);
    if (ec || !fs::exists(nStatus))
    {
        if (ec && ec != std::errc::no_such_file_or_directory && ra::services::ServiceLocator::Exists<ra::services::ILogger>())
        {
            RA_LOG_ERR("Error %d getting file size: %s", ec.value(), ra::util::String::Narrow(sPath).c_str());
        }

        return -1;
    }

    if (fs::is_directory(nStatus))
    {
        if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
        {
            RA_LOG_ERR("File size requested for directory: %s", ra::util::String::Narrow(sPath).c_str());
        }

        return -1;
    }

    const auto nSize = fs::file_size(pPath, ec);
    return ec ? -1 : gsl::narrow_cast<int64_t>(nSize);
}

std::chrono::system_clock::time_point PosixFileSystem::GetLastModified(const std::wstring& sPath) const
{
    std::error_code ec;
    const auto tFileTime = fs::last_write_time(ToPath(MakeAbsolute(sPath)), ec);
    if (ec)
    {
        RA_LOG_ERR("Error %d getting last modified for file: %s", ec.value(), ra::util::String::Narrow(sPath).c_str());
        return std::chrono::system_clock::time_point();
    }

    // file_clock and system_clock have different epochs; convert relative to "now" (exact within a few ns)
    const auto tNow = fs::file_time_type::clock::now();
    const auto tSystemNow = std::chrono::system_clock::now();
    return tSystemNow + std::chrono::duration_cast<std::chrono::system_clock::duration>(tFileTime - tNow);
}

std::unique_ptr<TextReader> PosixFileSystem::OpenTextFile(const std::wstring& sPath) const
{
    auto pReader = std::make_unique<FileTextReader>(MakeAbsolute(sPath));
    if (!pReader->GetFStream().is_open())
    {
        RA_LOG_INFO("Failed to open \"%s\": %d", ra::util::String::Narrow(sPath).c_str(), errno);
        return std::unique_ptr<TextReader>();
    }

    return std::unique_ptr<TextReader>(pReader.release());
}

std::unique_ptr<TextWriter> PosixFileSystem::CreateTextFile(const std::wstring& sPath) const
{
    auto pWriter = std::make_unique<FileTextWriter>(MakeAbsolute(sPath));
    if (!pWriter->GetFStream().is_open())
    {
        RA_LOG_WARN("Failed to create \"%s\": %d", ra::util::String::Narrow(sPath).c_str(), errno);
        return std::unique_ptr<TextWriter>();
    }

    return std::unique_ptr<TextWriter>(pWriter.release());
}

std::unique_ptr<TextWriter> PosixFileSystem::AppendTextFile(const std::wstring& sPath) const
{
    const auto sAbsolutePath = MakeAbsolute(sPath);

    // cannot use std::ios::app, or the SetPosition method doesn't work
    // have to specify std::ios::in or the previous contents are lost
    auto pWriter = std::make_unique<FileTextWriter>(sAbsolutePath, std::ios::ate | std::ios::in | std::ios::out);
    if (!pWriter->GetFStream().is_open())
    {
        // failed to open the file - try creating it
        std::ofstream file(ToPath(sAbsolutePath), std::ios::out);
        if (file.is_open())
        {
            // create succeeded, reopen it
            file.close();

            pWriter = std::make_unique<FileTextWriter>(sAbsolutePath, std::ios::in | std::ios::out);
        }
        else
        {
            // create failed
            if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
            {
                RA_LOG_WARN("Failed to open \"%s\" for append: %d", ra::util::String::Narrow(sPath).c_str(), errno);
            }

            return std::unique_ptr<TextWriter>();
        }
    }

    return std::unique_ptr<TextWriter>(pWriter.release());
}

} // namespace impl
} // namespace services
} // namespace ra
