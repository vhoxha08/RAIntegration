#ifndef RA_UI_PLATFORM_UI_HH
#define RA_UI_PLATFORM_UI_HH
#pragma once

#include <memory>

namespace ra {
namespace ui {

class IDesktop;
class IImageRepository;

namespace drawing {
class ISurfaceFactory;
}

/// <summary>
/// Factory hooks implemented by the UI backend linked into a non-Windows build (headless, ImGui, ...).
/// The Windows build creates its Win32/GDI services directly in Initialization.cpp.
/// </summary>
class PlatformUi
{
public:
    static std::unique_ptr<IDesktop> CreateDesktop();
    static std::unique_ptr<drawing::ISurfaceFactory> CreateSurfaceFactory();
    static std::unique_ptr<IImageRepository> CreateImageRepository();

    /// <summary>
    /// Releases backend resources. Called from Initialization::Shutdown before the services are torn down.
    /// </summary>
    static void Shutdown();
};

} // namespace ui
} // namespace ra

#endif // !RA_UI_PLATFORM_UI_HH
