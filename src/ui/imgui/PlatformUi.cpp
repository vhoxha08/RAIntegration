#include "ui/PlatformUi.hh"

#include "ui/imgui/ImGuiDesktop.hh"
#include "ui/imgui/ImGuiImageRepository.hh"
#include "ui/imgui/ImGuiSurface.hh"
#include "ui/imgui/ImGuiTextures.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace ui {

namespace imgui {
TextureHandlers& GetTextureHandlers() noexcept
{
    static TextureHandlers s_pHandlers;
    return s_pHandlers;
}
} // namespace imgui

static ra::ui::imgui::ImGuiDesktop::ModalLoopHandler s_fModalLoopHandler;

static ra::ui::imgui::ImGuiDesktop* GetDesktop()
{
    if (!ra::services::ServiceLocator::Exists<IDesktop>())
        return nullptr;

    return dynamic_cast<ra::ui::imgui::ImGuiDesktop*>(&ra::services::ServiceLocator::GetMutable<IDesktop>());
}

std::unique_ptr<IDesktop> PlatformUi::CreateDesktop()
{
    auto pDesktop = std::make_unique<ra::ui::imgui::ImGuiDesktop>();
    pDesktop->SetModalLoopHandler(s_fModalLoopHandler);
    return pDesktop;
}

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
}

void PlatformUi::Render()
{
    if (auto* pDesktop = GetDesktop())
        pDesktop->Render();
}

void PlatformUi::SetModalLoopHandler(std::function<void(const std::function<bool()>&)> fHandler)
{
    s_fModalLoopHandler = std::move(fHandler);

    if (auto* pDesktop = GetDesktop())
        pDesktop->SetModalLoopHandler(s_fModalLoopHandler);
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

void PlatformUi::Shutdown()
{
}

} // namespace ui
} // namespace ra
