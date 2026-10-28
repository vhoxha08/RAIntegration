#ifndef RA_UI_WX_DIALOG_HH
#define RA_UI_WX_DIALOG_HH
#pragma once

#include "ui/WindowViewModelBase.hh"
#include "ui/wx/WxBinding.hh"

#include <wx/dialog.h>

#include <atomic>
#include <functional>
#include <string>

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// Base for the wxWidgets windows of the toolkit (the counterpart of the Win32 DialogBase). Keeps the title in
/// sync with the view model, and closes when the view model sets a dialog result.
/// </summary>
class WxDialog : public wxDialog, protected ViewModelBase::NotifyTarget
{
public:
    WxDialog(WindowViewModelBase& vmWindow, long nStyle = wxDEFAULT_DIALOG_STYLE);
    ~WxDialog() override;
    WxDialog(const WxDialog&) noexcept = delete;
    WxDialog& operator=(const WxDialog&) noexcept = delete;
    WxDialog(WxDialog&&) noexcept = delete;
    WxDialog& operator=(WxDialog&&) noexcept = delete;

    WindowViewModelBase& GetViewModel() noexcept { return m_vmWindow; }

    /// <summary>
    /// Called by WxDesktop when the window is shown modeless (closing destroys it).
    /// </summary>
    void SetModeless() noexcept { m_bModeless = true; }

protected:
    /// <summary>
    /// Restores the window's size and position saved under <paramref name="sKey" /> (the same keys as the Win32
    /// dialogs, in the "Window Positions" preference) and saves them when the user moves or resizes the window.
    /// Call at the end of the constructor, after the layout.
    /// </summary>
    void RememberSizeAndPosition(const char* sKey);

    /// <summary>
    /// Closes the window with the given result.
    /// </summary>
    void CloseWithResult(DialogResult nResult);

    /// <summary>
    /// Runs an action once the current event has been handled (i.e. work that shows other modal windows).
    /// </summary>
    void RunLater(std::function<void()> fAction);

    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    /// <summary>
    /// Updates the controls from the view model(s). Called after any change of the window's view model or of
    /// anything passed to Watch, once per batch of changes. Views call it at the end of their constructor.
    /// </summary>
    virtual void UpdateControls() {}

    /// <summary>
    /// Updates the controls after the current event (coalesces multiple changes; safe from any thread).
    /// </summary>
    void ScheduleUpdate();

    void Watch(ViewModelBase& vmViewModel) { m_pWatcher.Watch(vmViewModel); }
    void Watch(ViewModelCollectionBase& vmCollection) { m_pWatcher.Watch(vmCollection); }

    WindowViewModelBase& m_vmWindow;

private:
    void OnCloseWindow(wxCloseEvent& event);
    void OnButton(wxCommandEvent& event);

    ChangeWatcher m_pWatcher;
    std::atomic<bool> m_bUpdateScheduled{false};
    std::string m_sSizeAndPositionKey;
    bool m_bModeless = false;
    bool m_bClosing = false;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_DIALOG_HH
