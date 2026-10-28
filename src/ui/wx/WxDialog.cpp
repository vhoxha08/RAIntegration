#include "WxDialog.hh"

#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/thread.h>

namespace ra {
namespace ui {
namespace wx {

WxDialog::WxDialog(WindowViewModelBase& vmWindow, long nStyle) :
    wxDialog(nullptr, wxID_ANY, ToWx(vmWindow.GetWindowTitle()), wxDefaultPosition, wxDefaultSize, nStyle),
    m_vmWindow(vmWindow),
    m_pWatcher([this]() { ScheduleUpdate(); })
{
    m_vmWindow.AddNotifyTarget(*this);

    Bind(wxEVT_CLOSE_WINDOW, &WxDialog::OnCloseWindow, this);
    Bind(wxEVT_BUTTON, &WxDialog::OnButton, this, wxID_CANCEL);
}

WxDialog::~WxDialog()
{
    m_pWatcher.Detach();
    m_vmWindow.RemoveNotifyTarget(*this);
}

void WxDialog::ScheduleUpdate()
{
    if (m_bUpdateScheduled.exchange(true))
        return;

    CallAfter([this]() {
        m_bUpdateScheduled = false;
        if (!m_bClosing)
            UpdateControls();
    });
}

void WxDialog::OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs&)
{
    ScheduleUpdate();
}

void WxDialog::CloseWithResult(DialogResult nResult)
{
    if (m_bClosing)
        return;

    m_bClosing = true;
    m_vmWindow.SetDialogResult(nResult);

    if (IsModal())
    {
        EndModal(nResult == DialogResult::OK ? wxID_OK : wxID_CANCEL);
    }
    else
    {
        m_vmWindow.SetIsVisible(false);
        if (m_bModeless)
            Destroy();
        else
            Hide();
    }
}

void WxDialog::RunLater(std::function<void()> fAction)
{
    CallAfter([fAction = std::move(fAction)]() { fAction(); });
}

void WxDialog::OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args)
{
    // view models may change on other threads (i.e. when a server response arrives); windows may only be
    // changed on the main thread
    if (args.Property == WindowViewModelBase::WindowTitleProperty)
    {
        const wxString sTitle = ToWx(args.tNewValue);
        if (wxIsMainThread())
            SetTitle(sTitle);
        else
            CallAfter([this, sTitle]() { SetTitle(sTitle); });
    }

    ScheduleUpdate();
}

void WxDialog::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    // like the Win32 dialogs: the view model setting a result closes the window
    if (args.Property == WindowViewModelBase::DialogResultProperty &&
        ra::itoe<DialogResult>(args.tNewValue) != DialogResult::None && !m_bClosing)
    {
        const auto nResult = ra::itoe<DialogResult>(args.tNewValue);
        CallAfter([this, nResult]() { CloseWithResult(nResult); });
        return;
    }

    ScheduleUpdate();
}

void WxDialog::OnCloseWindow(wxCloseEvent&)
{
    CloseWithResult(DialogResult::Cancel);
}

void WxDialog::OnButton(wxCommandEvent&)
{
    CloseWithResult(DialogResult::Cancel);
}

} // namespace wx
} // namespace ui
} // namespace ra
