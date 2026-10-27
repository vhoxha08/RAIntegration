#ifndef RA_UI_WX_LOGIN_DIALOG_HH
#define RA_UI_WX_LOGIN_DIALOG_HH
#pragma once

#include "ui/viewmodels/LoginViewModel.hh"
#include "ui/wx/WxDialog.hh"

class wxCheckBox;
class wxTextCtrl;

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 LoginDialog.
/// </summary>
class LoginDialog : public WxDialog
{
public:
    explicit LoginDialog(ra::ui::viewmodels::LoginViewModel& vmLogin);

private:
    void OnLogin(wxCommandEvent& event);

    ra::ui::viewmodels::LoginViewModel& m_vmLogin;
    wxTextCtrl* m_pUsername = nullptr;
    wxTextCtrl* m_pPassword = nullptr;
    wxCheckBox* m_pRemember = nullptr;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_LOGIN_DIALOG_HH
