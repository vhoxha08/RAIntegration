#include "ui/PlatformUi.hh"

#include "ui/imgui/ImGuiImageRepository.hh"
#include "ui/imgui/ImGuiOverlay.hh"
#include "ui/imgui/ImGuiSurface.hh"
#include "ui/imgui/ImGuiTextures.hh"
#include "ui/wx/WxApp.hh"
#include "ui/wx/WxDesktop.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace ui {

// The toolkit's windows are native wxWidgets windows. The overlay (popups, scoreboards, in-game overlay) is
// still drawn over the game by the host's Dear ImGui frame, so surfaces and images come from the ImGui code.

namespace imgui {
TextureHandlers& GetTextureHandlers() noexcept
{
    static TextureHandlers s_pHandlers;
    return s_pHandlers;
}
} // namespace imgui

static ra::ui::wx::WxDesktop* GetDesktop()
{
    if (!ra::services::ServiceLocator::Exists<IDesktop>())
        return nullptr;

    return dynamic_cast<ra::ui::wx::WxDesktop*>(&ra::services::ServiceLocator::GetMutable<IDesktop>());
}

std::unique_ptr<IDesktop> PlatformUi::CreateDesktop() { return std::make_unique<ra::ui::wx::WxDesktop>(); }

std::unique_ptr<drawing::ISurfaceFactory> PlatformUi::CreateSurfaceFactory()
{
    return std::make_unique<ra::ui::imgui::ImGuiSurfaceFactory>();
}

std::unique_ptr<IImageRepository> PlatformUi::CreateImageRepository()
{
    return std::make_unique<ra::ui::imgui::ImGuiImageRepository>();
}

void PlatformUi::ProcessPendingActions()
{
    if (auto* pDesktop = GetDesktop())
        pDesktop->ProcessPendingActions();

    ra::ui::wx::PumpWxEvents();
}

void PlatformUi::Render() { ra::ui::imgui::RenderOverlay(); }

void PlatformUi::SetModalLoopHandler(std::function<void(const std::function<bool()>&)>)
{
    // modal windows run the native modal loop
}

void PlatformUi::SetTextureHandlers(uintptr_t (*fpCreateFromFile)(const char*, int*, int*),
                                    uintptr_t (*fpCreateFromPixels)(const uint32_t*, int, int),
                                    void (*fpDestroy)(uintptr_t))
{
    auto& pHandlers = ra::ui::imgui::GetTextureHandlers();
    pHandlers.fpCreateFromFile = fpCreateFromFile;
    pHandlers.fpCreateFromPixels = fpCreateFromPixels;
    pHandlers.fpDestroy = fpDestroy;
}

void PlatformUi::SetFileDialogHandler(int (*)(int, const char*, const char*, const char*, const char*, char*, int))
{
    // file dialogs are native wxWidgets dialogs
}

void PlatformUi::Shutdown()
{
    // Initialization::Shutdown calls this after the desktop's Shutdown destroyed the windows
    ra::ui::wx::ShutdownWx();
}

} // namespace ui
} // namespace ra
