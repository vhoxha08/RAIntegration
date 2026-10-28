#ifndef RA_UI_PLATFORM_UI_HH
#define RA_UI_PLATFORM_UI_HH
#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace ra {
namespace ui {

class IDesktop;
class IImageRepository;

namespace drawing {
class ISurfaceFactory;
}

/// <summary>
/// Factory hooks implemented by the UI backend linked into a non-Windows build (headless, wxWidgets).
/// The Windows build creates its Win32/GDI services directly in Initialization.cpp.
/// </summary>
class PlatformUi
{
public:
    static std::unique_ptr<IDesktop> CreateDesktop();
    static std::unique_ptr<drawing::ISurfaceFactory> CreateSurfaceFactory();
    static std::unique_ptr<IImageRepository> CreateImageRepository();

    /// <summary>
    /// Runs work queued for the UI thread (IDesktop::InvokeOnUIThread). The host calls this once per frame
    /// from the thread that initialized the integration.
    /// </summary>
    static void ProcessPendingActions();

    /// <summary>
    /// The host's main window (_RA_UpdateHWnd): the overlay is shown over its client area. UI thread only.
    /// </summary>
    static void SetMainWindow(void* hWnd);

    /// <summary>
    /// Sets the host's function receiving the overlay's image (see _RA_SetOverlayImageHandler). Ignored without a GUI
    /// backend.
    /// </summary>
    static void SetOverlayImageHandler(void (*fpHandleImage)(const uint32_t* pARGB, int nWidth, int nHeight));

    /// <summary>
    /// Sets the host's file dialog function (see _RA_SetFileDialogHandler). Ignored without a GUI backend.
    /// </summary>
    static void SetFileDialogHandler(int (*fpShowFileDialog)(int nMode, const char* sTitle, const char* sFilters,
                                                             const char* sDefaultPath, const char* sDefaultName,
                                                             char* sResult, int nResultSize));

    /// <summary>
    /// Releases backend resources. Called from Initialization::Shutdown before the services are torn down.
    /// </summary>
    static void Shutdown();
};

} // namespace ui
} // namespace ra

#endif // !RA_UI_PLATFORM_UI_HH
