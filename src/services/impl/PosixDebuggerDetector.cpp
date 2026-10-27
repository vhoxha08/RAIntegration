#include "PosixDebuggerDetector.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <vector>

#ifdef __APPLE__
 #include <libproc.h>
 #include <sys/sysctl.h>
 #include <unistd.h>
#else
 #include <dirent.h>
#endif

namespace ra {
namespace services {
namespace impl {

static bool IsSuspiciousProcessName(std::string sName)
{
    std::transform(sName.begin(), sName.end(), sName.begin(),
                   [](unsigned char c) { return gsl::narrow_cast<char>(std::tolower(c)); });

    // same idea as the Windows check: look for the default process names of common memory editors
    if (sName.find("cheat") != std::string::npos && sName.find("engine") != std::string::npos)
        return true; // Cheat Engine

    if (sName.find("bit slicer") != std::string::npos || sName.find("bitslicer") != std::string::npos)
        return true; // Bit Slicer (macOS)

    if (sName == "scanmem" || sName.find("gameconqueror") != std::string::npos || sName.find("pince") != std::string::npos)
        return true; // scanmem / GameConqueror / PINCE (Linux)

    return false;
}

#ifdef __APPLE__

static bool IsBeingTraced()
{
    kinfo_proc info{};
    size_t nSize = sizeof(info);
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    if (sysctl(mib, 4, &info, &nSize, nullptr, 0) != 0)
        return false;

    return (info.kp_proc.p_flag & P_TRACED) != 0;
}

static bool IsSuspiciousProcessRunning()
{
    const int nCount = proc_listallpids(nullptr, 0);
    if (nCount <= 0)
        return false;

    std::vector<pid_t> vPids(gsl::narrow_cast<size_t>(nCount) + 16);
    const int nActual = proc_listallpids(vPids.data(), gsl::narrow_cast<int>(vPids.size() * sizeof(pid_t)));
    for (int i = 0; i < nActual; ++i)
    {
        char sName[2 * MAXCOMLEN + 1]{};
        if (proc_name(vPids.at(i), sName, sizeof(sName)) > 0 && IsSuspiciousProcessName(sName))
        {
            RA_LOG_WARN("Memory editor detected: %s", sName);
            return true;
        }
    }

    return false;
}

#else

static bool IsBeingTraced()
{
    std::ifstream pStatus("/proc/self/status");
    std::string sLine;
    while (std::getline(pStatus, sLine))
    {
        if (sLine.rfind("TracerPid:", 0) == 0)
            return std::stoi(sLine.substr(10)) != 0;
    }

    return false;
}

static bool IsSuspiciousProcessRunning()
{
    DIR* pDir = opendir("/proc");
    if (pDir == nullptr)
        return false;

    bool bFound = false;
    while (const dirent* pEntry = readdir(pDir))
    {
        if (!std::isdigit(static_cast<unsigned char>(pEntry->d_name[0])))
            continue;

        std::ifstream pComm(std::string("/proc/") + pEntry->d_name + "/comm");
        std::string sName;
        if (std::getline(pComm, sName) && IsSuspiciousProcessName(sName))
        {
            RA_LOG_WARN("Memory editor detected: %s", sName.c_str());
            bFound = true;
            break;
        }
    }

    closedir(pDir);
    return bFound;
}

#endif

bool PosixDebuggerDetector::IsDebuggerPresent() const
{
#ifdef NDEBUG // allow debugger-limited functionality when using a DEBUG build
    if (IsBeingTraced())
    {
        RA_LOG_WARN("Debugger detected");
        return true;
    }
#endif

    return IsSuspiciousProcessRunning();
}

} // namespace impl
} // namespace services
} // namespace ra
