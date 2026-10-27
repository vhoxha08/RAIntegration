#include "ImGuiDesktop.hh"

#include "ui/imgui/ImGuiSurface.hh"
#include "ui/imgui/LoginView.hh"
#include "ui/viewmodels/OverlayManager.hh"
#include "ui/imgui/MessageBoxView.hh"

#include "services/ServiceLocator.hh"

#include "util/Log.hh"
#include "util/Strings.hh"

#include <imgui.h>

#include <condition_variable>
#include <mutex>

namespace ra {
namespace ui {
namespace imgui {

std::string ToUtf8(const std::wstring& sText) { return ra::util::String::Narrow(sText); }
std::wstring FromUtf8(const std::string& sText) { return ra::util::String::Widen(sText); }

void ImGuiWindowView::RunAfterFrame(std::function<void()> fAction)
{
    const auto* pDesktop = dynamic_cast<const ra::ui::headless::HeadlessDesktop*>(
        &ra::services::ServiceLocator::Get<ra::ui::IDesktop>());
    if (pDesktop != nullptr)
        pDesktop->PostToUIThread(std::move(fAction));
}

std::unique_ptr<ImGuiWindowView> ImGuiDesktop::CreateView(WindowViewModelBase& vmViewModel)
{
    if (auto* vmLogin = dynamic_cast<ra::ui::viewmodels::LoginViewModel*>(&vmViewModel))
        return std::make_unique<LoginView>(*vmLogin);

    if (auto* vmMessageBox = dynamic_cast<ra::ui::viewmodels::MessageBoxViewModel*>(&vmViewModel))
        return std::make_unique<MessageBoxView>(*vmMessageBox);

    return nullptr;
}

void ImGuiDesktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    for (const auto& pWindow : m_vWindows)
    {
        if (&pWindow->pView->GetViewModel() == &vmViewModel)
            return; // already open
    }

    auto pView = CreateView(vmViewModel);
    if (pView == nullptr)
    {
        HeadlessDesktop::ShowWindow(vmViewModel);
        return;
    }

    auto pWindow = std::make_unique<OpenWindow>();
    pWindow->pView = std::move(pView);
    m_vWindows.push_back(std::move(pWindow));
}

ra::ui::DialogResult ImGuiDesktop::ShowModal(WindowViewModelBase& vmViewModel) const
{
    if (IsOnUIThread())
        return ShowModalOnUIThread(vmViewModel);

    // like the Win32 desktop: show it on the UI thread and wait for it to be closed
    std::mutex mutex;
    std::condition_variable condition;
    bool bDone = false;
    ra::ui::DialogResult nResult = ra::ui::DialogResult::None;

    PostToUIThread([this, &vmViewModel, &mutex, &condition, &bDone, &nResult]() {
        const auto nModalResult = ShowModalOnUIThread(vmViewModel);

        std::lock_guard<std::mutex> lock(mutex);
        nResult = nModalResult;
        bDone = true;
        condition.notify_all();
    });

    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&bDone]() { return bDone; });
    return nResult;
}

ra::ui::DialogResult ImGuiDesktop::ShowModal(WindowViewModelBase& vmViewModel,
                                             [[maybe_unused]] const WindowViewModelBase& vmParentWindow) const
{
    return ShowModal(vmViewModel);
}

ra::ui::DialogResult ImGuiDesktop::ShowModalOnUIThread(WindowViewModelBase& vmViewModel) const
{
    auto pView = CreateView(vmViewModel);
    if (pView == nullptr || !m_fModalLoop)
        return HeadlessDesktop::ShowModal(vmViewModel);

    vmViewModel.SetDialogResult(ra::ui::DialogResult::None);

    auto pWindow = std::make_unique<OpenWindow>();
    pWindow->pView = std::move(pView);
    pWindow->bModal = true;
    const OpenWindow* pModal = pWindow.get();
    m_vWindows.push_back(std::move(pWindow));

    m_fModalLoop([pModal]() { return !pModal->pView->IsClosed(); });

    for (auto pIter = m_vWindows.begin(); pIter != m_vWindows.end(); ++pIter)
    {
        if (pIter->get() == pModal)
        {
            m_vWindows.erase(pIter);
            break;
        }
    }

    return vmViewModel.GetDialogResult();
}

void ImGuiDesktop::CloseWindow(WindowViewModelBase& vmViewModel) const
{
    for (const auto& pWindow : m_vWindows)
    {
        if (&pWindow->pView->GetViewModel() == &vmViewModel)
            pWindow->pView->OnCloseRequested();
    }
}

void ImGuiDesktop::Render() const
{
    RenderOverlay();

    // only the most recently opened modal window accepts input
    size_t nTopModal = m_vWindows.size();
    for (size_t i = 0; i < m_vWindows.size(); ++i)
    {
        if (m_vWindows.at(i)->bModal)
            nTopModal = i;
    }

    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    for (size_t i = 0; i < m_vWindows.size(); ++i)
    {
        auto& pWindow = *m_vWindows.at(i);
        auto& pView = *pWindow.pView;
        if (pView.IsClosed())
            continue;

        const bool bInteractive = (nTopModal == m_vWindows.size() || i >= nTopModal);
        const std::string sTitle = ToUtf8(pView.GetViewModel().GetWindowTitle()) + "##ra" +
                                   std::to_string(reinterpret_cast<uintptr_t>(&pView));

        ImGuiWindowFlags nFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        if (pWindow.bModal)
        {
            nFlags |= ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::SetNextWindowPos(pViewport->GetWorkCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), pViewport->WorkSize);
        }

        ImGui::BeginDisabled(!bInteractive);
        bool bOpen = true;
        if (ImGui::Begin(sTitle.c_str(), bInteractive ? &bOpen : nullptr, nFlags))
        {
            if (pWindow.bModal && i == nTopModal && ImGui::IsWindowAppearing())
                ImGui::SetWindowFocus();

            pView.RenderContents();
        }
        ImGui::End();
        ImGui::EndDisabled();

        if (!bOpen)
            pView.OnCloseRequested();
    }

    // modal windows are removed by ShowModal; drop closed modeless ones
    for (auto pIter = m_vWindows.begin(); pIter != m_vWindows.end();)
    {
        if (!(*pIter)->bModal && (*pIter)->pView->IsClosed())
            pIter = m_vWindows.erase(pIter);
        else
            ++pIter;
    }
}

void ImGuiDesktop::RenderOverlay() const
{
    if (!ra::services::ServiceLocator::Exists<ra::ui::viewmodels::OverlayManager>())
        return;

    // The Win32 build paints popups incrementally into a layered window. ImGui starts each frame empty,
    // so the whole overlay is rendered every frame onto a surface covering the game area (below the menu
    // bar), and drawn behind the toolkit's windows.
    const ImGuiViewport* pViewport = ImGui::GetMainViewport();
    ImGuiSurface pSurface(static_cast<int>(pViewport->WorkSize.x), static_cast<int>(pViewport->WorkSize.y), true);

    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.Render(pSurface, true);

    pSurface.Replay(*ImGui::GetBackgroundDrawList(), pViewport->WorkPos.x, pViewport->WorkPos.y);
}

void ImGuiDesktop::Shutdown()
{
    m_vWindows.clear();
    HeadlessDesktop::Shutdown();
}

} // namespace imgui
} // namespace ui
} // namespace ra
