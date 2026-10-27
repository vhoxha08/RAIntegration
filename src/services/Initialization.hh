#ifndef RA_SERVICES_INITIALIZATION_HH
#define RA_SERVICES_INITIALIZATION_HH
#pragma once

#include "RAInterface/RA_Emulators.h"

#include <string>

namespace ra {
namespace services {

class Initialization
{
public:
    static void RegisterCoreServices();

    static void RegisterServices(EmulatorID nEmulatorId, const char* sClientName);

    static void Shutdown();

    static bool IsInitialized() noexcept { return s_bIsInitialized; }

    static bool IsShuttingDown() noexcept { return s_bIsShuttingDown; }

    static void StartShutdown() noexcept { s_bIsShuttingDown = true; }

#ifndef _WIN32
    /// <summary>
    /// Sets the directory that holds RACache, Overlay and the RAPrefs files. Must be called before
    /// RegisterCoreServices. If not set, a per-user data directory is used (see PosixFileSystem).
    /// </summary>
    static void SetBaseDirectory(const std::wstring& sBaseDirectory) { s_sBaseDirectory = sBaseDirectory; }
#endif

private:
    static void InitializeNotifyTargets();

    static bool s_bIsInitialized;
    static bool s_bIsShuttingDown;
#ifndef _WIN32
    static inline std::wstring s_sBaseDirectory;
#endif
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_INITIALIZATION_HH
