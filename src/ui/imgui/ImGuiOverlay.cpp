#include "ImGuiOverlay.hh"

#include "ui/imgui/ImGuiSurface.hh"
#include "ui/viewmodels/OverlayManager.hh"

#include "services/ServiceLocator.hh"

#include <imgui.h>

namespace ra {
namespace ui {
namespace imgui {

void RenderOverlay()
{
    if (!ra::services::ServiceLocator::Exists<ra::ui::viewmodels::OverlayManager>())
        return;

    // The Win32 build paints popups incrementally into a layered window. ImGui starts each frame empty,
    // so the whole overlay is rendered every frame onto a surface covering the game area (below the menu
    // bar), and drawn behind any other ImGui windows.
    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGuiSurface pSurface(static_cast<int>(pViewport->WorkSize.x), static_cast<int>(pViewport->WorkSize.y), true);

    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.Render(pSurface, true);

    pSurface.Replay(*ImGui::GetBackgroundDrawList(), pViewport->WorkPos.x, pViewport->WorkPos.y);
}

} // namespace imgui
} // namespace ui
} // namespace ra
