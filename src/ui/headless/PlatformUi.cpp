#include "ui/PlatformUi.hh"

#include "ui/headless/HeadlessDesktop.hh"
#include "ui/headless/HeadlessImageRepository.hh"
#include "ui/headless/MemorySurface.hh"

#include "services/ServiceLocator.hh"

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

void PlatformUi::ProcessPendingActions()
{
    if (!ra::services::ServiceLocator::Exists<IDesktop>())
        return;

    auto* pDesktop = dynamic_cast<ra::ui::headless::HeadlessDesktop*>(
        &ra::services::ServiceLocator::GetMutable<IDesktop>());
    if (pDesktop != nullptr)
        pDesktop->ProcessPendingActions();
}

void PlatformUi::Shutdown()
{
}

} // namespace ui
} // namespace ra
