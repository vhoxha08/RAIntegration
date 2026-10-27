#include "ui/PlatformUi.hh"

#include "ui/headless/HeadlessDesktop.hh"
#include "ui/headless/HeadlessImageRepository.hh"
#include "ui/headless/MemorySurface.hh"

namespace ra {
namespace ui {

std::unique_ptr<IDesktop> PlatformUi::CreateDesktop()
{
    return std::make_unique<ra::ui::headless::HeadlessDesktop>();
}

std::unique_ptr<drawing::ISurfaceFactory> PlatformUi::CreateSurfaceFactory()
{
    return std::make_unique<ra::ui::headless::MemorySurfaceFactory>();
}

std::unique_ptr<IImageRepository> PlatformUi::CreateImageRepository()
{
    return std::make_unique<ra::ui::headless::HeadlessImageRepository>();
}

void PlatformUi::Shutdown()
{
}

} // namespace ui
} // namespace ra
