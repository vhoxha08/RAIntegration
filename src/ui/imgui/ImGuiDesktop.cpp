#include "ImGuiDesktop.hh"

#include "ui/imgui/BrokenAchievementsView.hh"
#include "ui/imgui/GameChecksumView.hh"
#include "ui/imgui/ImGuiSurface.hh"
#include "ui/imgui/LoginView.hh"
#include "ui/imgui/OverlaySettingsView.hh"
#include "ui/imgui/RichPresenceView.hh"
#include "ui/imgui/UnknownGameView.hh"
#include "ui/viewmodels/OverlayManager.hh"
#include "ui/imgui/MessageBoxView.hh"
#include "ui/viewmodels/FileDialogViewModel.hh"

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

ImGuiDesktop::FileDialogHandler ImGuiDesktop::s_fpFileDialog = nullptr;

std::unique_ptr<ImGuiWindowView> ImGuiDesktop::CreateView(WindowViewModelBase& vmViewModel)
{
    if (auto* vmLogin = dynamic_cast<ra::ui::viewmodels::LoginViewModel*>(&vmViewModel))
        return std::make_unique<LoginView>(*vmLogin);

    if (auto* vmMessageBox = dynamic_cast<ra::ui::viewmodels::MessageBoxViewModel*>(&vmViewModel))
        return std::make_unique<MessageBoxView>(*vmMessageBox);

    if (auto* vmOverlaySettings = dynamic_cast<ra::ui::viewmodels::OverlaySettingsViewModel*>(&vmViewModel))
        return std::make_unique<OverlaySettingsView>(*vmOverlaySettings);

    if (auto* vmChecksum = dynamic_cast<ra::ui::viewmodels::GameChecksumViewModel*>(&vmViewModel))
        return std::make_unique<GameChecksumView>(*vmChecksum);

    if (auto* vmUnknownGame = dynamic_cast<ra::ui::viewmodels::UnknownGameViewModel*>(&vmViewModel))
        return std::make_unique<UnknownGameView>(*vmUnknownGame);

    if (auto* vmRichPresence = dynamic_cast<ra::ui::viewmodels::RichPresenceMonitorViewModel*>(&vmViewModel))
        return std::make_unique<RichPresenceView>(*vmRichPresence);

    if (auto* vmBroken = dynamic_cast<ra::ui::viewmodels::BrokenAchievementsViewModel*>(&vmViewModel))
        return std::make_unique<BrokenAchievementsView>(*vmBroken);

    return nullptr;
}

void ImGuiDesktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    for (const auto& pWindow : m_vWindows)
    {
        if (&pWindow->pView->GetViewModel() == &vmViewModel && !pWindow->pView->IsClosed())
        {
            pWindow->bFocus = true; // already open, bring it to the front
            return;
        }
    }

    auto pView = CreateView(vmViewModel);
    if (pView == nullptr)
    {
        HeadlessDesktop::ShowWindow(vmViewModel);
        return;
    }

    vmViewModel.SetDialogResult(ra::ui::DialogResult::None);

    auto pWindow = std::make_unique<OpenWindow>();
    pWindow->pView = std::move(pView);
    pWindow->bFocus = true;
    m_vWindows.push_back(std::move(pWindow));

    vmViewModel.SetIsVisible(true);
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
    if (dynamic_cast<ra::ui::viewmodels::FileDialogViewModel*>(&vmViewModel) != nullptr && s_fpFileDialog)
        return ShowFileDialog(vmViewModel);

    auto pView = CreateView(vmViewModel);
    if (pView == nullptr || !m_fModalLoop)
        return HeadlessDesktop::ShowModal(vmViewModel);

    vmViewModel.SetDialogResult(ra::ui::DialogResult::None);

    auto pWindow = std::make_unique<OpenWindow>();
    pWindow->pView = std::move(pView);
    pWindow->bModal = true;
    const OpenWindow* pModal = pWindow.get();
    m_vWindows.push_back(std::move(pWindow));

    vmViewModel.SetIsVisible(true);
    m_fModalLoop([pModal]() { return !pModal->pView->IsClosed(); });
    vmViewModel.SetIsVisible(false);

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

ra::ui::DialogResult ImGuiDesktop::ShowFileDialog(WindowViewModelBase& vmViewModel)
{
    auto& vmFileDialog = dynamic_cast<ra::ui::viewmodels::FileDialogViewModel&>(vmViewModel);

    int nMode = 0;
    switch (vmFileDialog.GetMode())
    {
        case ra::ui::viewmodels::FileDialogViewModel::Mode::Open: nMode = 1; break;
        case ra::ui::viewmodels::FileDialogViewModel::Mode::Save: nMode = 2; break;
        case ra::ui::viewmodels::FileDialogViewModel::Mode::Folder: nMode = 3; break;
        default: return ra::ui::DialogResult::None;
    }

    // file types are stored as "*.ext1;*.ext2", the host expects "ext1,ext2"
    std::string sFilters;
    for (const auto& pType : vmFileDialog.GetFileTypes())
    {
        std::string sSpec;
        const std::string sPatterns = ra::util::String::Narrow(pType.second);
        size_t nStart = 0;
        while (nStart <= sPatterns.length())
        {
            size_t nEnd = sPatterns.find(';', nStart);
            if (nEnd == std::string::npos)
                nEnd = sPatterns.length();
            const std::string sPattern = sPatterns.substr(nStart, nEnd - nStart);
            nStart = nEnd + 1;

            const auto nIndex = sPattern.find_last_of('.');
            const std::string sExtension = (nIndex == std::string::npos) ? sPattern : sPattern.substr(nIndex + 1);
            if (sExtension.empty() || sExtension == "*")
                continue;

            if (!sSpec.empty())
                sSpec.push_back(',');
            sSpec.append(sExtension);
        }

        if (!sSpec.empty())
            sFilters.append(ra::util::String::Narrow(pType.first)).append("\t").append(sSpec).append("\n");
    }

    const std::string sTitle = ra::util::String::Narrow(vmFileDialog.GetWindowTitle());
    const std::string sDirectory = ra::util::String::Narrow(vmFileDialog.GetInitialDirectory());
    const std::string sFileName = ra::util::String::Narrow(vmFileDialog.GetFileName());

    std::string sResult(4096, '\0');
    if (!s_fpFileDialog(nMode, sTitle.c_str(), sFilters.c_str(), sDirectory.c_str(), sFileName.c_str(), sResult.data(),
                        gsl::narrow_cast<int>(sResult.size())))
    {
        vmFileDialog.SetDialogResult(ra::ui::DialogResult::Cancel);
        return ra::ui::DialogResult::Cancel;
    }

    sResult.resize(strlen(sResult.c_str()));
    vmFileDialog.SetFileName(ra::util::String::Widen(sResult));
    vmFileDialog.SetDialogResult(ra::ui::DialogResult::OK);
    return ra::ui::DialogResult::OK;
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
        else
        {
            const ImVec2 vSize = pView.GetInitialSize();
            if (vSize.x > 0.0f)
            {
                ImGui::SetNextWindowSize(vSize, ImGuiCond_Appearing);
            }
            else
            {
                nFlags |= ImGuiWindowFlags_AlwaysAutoResize;
            }

            // cascade new windows from the top left of the game area
            const float fOffset = ImGui::GetFontSize() * 2.0f * static_cast<float>(i % 8);
            ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + 20.0f + fOffset, pViewport->WorkPos.y + 20.0f + fOffset),
                                    ImGuiCond_Appearing);
        }

        if (pWindow.bFocus)
        {
            ImGui::SetNextWindowFocus();
            pWindow.bFocus = false;
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
        {
            (*pIter)->pView->GetViewModel().SetIsVisible(false);
            pIter = m_vWindows.erase(pIter);
        }
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
