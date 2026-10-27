#include "GameChecksumView.hh"

#include "ui/imgui/ImGuiControls.hh"

#include <imgui.h>

namespace ra {
namespace ui {
namespace imgui {

void GameChecksumView::RenderContents()
{
    ImGui::TextUnformatted("RetroAchievements Hash:");
    ImGui::TextUnformatted(ToUtf8(m_vmChecksum.GetChecksum()).c_str());
    ImGui::Spacing();

    if (ImGui::Button("Copy To Clipboard"))
        m_vmChecksum.CopyChecksumToClipboard();

    const float fButtonWidth = ImGui::GetFontSize() * 5.0f;
    ImGui::SameLine(0.0f, ImGui::GetFontSize() * 4.0f);
    AlignButtonsRight(1, fButtonWidth);
    if (ImGui::Button("OK", ImVec2(fButtonWidth, 0.0f)) || IsEscapePressed())
        Close(DialogResult::OK);
}

} // namespace imgui
} // namespace ui
} // namespace ra
