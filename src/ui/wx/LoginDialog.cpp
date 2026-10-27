#include "LoginDialog.hh"

#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

LoginDialog::LoginDialog(ra::ui::viewmodels::LoginViewModel& vmLogin) : WxDialog(vmLogin), m_vmLogin(vmLogin)
{
    SetTitle("Login");

    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    pSizer->Add(new wxStaticText(this, wxID_ANY, "Log in to RetroAchievements"), 0, wxALL, 10);

    auto* pFields = new wxFlexGridSizer(2, wxSize(8, 8));
    pFields->AddGrowableCol(1);
    pFields->Add(new wxStaticText(this, wxID_ANY, "Username:"), 0, wxALIGN_CENTER_VERTICAL);
    m_pUsername = new wxTextCtrl(this, wxID_ANY, ToWx(vmLogin.GetUsername()), wxDefaultPosition, wxSize(220, -1));
    pFields->Add(m_pUsername, 1, wxEXPAND);
    pFields->Add(new wxStaticText(this, wxID_ANY, "Password:"), 0, wxALIGN_CENTER_VERTICAL);
    m_pPassword = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    pFields->Add(m_pPassword, 1, wxEXPAND);
    pSizer->Add(pFields, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    m_pRemember = new wxCheckBox(this, wxID_ANY, "Remember password");
    m_pRemember->SetValue(vmLogin.IsPasswordRemembered());
    pSizer->Add(m_pRemember, 0, wxALL, 10);

    auto* pButtons = new wxStdDialogButtonSizer();
    auto* pLogin = new wxButton(this, wxID_OK, "Login");
    pLogin->SetDefault();
    pButtons->AddButton(pLogin);
    pButtons->AddButton(new wxButton(this, wxID_CANCEL));
    pButtons->Realize();
    pSizer->Add(pButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizerAndFit(pSizer);

    Bind(wxEVT_BUTTON, &LoginDialog::OnLogin, this, wxID_OK);

    if (vmLogin.GetUsername().empty())
        m_pUsername->SetFocus();
    else
        m_pPassword->SetFocus();
}

void LoginDialog::OnLogin(wxCommandEvent&)
{
    m_vmLogin.SetUsername(FromWx(m_pUsername->GetValue()));
    m_vmLogin.SetPassword(FromWx(m_pPassword->GetValue()));
    m_vmLogin.SetPasswordRemembered(m_pRemember->GetValue());

    // like the Win32 dialog, a failed login (which shows a message box) keeps the dialog open
    if (m_vmLogin.Login())
        CloseWithResult(DialogResult::OK);
}

} // namespace wx
} // namespace ui
} // namespace ra
