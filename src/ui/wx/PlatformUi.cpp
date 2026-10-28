#include "ui/PlatformUi.hh"

#include "ui/drawing/RecordingSurface.hh"
#include "ui/wx/OverlayRenderer.hh"
#include "ui/wx/WxApp.hh"
#include "ui/wx/WxDesktop.hh"
#include "ui/wx/WxImageRepository.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace ui {

// The toolkit's windows are native wxWidgets windows. The overlay (popups, scoreboards, in-game overlay) is drawn into
// an image the host draws over the game (OverlayRenderer). Surfaces record what is drawn on them for the windows to
// paint (SurfaceRenderer).

static ra::ui::wx::WxDesktop* GetDesktop()
{
    if (!ra::services::ServiceLocator::Exists<IDesktop>())
        return nullptr;

    return dynamic_cast<ra::ui::wx::WxDesktop*>(&ra::services::ServiceLocator::GetMutable<IDesktop>());
}

std::unique_ptr<IDesktop> PlatformUi::CreateDesktop() { return std::make_unique<ra::ui::wx::WxDesktop>(); }

std::unique_ptr<drawing::ISurfaceFactory> PlatformUi::CreateSurfaceFactory()
{
    return std::make_unique<ra::ui::drawing::RecordingSurfaceFactory>();
}

std::unique_ptr<IImageRepository> PlatformUi::CreateImageRepository()
{
    return std::make_unique<ra::ui::wx::WxImageRepository>();
}

void PlatformUi::ProcessPendingActions()
{
    if (auto* pDesktop = GetDesktop())
        pDesktop->ProcessPendingActions();

    ra::ui::wx::PumpWxEvents();
}

void PlatformUi::SetMainWindow(void* hWnd)
{
    // only a host that is a wxWidgets application itself passes a window the overlay can be sized after
    if (ra::ui::wx::HostOwnsWxApp())
        ra::ui::wx::OverlayRenderer::Attach(static_cast<wxWindow*>(hWnd));
}

void PlatformUi::SetOverlayImageHandler(void (*fpHandleImage)(const uint32_t* pARGB, int nX, int nY, int nWidth,
                                                             int nHeight))
{
    ra::ui::wx::OverlayRenderer::SetImageHandler(fpHandleImage);
}

void PlatformUi::SetFileDialogHandler(int (*)(int, const char*, const char*, const char*, const char*, char*, int))
{
    // file dialogs are native wxWidgets dialogs
}

void PlatformUi::Shutdown()
{
    // Initialization::Shutdown calls this after the desktop's Shutdown destroyed the windows
    ra::ui::wx::OverlayRenderer::Detach();
    if (auto* pImageRepository = ra::ui::wx::WxImageRepository::Get())
        pImageRepository->ReleaseImages();

    ra::ui::wx::ShutdownWx();
}

} // namespace ui
} // namespace ra
