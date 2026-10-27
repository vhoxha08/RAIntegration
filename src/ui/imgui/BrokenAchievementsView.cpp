#include "BrokenAchievementsView.hh"

#include "ui/imgui/ImGuiControls.hh"

#include <imgui.h>

namespace ra {
namespace ui {
namespace imgui {

void BrokenAchievementsView::RenderContents()
{
    auto& vm = m_vmBrokenAchievements;
    const float fFontSize = ImGui::GetFontSize();

    ImGui::TextUnformatted("Which achievement would you like to report?");

    const ImGuiTableFlags nFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                   ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##achievements", 4, nFlags, ImVec2(fFontSize * 40.0f, fFontSize * 20.0f)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
        ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 4.0f);
        ImGui::TableSetupColumn("Achieved", ImGuiTableColumnFlags_WidthFixed, fFontSize * 4.0f);
        ImGui::TableHeadersRow();

        auto& vAchievements = vm.Achievements();
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vAchievements.Count()); ++i)
        {
            auto* pItem = vAchievements.GetItemAt(i);
            if (pItem == nullptr)
                continue;

            ImGui::PushID(gsl::narrow_cast<int>(i));
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            bool bSelected = pItem->IsSelected();
            if (ImGui::Checkbox("##selected", &bSelected))
                pItem->SetSelected(bSelected); // the view model unselects the previous item

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(ToUtf8(pItem->GetLabel()).c_str());
            ImGui::TableNextColumn();
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextUnformatted(ToUtf8(pItem->GetDescription()).c_str());
            ImGui::PopTextWrapPos();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(pItem->IsAchieved() ? "Yes" : "No");
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    const float fButtonWidth = fFontSize * 7.0f;
    AlignButtonsRight(2, fButtonWidth);
    if (ImGui::Button("Report Problem", ImVec2(fButtonWidth, 0.0f)))
    {
        // Submit opens the report page in the browser, or shows an error message box
        RunAfterFrame([&vm]() {
            if (vm.Submit())
                vm.SetDialogResult(DialogResult::OK);
        });
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(fButtonWidth, 0.0f)) || IsEscapePressed())
        Close(DialogResult::Cancel);
}

} // namespace imgui
} // namespace ui
} // namespace ra
