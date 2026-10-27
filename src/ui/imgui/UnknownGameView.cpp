#include "UnknownGameView.hh"

#include "ui/imgui/ImGuiControls.hh"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace ra {
namespace ui {
namespace imgui {

void UnknownGameView::RenderContents()
{
    auto& vm = m_vmUnknownGame;
    const float fWidth = ImGui::GetFontSize() * 26.0f;
    const float fButtonWidth = ImGui::GetFontSize() * 6.0f;

    ImGui::TextUnformatted(ToUtf8(vm.GetProblemHeader()).c_str());
    ImGui::Spacing();

    if (ImGui::BeginTable("##info", 2, ImGuiTableFlags_SizingFixedFit))
    {
        auto Row = [](const char* sLabel, const std::wstring& sValue) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(sLabel);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(ToUtf8(sValue).c_str());
        };
        Row("Name:", vm.GetEstimatedGameName());
        Row("System:", vm.GetSystemName());
        Row("RA Hash:", vm.GetChecksum());
        ImGui::EndTable();
    }
    ImGui::SameLine();
    AlignButtonsRight(1, fButtonWidth);
    if (ImGui::Button("Copy Hash", ImVec2(fButtonWidth, 0.0f)))
        vm.CopyChecksumToClipboard();

    ImGui::Spacing();
    ImGui::BeginDisabled(m_bBusy);

    ImGui::BeginDisabled(!vm.IsSelectedGameEnabled());
    ImGui::TextUnformatted("Select an existing title for the game from the following list of known titles:");
    int nSelectedId = vm.GetSelectedGameId();
    ImGui::SetNextItemWidth(fWidth);
    if (LookupCombo("##games", vm.GameTitles(), nSelectedId))
        vm.SetSelectedGameId(nSelectedId);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Filter:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(fWidth * 0.6f);
    if (ImGui::InputText("##filter", &m_sFilter))
        vm.SetFilterText(FromUtf8(m_sFilter));
    ImGui::SameLine();
    ImGui::Text("Results: %s", ToUtf8(vm.GetFilterResults()).c_str());

    ImGui::Spacing();
    ImGui::TextUnformatted("Or provide an appropriate name to create a new entry:");
    ImGui::SetNextItemWidth(fWidth);
    if (ImGui::InputText("##newname", &m_sNewName))
        vm.SetNewGameName(FromUtf8(m_sNewName));
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::PushTextWrapPos(fWidth);
    ImGui::TextUnformatted("Then press 'Link' to associate the hash to the game, or 'Test' to load the game "
                           "without registering the hash.");
    ImGui::PopTextWrapPos();
    ImGui::Spacing();

    // Associate and BeginTest talk to the server and may show message boxes, so they run after the frame.
    // They set the dialog result (closing the window) when they succeed.
    ImGui::BeginDisabled(!vm.IsAssociateEnabled());
    if (ImGui::Button("Link", ImVec2(fButtonWidth, 0.0f)))
    {
        m_bBusy = true;
        RunAfterFrame([this]() {
            m_vmUnknownGame.Associate();
            m_bBusy = false;
        });
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    AlignButtonsRight(2, fButtonWidth);
    ImGui::BeginDisabled(!vm.IsSelectedGameEnabled());
    if (ImGui::Button("Test", ImVec2(fButtonWidth, 0.0f)))
    {
        m_bBusy = true;
        RunAfterFrame([this]() {
            m_vmUnknownGame.BeginTest();
            m_bBusy = false;
        });
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(fButtonWidth, 0.0f)) || IsEscapePressed())
        Close(DialogResult::Cancel);

    ImGui::EndDisabled();
}

} // namespace imgui
} // namespace ui
} // namespace ra
