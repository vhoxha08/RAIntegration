#ifndef RA_UI_IMGUI_DESKTOP_HH
#define RA_UI_IMGUI_DESKTOP_HH
#pragma once

#include "ui/headless/HeadlessDesktop.hh"
#include "ui/imgui/ImGuiWindowView.hh"

#include <memory>
#include <vector>

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// IDesktop that shows windows with Dear ImGui inside the host's window. The host draws them by calling
/// Render from its ImGui frame, and lends its event loop for modal windows (SetModalLoopHandler).
/// View models without an ImGui view yet are handled like the headless desktop does.
/// </summary>
class ImGuiDesktop : public ra::ui::headless::HeadlessDesktop
{
public:
    using ModalLoopHandler = std::function<void(const std::function<bool()>& fIsOpen)>;

    void SetModalLoopHandler(ModalLoopHandler fHandler) { m_fModalLoop = std::move(fHandler); }

    void ShowWindow(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel, const WindowViewModelBase& vmParentWindow) const override;
    void CloseWindow(WindowViewModelBase& vmViewModel) const override;

    /// <summary>
    /// Draws the open windows. Must be called inside an ImGui frame on the UI thread.
    /// </summary>
    void Render() const;

    void Shutdown() override;

private:
    static std::unique_ptr<ImGuiWindowView> CreateView(WindowViewModelBase& vmViewModel);

    ra::ui::DialogResult ShowModalOnUIThread(WindowViewModelBase& vmViewModel) const;

    struct OpenWindow
    {
        std::unique_ptr<ImGuiWindowView> pView;
        bool bModal = false;
    };

    ModalLoopHandler m_fModalLoop;
    mutable std::vector<std::unique_ptr<OpenWindow>> m_vWindows;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_DESKTOP_HH
