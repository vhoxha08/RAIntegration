#include "HeadlessDesktop.hh"

#include "ui/headless/MemorySurface.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <spawn.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __APPLE__
 #include <mach-o/dyld.h>
#endif

extern char** environ;

namespace ra {
namespace ui {
namespace headless {

HeadlessDesktop::HeadlessDesktop() noexcept
    : m_nUIThreadId(std::this_thread::get_id())
{
}

void HeadlessDesktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    RA_LOG_INFO("[headless] ShowWindow: %s", ra::util::String::Narrow(vmViewModel.GetWindowTitle()).c_str());
}

ra::ui::DialogResult HeadlessDesktop::ShowModal(WindowViewModelBase& vmViewModel) const
{
    RA_LOG_INFO("[headless] ShowModal: %s", ra::util::String::Narrow(vmViewModel.GetWindowTitle()).c_str());
    vmViewModel.SetDialogResult(DialogResult::None);
    return DialogResult::None;
}

ra::ui::DialogResult HeadlessDesktop::ShowModal(WindowViewModelBase& vmViewModel,
                                                [[maybe_unused]] const WindowViewModelBase& vmParentWindow) const
{
    return ShowModal(vmViewModel);
}

void HeadlessDesktop::CloseWindow([[maybe_unused]] WindowViewModelBase& vmViewModel) const
{
}

void HeadlessDesktop::GetWorkArea(_Out_ ra::ui::Position& oUpperLeftCorner, _Out_ ra::ui::Size& oSize) const
{
    oUpperLeftCorner = {0, 0};
    oSize = {1920, 1080};
}

ra::ui::Size HeadlessDesktop::GetClientSize([[maybe_unused]] const WindowViewModelBase& vmViewModel) const
{
    return {0, 0};
}

std::wstring HeadlessDesktop::GetRunningExecutable() const
{
#ifdef __APPLE__
    uint32_t nSize = 0;
    _NSGetExecutablePath(nullptr, &nSize);
    std::string sPath(nSize, '\0');
    if (_NSGetExecutablePath(sPath.data(), &nSize) != 0)
        return L"";
    sPath.resize(strlen(sPath.c_str()));
#else
    std::string sPath(4096, '\0');
    const auto nLength = readlink("/proc/self/exe", sPath.data(), sPath.size() - 1);
    if (nLength <= 0)
        return L"";
    sPath.resize(gsl::narrow_cast<size_t>(nLength));
#endif

    return ra::util::String::Widen(sPath);
}

std::string HeadlessDesktop::GetOSVersionString() const
{
    struct utsname pInfo{};
    if (uname(&pInfo) != 0)
        return "Unknown";

    // i.e. "Darwin 25.6.0 arm64" or "Linux 6.8.0 x86_64"
    return ra::util::String::Printf("%s %s %s", pInfo.sysname, pInfo.release, pInfo.machine);
}

void HeadlessDesktop::OpenUrl(const std::string& sUrl) const
{
    // only hand web links to the system opener, and never through a shell
    if (sUrl.rfind("http://", 0) != 0 && sUrl.rfind("https://", 0) != 0)
    {
        RA_LOG_WARN("Refusing to open non-http URL: %s", sUrl.c_str());
        return;
    }

#ifdef __APPLE__
    const char* sOpener = "/usr/bin/open";
#else
    const char* sOpener = "xdg-open";
#endif

    std::string sUrlArg = sUrl;
    char* const vArgs[] = {const_cast<char*>(sOpener), sUrlArg.data(), nullptr};

    pid_t nPid = 0;
    const int nResult = posix_spawnp(&nPid, sOpener, nullptr, nullptr, vArgs, environ);
    if (nResult != 0)
    {
        RA_LOG_ERR("Could not launch %s (%d) for %s", sOpener, nResult, sUrl.c_str());
        return;
    }

    // the opener returns immediately; reap it so it doesn't linger as a zombie
    int nStatus = 0;
    waitpid(nPid, &nStatus, 0);
}

std::unique_ptr<ra::ui::drawing::ISurface> HeadlessDesktop::CaptureClientArea(
    [[maybe_unused]] const WindowViewModelBase& vmViewModel) const
{
    return std::make_unique<MemorySurface>(0, 0);
}

bool HeadlessDesktop::IsOnUIThread() const
{
    return std::this_thread::get_id() == m_nUIThreadId;
}

void HeadlessDesktop::InvokeOnUIThread(std::function<void()> fAction) const
{
    if (IsOnUIThread())
    {
        fAction();
        return;
    }

    PostToUIThread(std::move(fAction));
}

void HeadlessDesktop::PostToUIThread(std::function<void()> fAction) const
{
    std::lock_guard<std::mutex> lock(m_pMutex);
    m_vPendingActions.push_back(std::move(fAction));
}

void HeadlessDesktop::ProcessPendingActions()
{
    std::vector<std::function<void()>> vActions;
    {
        std::lock_guard<std::mutex> lock(m_pMutex);
        vActions.swap(m_vPendingActions);
    }

    for (auto& fAction : vActions)
        fAction();
}

void HeadlessDesktop::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_pMutex);
    m_vPendingActions.clear();
}

} // namespace headless
} // namespace ui
} // namespace ra
