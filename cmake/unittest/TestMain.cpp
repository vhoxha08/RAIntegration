/* Test runner for the CppUnitTest.h stand-in.
 *
 * Usage: <test-binary> [-v] [filter...]
 *   filter  only run tests whose "Class::Method" name contains one of the filters
 *   -v      print every test name as it runs
 */
#include "CppUnitTest.h"

#include <chrono>
#include <cstdio>
#include <exception>
#include <string>

namespace {

std::string ToUtf8(const std::wstring& sText)
{
    std::string sUtf8;
    sUtf8.reserve(sText.size());
    for (const wchar_t wc : sText)
    {
        auto c = static_cast<uint32_t>(wc);
        if (c < 0x80)
        {
            sUtf8.push_back(static_cast<char>(c));
        }
        else if (c < 0x800)
        {
            sUtf8.push_back(static_cast<char>(0xC0 | (c >> 6)));
            sUtf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
        else if (c < 0x10000)
        {
            sUtf8.push_back(static_cast<char>(0xE0 | (c >> 12)));
            sUtf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            sUtf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
        else
        {
            sUtf8.push_back(static_cast<char>(0xF0 | (c >> 18)));
            sUtf8.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            sUtf8.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            sUtf8.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return sUtf8;
}

} // namespace

int main(int argc, char* argv[])
{
    bool bVerbose = false;
    std::vector<std::string> vFilters;
    for (int i = 1; i < argc; ++i)
    {
        const std::string sArg = argv[i];
        if (sArg == "-v")
            bVerbose = true;
        else
            vFilters.push_back(sArg);
    }

    size_t nRun = 0, nFailed = 0;
    const auto tStart = std::chrono::steady_clock::now();

    for (const auto& pTest : ra_test::Registry())
    {
        const std::string sName = std::string(pTest.sClassName) + "::" + pTest.sMethodName;
        if (!vFilters.empty())
        {
            bool bMatch = false;
            for (const auto& sFilter : vFilters)
                bMatch |= (sName.find(sFilter) != std::string::npos);
            if (!bMatch)
                continue;
        }

        ++nRun;
        if (bVerbose)
            std::printf("RUN  %s\n", sName.c_str());

        std::string sFailure;
        try
        {
            pTest.fRun();
        }
        catch (const ra_test::AssertFailedException& ex)
        {
            sFailure = ToUtf8(ex.sMessage);
            if (ex.sFile)
                sFailure += " (" + std::string(ex.sFile) + ":" + std::to_string(ex.nLine) + ")";
        }
        catch (const std::exception& ex)
        {
            sFailure = std::string("unhandled exception: ") + ex.what();
        }
        catch (...)
        {
            sFailure = "unhandled non-standard exception";
        }

        if (!sFailure.empty())
        {
            ++nFailed;
            std::printf("FAIL %s\n     %s\n     at %s:%d\n", sName.c_str(), sFailure.c_str(), pTest.sFile,
                        pTest.nLine);
        }
    }

    const auto nMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tStart).count();
    std::printf("\n%zu tests run, %zu passed, %zu failed (%lld ms)\n", nRun, nRun - nFailed, nFailed,
                static_cast<long long>(nMs));
    return nFailed ? 1 : 0;
}
