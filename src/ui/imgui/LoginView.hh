#ifndef RA_UI_IMGUI_LOGIN_VIEW_HH
#define RA_UI_IMGUI_LOGIN_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/LoginViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 LoginDialog.
/// </summary>
class LoginView : public ImGuiWindowView
{
public:
    explicit LoginView(ra::ui::viewmodels::LoginViewModel& vmLogin);

    void RenderContents() override;

    // the queued login action refers to this view; don't close while it is pending
    void OnCloseRequested() override
    {
        if (!m_bLoggingIn)
            Close(DialogResult::Cancel);
    }

private:
    ra::ui::viewmodels::LoginViewModel& m_vmLogin;
    std::string m_sUsername;
    std::string m_sPassword;
    bool m_bRemember = false;
    bool m_bLoggingIn = false;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_LOGIN_VIEW_HH
