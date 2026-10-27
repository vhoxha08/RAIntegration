#ifndef RA_UI_WX_DESKTOP_HH
#define RA_UI_WX_DESKTOP_HH
#pragma once

#include "ui/headless/HeadlessDesktop.hh"

namespace ra {
namespace ui {
namespace wx {

class WxDialog;

/// <summary>
/// IDesktop that shows the toolkit's windows as native wxWidgets windows. View models without a wxWidgets view
/// yet are handled like the headless desktop does.
/// </summary>
class WxDesktop : public ra::ui::headless::HeadlessDesktop
{
public:
    void ShowWindow(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel, const WindowViewModelBase& vmParentWindow) const override;
    void CloseWindow(WindowViewModelBase& vmViewModel) const override;

    void Shutdown() override;

private:
    static WxDialog* CreateDialog(WindowViewModelBase& vmViewModel);
    static WxDialog* FindWindow(const WindowViewModelBase& vmViewModel);

    ra::ui::DialogResult ShowModalOnUIThread(WindowViewModelBase& vmViewModel) const;
    static ra::ui::DialogResult ShowMessageBox(WindowViewModelBase& vmViewModel);
    static ra::ui::DialogResult ShowFileDialog(WindowViewModelBase& vmViewModel);
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_DESKTOP_HH
