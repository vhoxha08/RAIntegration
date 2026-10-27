#include "CppUnitTest.h"

#include "services/impl/HostAudioSystem.hh"
#include "services/impl/HostClipboard.hh"
#include "services/impl/PosixFileSystem.hh"

#include <filesystem>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace impl {
namespace tests {

// creates a unique scratch directory and removes it again
class TempDirectory
{
public:
    TempDirectory()
    {
        std::string sTemplate = (std::filesystem::temp_directory_path() / "ra_posix_tests_XXXXXX").string();
        const char* sPath = mkdtemp(sTemplate.data());
        Assert::IsNotNull(sPath);
        m_sPath = sPath;
    }

    ~TempDirectory() noexcept
    {
        std::error_code ec;
        std::filesystem::remove_all(m_sPath, ec);
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
    TempDirectory(TempDirectory&&) = delete;
    TempDirectory& operator=(TempDirectory&&) = delete;

    const std::string& Path() const noexcept { return m_sPath; }
    std::wstring WPath() const { return ra::util::String::Widen(m_sPath); }

    std::string ReadFile(const std::string& sRelativePath) const
    {
        std::ifstream pFile(m_sPath + "/" + sRelativePath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(pFile), std::istreambuf_iterator<char>());
    }

    void WriteFile(const std::string& sRelativePath, const std::string& sContents) const
    {
        std::ofstream pFile(m_sPath + "/" + sRelativePath, std::ios::binary);
        pFile << sContents;
    }

private:
    std::string m_sPath;
};

TEST_CLASS(PosixFileSystem_Tests)
{
public:
    TEST_METHOD(TestBaseDirectory)
    {
        TempDirectory pTemp;
        const auto sBase = pTemp.WPath() + L"/nested/base";
        PosixFileSystem pFileSystem(sBase);

        Assert::AreEqual(sBase + L"/", pFileSystem.BaseDirectory());
        Assert::IsTrue(std::filesystem::is_directory(pTemp.Path() + "/nested/base"));
    }

    TEST_METHOD(TestDefaultBaseDirectory)
    {
        const auto sDefault = PosixFileSystem::GetDefaultBaseDirectory();
        Assert::IsTrue(ra::util::String::EndsWith(sDefault, L"/RAIntegration/"));
        Assert::AreEqual(L'/', sDefault.front());
    }

    TEST_METHOD(TestMakeAbsolute)
    {
        TempDirectory pTemp;
        PosixFileSystem pFileSystem(pTemp.WPath());

        Assert::AreEqual(pTemp.WPath() + L"/RACache/Data/1.json", pFileSystem.MakeAbsolute(L"RACache\\Data\\1.json"));
        Assert::AreEqual(std::wstring(L"/tmp/abs/file.txt"), pFileSystem.MakeAbsolute(L"/tmp/abs/file.txt"));
        Assert::AreEqual(pTemp.WPath() + L"/RACache/Data/1.json",
                         pFileSystem.MakeAbsolute(pFileSystem.BaseDirectory() + L"RACache\\Data\\1.json"));
    }

    TEST_METHOD(TestDirectories)
    {
        TempDirectory pTemp;
        PosixFileSystem pFileSystem(pTemp.WPath());

        Assert::IsFalse(pFileSystem.DirectoryExists(L"RACache"));
        Assert::IsTrue(pFileSystem.CreateDirectory(L"RACache"));
        Assert::IsTrue(pFileSystem.DirectoryExists(L"RACache"));
        Assert::IsFalse(pFileSystem.CreateDirectory(L"RACache"), L"already exists");
        Assert::IsTrue(pFileSystem.CreateDirectory(pFileSystem.BaseDirectory() + L"RACache\\Data"));
        Assert::IsTrue(std::filesystem::is_directory(pTemp.Path() + "/RACache/Data"));
    }

    TEST_METHOD(TestTextFiles)
    {
        TempDirectory pTemp;
        PosixFileSystem pFileSystem(pTemp.WPath());
        pFileSystem.CreateDirectory(L"RACache");

        {
            auto pWriter = pFileSystem.CreateTextFile(L"RACache\\test.txt");
            Assert::IsTrue(pWriter != nullptr);
            pWriter->WriteLine("first");
        }
        {
            auto pWriter = pFileSystem.AppendTextFile(L"RACache\\test.txt");
            Assert::IsTrue(pWriter != nullptr);
            pWriter->WriteLine("second");
        }
        {
            // append creates the file if it doesn't exist (relative to the base directory)
            auto pWriter = pFileSystem.AppendTextFile(L"RACache\\new.txt");
            Assert::IsTrue(pWriter != nullptr);
            pWriter->Write("created");
        }

        // FileTextWriter always writes CRLF so files match the ones written on Windows
        Assert::AreEqual(std::string("first\r\nsecond\r\n"), pTemp.ReadFile("RACache/test.txt"));
        Assert::AreEqual(std::string("created"), pTemp.ReadFile("RACache/new.txt"));

        auto pReader = pFileSystem.OpenTextFile(L"RACache\\test.txt");
        Assert::IsTrue(pReader != nullptr);
        std::string sLine;
        Assert::IsTrue(pReader->GetLine(sLine));
        Assert::AreEqual(std::string("first"), sLine);

        Assert::IsTrue(pFileSystem.OpenTextFile(L"RACache\\missing.txt") == nullptr);
    }

    TEST_METHOD(TestFileSizeAndModified)
    {
        TempDirectory pTemp;
        PosixFileSystem pFileSystem(pTemp.WPath());
        pTemp.WriteFile("data.bin", std::string(1234, 'x'));

        Assert::AreEqual(int64_t{1234}, pFileSystem.GetFileSize(L"data.bin"));
        Assert::AreEqual(int64_t{-1}, pFileSystem.GetFileSize(L"missing.bin"));
        Assert::AreEqual(int64_t{-1}, pFileSystem.GetFileSize(L""), L"directory");

        const auto tModified = pFileSystem.GetLastModified(L"data.bin");
        const auto tDelta = std::chrono::system_clock::now() - tModified;
        Assert::IsTrue(tDelta < std::chrono::minutes(1) && tDelta > -std::chrono::minutes(1));
    }

    TEST_METHOD(TestMoveCopyDelete)
    {
        TempDirectory pTemp;
        PosixFileSystem pFileSystem(pTemp.WPath());
        pTemp.WriteFile("a.txt", "A");
        pTemp.WriteFile("b.txt", "B");

        Assert::IsFalse(pFileSystem.MoveFile(L"a.txt", L"b.txt"), L"move must not overwrite");
        Assert::IsTrue(pFileSystem.MoveFile(L"a.txt", L"c.txt"));
        Assert::AreEqual(std::string("A"), pTemp.ReadFile("c.txt"));
        Assert::AreEqual(int64_t{-1}, pFileSystem.GetFileSize(L"a.txt"));

        Assert::IsTrue(pFileSystem.CopyFile(L"c.txt", L"b.txt"), L"copy overwrites");
        Assert::AreEqual(std::string("A"), pTemp.ReadFile("b.txt"));

        Assert::IsTrue(pFileSystem.DeleteFile(L"b.txt"));
        Assert::IsFalse(pFileSystem.DeleteFile(L"b.txt"));

        pFileSystem.CreateDirectory(L"dir");
        Assert::IsFalse(pFileSystem.DeleteFile(L"dir"), L"not a file");
    }

    TEST_METHOD(TestGetFilesInDirectory)
    {
        TempDirectory pTemp;
        PosixFileSystem pFileSystem(pTemp.WPath());
        pFileSystem.CreateDirectory(L"Badge");
        pFileSystem.CreateDirectory(L"Badge\\local");
        pTemp.WriteFile("Badge/1.png", "x");
        pTemp.WriteFile("Badge/2.png", "x");

        std::vector<std::wstring> vFiles{L"existing"};
        Assert::AreEqual(size_t{2}, pFileSystem.GetFilesInDirectory(L"Badge", vFiles));
        Assert::AreEqual(size_t{3}, vFiles.size());
        std::sort(vFiles.begin() + 1, vFiles.end());
        Assert::AreEqual(std::wstring(L"1.png"), vFiles.at(1));
        Assert::AreEqual(std::wstring(L"2.png"), vFiles.at(2));

        Assert::AreEqual(size_t{0}, pFileSystem.GetFilesInDirectory(L"Missing", vFiles));
    }

    TEST_METHOD(TestPathHelpers)
    {
        PosixFileSystem pFileSystem(L"/tmp");
        Assert::AreEqual(std::wstring(L"RACache\\Data"), pFileSystem.GetDirectory(L"RACache\\Data\\1.json"));
        Assert::AreEqual(std::wstring(L"1.json"), pFileSystem.GetFileName(L"RACache/Data/1.json"));
        Assert::AreEqual(std::wstring(L"json"), pFileSystem.GetExtension(L"RACache\\Data\\1.json"));
        Assert::AreEqual(std::wstring(L""), pFileSystem.GetExtension(L"dir.d/file"));
        Assert::AreEqual(std::wstring(L"dir/file"), pFileSystem.RemoveExtension(L"dir/file.txt"));
    }
};

TEST_CLASS(HostServices_Tests)
{
public:
    TEST_METHOD(TestClipboardFallbackAndHandlers)
    {
        HostClipboard pClipboard;
        pClipboard.SetText(L"\U0001F30F globe");
        Assert::AreEqual(std::wstring(L"\U0001F30F globe"), pClipboard.GetText());

        std::string sHostClipboard;
        pClipboard.SetHandlers([&sHostClipboard](const std::string& sText) { sHostClipboard = sText; },
                               [&sHostClipboard]() { return sHostClipboard; });
        pClipboard.SetText(L"\U0001F30F");
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), sHostClipboard);
        Assert::AreEqual(std::wstring(L"\U0001F30F"), pClipboard.GetText());
    }

    TEST_METHOD(TestAudioPath)
    {
        PosixFileSystem pFileSystem(L"/base/dir");
        ra::services::ServiceLocator::ServiceOverride<ra::services::IFileSystem> pOverride(&pFileSystem);

        HostAudioSystem pAudio;
        pAudio.PlayAudioFile(L"Overlay\\unlock.wav"); // no handler - ignored

        std::string sPlayed;
        int nBeeps = 0;
        pAudio.SetHandlers([&sPlayed](const std::string& sPath) { sPlayed = sPath; }, [&nBeeps]() { ++nBeeps; });
        pAudio.PlayAudioFile(L"Overlay\\unlock.wav");
        pAudio.Beep();

        Assert::AreEqual(std::string("/base/dir/Overlay/unlock.wav"), sPlayed);
        Assert::AreEqual(1, nBeeps);
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
