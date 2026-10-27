#ifndef RA_UI_HEADLESS_DESKTOP_HH
#define RA_UI_HEADLESS_DESKTOP_HH
#pragma once

#include "ui/IDesktop.hh"

#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace ra {
namespace ui {
namespace headless {

/// <summary>
/// IDesktop without any windows. Windows and dialogs are logged instead of shown, and modal dialogs return
/// DialogResult::None (the "declined" answer). Work queued with InvokeOnUIThread runs when the host calls
/// ProcessPendingActions from the thread that initialized the integration.
/// </summary>
class HeadlessDesktop : public IDesktop
{
public:
    HeadlessDesktop() noexcept;

    void ShowWindow(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override;
    ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel, const WindowViewModelBase& vmParentWindow) const override;
    void CloseWindow(WindowViewModelBase& vmViewModel) const override;

    void GetWorkArea(_Out_ ra::ui::Position& oUpperLeftCorner, _Out_ ra::ui::Size& oSize) const override;
    ra::ui::Size GetClientSize(const WindowViewModelBase& vmViewModel) const override;

    std::wstring GetRunningExecutable() const override;
    std::string GetOSVersionString() const override;
    void OpenUrl(const std::string& sUrl) const override;

    std::unique_ptr<ra::ui::drawing::ISurface> CaptureClientArea(const WindowViewModelBase& vmViewModel) const override;

    bool IsOnUIThread() const override;
    void InvokeOnUIThread(std::function<void()> fAction) const override;

    /// <summary>
    /// Runs the actions queued by InvokeOnUIThread. Must be called from the UI thread.
    /// </summary>
    void ProcessPendingActions();

    void Shutdown() override;

private:
    std::thread::id m_nUIThreadId;
    mutable std::mutex m_pMutex;
    mutable std::vector<std::function<void()>> m_vPendingActions;
};

} // namespace headless
} // namespace ui
} // namespace ra

#endif // !RA_UI_HEADLESS_DESKTOP_HH
