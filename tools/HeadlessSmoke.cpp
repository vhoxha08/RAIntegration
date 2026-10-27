/* Starts the integration with the headless UI backend and exercises the platform services.
 *
 * Usage: RAHeadlessSmoke <base-directory> [url]
 */
#include "services/Initialization.hh"

#include "services/Http.hh"
#include "services/IDebuggerDetector.hh"
#include "services/IFileSystem.hh"
#include "services/IHttpRequester.hh"
#include "services/ServiceLocator.hh"

#include "ui/IDesktop.hh"

#include "util/Strings.hh"

#include <cstdio>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s <base-directory> [url]\n", argv[0]);
        return 2;
    }

    ra::services::Initialization::SetBaseDirectory(ra::util::String::Widen(argv[1]));
    ra::services::Initialization::RegisterServices(EmulatorID::UnknownEmulator, "RAHeadlessSmoke");

    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    std::printf("base directory:   %s\n", ra::util::String::Narrow(pFileSystem.BaseDirectory()).c_str());
    std::printf("RACache/Data:     %s\n", pFileSystem.DirectoryExists(L"RACache\\Data") ? "exists" : "MISSING");
    std::printf("RALog.txt:        %lld bytes\n", static_cast<long long>(pFileSystem.GetFileSize(L"RACache\\RALog.txt")));

    const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
    std::printf("os version:       %s\n", pDesktop.GetOSVersionString().c_str());
    std::printf("executable:       %s\n", ra::util::String::Narrow(pDesktop.GetRunningExecutable()).c_str());

    const auto& pDebuggerDetector = ra::services::ServiceLocator::Get<ra::services::IDebuggerDetector>();
    std::printf("debugger/editor:  %s\n", pDebuggerDetector.IsDebuggerPresent() ? "detected" : "none");

    int nResult = 0;
    if (argc > 2)
    {
        ra::services::Http::Request pRequest(argv[2]);
        const auto pResponse = pRequest.Call();
        const auto nStatus = ra::etoi(pResponse.StatusCode());
        const auto& pHttp = ra::services::ServiceLocator::Get<ra::services::IHttpRequester>();
        std::printf("GET %s -> %u %s, %zu bytes\n", argv[2], nStatus, pHttp.GetStatusCodeText(nStatus).c_str(),
                    pResponse.Content().length());
        std::printf("first line:       %s\n", pResponse.Content().substr(0, pResponse.Content().find('\n')).c_str());
        nResult = (nStatus == 200) ? 0 : 1;
    }

    ra::services::Initialization::Shutdown();
    std::printf("shutdown complete\n");
    return nResult;
}
