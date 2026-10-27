#include "MessageBoxView.hh"

#include <imgui.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace ra {
namespace ui {
namespace imgui {

using ra::ui::viewmodels::MessageBoxViewModel;

MessageBoxView::MessageBoxView(MessageBoxViewModel& vmMessageBox) noexcept :
    ImGuiWindowView(vmMessageBox),
    m_vmMessageBox(vmMessageBox)
{
}

void MessageBoxView::OnCloseRequested()
{
    // the title bar button does what Escape does in a Win32 message box
    switch (m_vmMessageBox.GetButtons())
    {
        case MessageBoxViewModel::Buttons::OK:
            Close(DialogResult::OK);
            break;

        case MessageBoxViewModel::Buttons::YesNo:
            Close(DialogResult::No);
            break;

        default:
            Close(DialogResult::Cancel);
            break;
    }
}

void MessageBoxView::RenderContents()
{
    const float fWrapWidth = ImGui::GetFontSize() * 28.0f;

    ImVec4 nHeaderColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    switch (m_vmMessageBox.GetIcon())
    {
        case MessageBoxViewModel::Icon::Warning: nHeaderColor = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); break;
        case MessageBoxViewModel::Icon::Error: nHeaderColor = ImVec4(1.0f, 0.45f, 0.4f, 1.0f); break;
        case MessageBoxViewModel::Icon::Info: nHeaderColor = ImVec4(0.55f, 0.75f, 1.0f, 1.0f); break;
        default: break;
    }

    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + fWrapWidth);

    const auto& sHeader = m_vmMessageBox.GetHeader();
    if (!sHeader.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, nHeaderColor);
        ImGui::TextUnformatted(ToUtf8(sHeader).c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    ImGui::TextUnformatted(ToUtf8(m_vmMessageBox.GetMessage()).c_str());
    ImGui::PopTextWrapPos();
    ImGui::Spacing();

    std::vector<std::pair<const char*, DialogResult>> vButtons;
    switch (m_vmMessageBox.GetButtons())
    {
        case MessageBoxViewModel::Buttons::OK:
            vButtons = {{"OK", DialogResult::OK}};
            break;
        case MessageBoxViewModel::Buttons::OKCancel:
            vButtons = {{"OK", DialogResult::OK}, {"Cancel", DialogResult::Cancel}};
            break;
        case MessageBoxViewModel::Buttons::YesNo:
            vButtons = {{"Yes", DialogResult::Yes}, {"No", DialogResult::No}};
            break;
        case MessageBoxViewModel::Buttons::YesNoCancel:
            vButtons = {{"Yes", DialogResult::Yes}, {"No", DialogResult::No}, {"Cancel", DialogResult::Cancel}};
            break;
        case MessageBoxViewModel::Buttons::RetryCancel:
            vButtons = {{"Retry", DialogResult::Retry}, {"Cancel", DialogResult::Cancel}};
            break;
    }

    // right-align the buttons
    const float fButtonWidth = ImGui::GetFontSize() * 6.0f;
    const float fSpacing = ImGui::GetStyle().ItemSpacing.x;
    const float fTotal = vButtons.size() * fButtonWidth + (vButtons.size() - 1) * fSpacing;
    const float fAvailable = std::max(fWrapWidth, ImGui::GetContentRegionAvail().x);
    if (fAvailable > fTotal)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + fAvailable - fTotal);

    for (size_t i = 0; i < vButtons.size(); ++i)
    {
        if (i > 0)
            ImGui::SameLine();

        if (i == 0 && ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();

        if (ImGui::Button(vButtons.at(i).first, ImVec2(fButtonWidth, 0.0f)))
            Close(vButtons.at(i).second);
    }

    // Enter picks the first button, like the default button of a Win32 message box
    if (!IsClosed() && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
        Close(vButtons.front().second);
    else if (!IsClosed() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        OnCloseRequested();
}

} // namespace imgui
} // namespace ui
} // namespace ra
