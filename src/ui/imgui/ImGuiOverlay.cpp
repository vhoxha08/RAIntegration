#include "ImGuiOverlay.hh"

#include "ui/imgui/ImGuiSurface.hh"
#include "ui/viewmodels/OverlayManager.hh"

#include "services/ServiceLocator.hh"

#include "ui/IDesktop.hh"

#include <imgui.h>

#include <mutex>
#include <thread>
#include <vector>

namespace ra {
namespace ui {
namespace imgui {

namespace {

std::mutex s_mutex;
bool s_bHasRenderThread = false;
std::thread::id s_nRenderThread;
std::vector<std::function<void()>> s_vPendingActions;

} // namespace

bool IsOnRenderThread()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_bHasRenderThread)
            return std::this_thread::get_id() == s_nRenderThread;
    }

    return ra::services::ServiceLocator::Exists<ra::ui::IDesktop>() &&
           ra::services::ServiceLocator::Get<ra::ui::IDesktop>().IsOnUIThread();
}

void RunOnRenderThread(std::function<void()> fAction)
{
    if (IsOnRenderThread())
    {
        fAction();
        return;
    }

    std::lock_guard<std::mutex> lock(s_mutex);
    s_vPendingActions.push_back(std::move(fAction));
}

void RenderOverlay()
{
    std::vector<std::function<void()>> vActions;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_bHasRenderThread = true;
        s_nRenderThread = std::this_thread::get_id();
        vActions.swap(s_vPendingActions);
    }
    for (auto& fAction : vActions)
        fAction();

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
