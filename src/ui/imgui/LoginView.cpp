#include "LoginView.hh"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace ra {
namespace ui {
namespace imgui {

LoginView::LoginView(ra::ui::viewmodels::LoginViewModel& vmLogin) :
    ImGuiWindowView(vmLogin),
    m_vmLogin(vmLogin),
    m_sUsername(ToUtf8(vmLogin.GetUsername())),
    m_bRemember(vmLogin.IsPasswordRemembered())
{
}

void LoginView::RenderContents()
{
    const float fFieldWidth = ImGui::GetFontSize() * 18.0f;

    ImGui::TextUnformatted("Log in to RetroAchievements");
    ImGui::Spacing();

    ImGui::BeginDisabled(m_bLoggingIn);

    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere(m_sUsername.empty() ? 0 : 1);

    ImGui::TextUnformatted("Username:");
    ImGui::SetNextItemWidth(fFieldWidth);
    bool bSubmit = ImGui::InputText("##username", &m_sUsername, ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::TextUnformatted("Password:");
    ImGui::SetNextItemWidth(fFieldWidth);
    bSubmit |= ImGui::InputText("##password", &m_sPassword,
                                ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::Checkbox("Remember password", &m_bRemember);

    ImGui::Spacing();
    bSubmit |= ImGui::Button(m_bLoggingIn ? "Logging in..." : "Login", ImVec2(fFieldWidth * 0.5f - 4.0f, 0.0f));
    ImGui::SameLine();
    const bool bCancel = ImGui::Button("Cancel", ImVec2(fFieldWidth * 0.5f - 4.0f, 0.0f)) ||
                         ImGui::IsKeyPressed(ImGuiKey_Escape);

    ImGui::EndDisabled();

    if (bCancel && !m_bLoggingIn)
    {
        Close(DialogResult::Cancel);
    }
    else if (bSubmit && !m_bLoggingIn)
    {
        m_vmLogin.SetUsername(FromUtf8(m_sUsername));
        m_vmLogin.SetPassword(FromUtf8(m_sPassword));
        m_vmLogin.SetPasswordRemembered(m_bRemember);

        // Login() blocks on the server request and may show message boxes, so it can't run inside the frame.
        // Like the Win32 dialog, a failed login keeps the dialog open.
        m_bLoggingIn = true;
        RunAfterFrame([this]() {
            const bool bSuccess = m_vmLogin.Login();
            m_bLoggingIn = false;
            if (bSuccess)
                Close(DialogResult::OK);
        });
    }
}

} // namespace imgui
} // namespace ui
} // namespace ra
