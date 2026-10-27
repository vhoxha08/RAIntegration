#include "OverlaySettingsView.hh"

#include "ui/imgui/ImGuiControls.hh"

#include <imgui.h>

namespace ra {
namespace ui {
namespace imgui {

using ra::ui::viewmodels::OverlaySettingsViewModel;
using ra::ui::viewmodels::PopupLocation;

void OverlaySettingsView::RenderContents()
{
    const float fComboWidth = ImGui::GetFontSize() * 9.0f;

    auto LocationRow = [fComboWidth](const char* sLabel, const ra::ui::viewmodels::LookupItemViewModelCollection& vItems,
                                     PopupLocation nLocation, auto fSetLocation) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(sLabel);
        ImGui::TableNextColumn();

        int nId = ra::etoi(nLocation);
        ImGui::SetNextItemWidth(fComboWidth);
        ImGui::PushID(sLabel);
        if (LookupCombo("##location", vItems, nId))
            fSetLocation(ra::itoe<PopupLocation>(nId));
        ImGui::PopID();
    };

    auto CheckboxRow = [](const char* sLabel, bool bValue, auto fSetValue) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (ImGui::Checkbox(sLabel, &bValue))
            fSetValue(bValue);
    };

    auto& vm = m_vmSettings;
    if (ImGui::BeginTable("##settings", 2, ImGuiTableFlags_SizingFixedFit))
    {
        LocationRow("Achievement triggered notification", vm.PopupLocations(), vm.GetAchievementTriggerLocation(),
                    [&vm](PopupLocation n) { vm.SetAchievementTriggerLocation(n); });
        CheckboxRow("Capture achievement triggered screenshot", vm.ScreenshotAchievementTrigger(),
                    [&vm](bool b) { vm.SetScreenshotAchievementTrigger(b); });
        LocationRow("Game mastery notification", vm.PopupLocations(), vm.GetMasteryLocation(),
                    [&vm](PopupLocation n) { vm.SetMasteryLocation(n); });
        CheckboxRow("Capture game mastery screenshot", vm.ScreenshotMastery(),
                    [&vm](bool b) { vm.SetScreenshotMastery(b); });
        LocationRow("Leaderboard started notification", vm.PopupLocations(), vm.GetLeaderboardStartedLocation(),
                    [&vm](PopupLocation n) { vm.SetLeaderboardStartedLocation(n); });
        LocationRow("Leaderboard canceled notification", vm.PopupLocations(), vm.GetLeaderboardCanceledLocation(),
                    [&vm](PopupLocation n) { vm.SetLeaderboardCanceledLocation(n); });
        LocationRow("Active leaderboard time/score values", vm.PopupLocations(), vm.GetLeaderboardTrackerLocation(),
                    [&vm](PopupLocation n) { vm.SetLeaderboardTrackerLocation(n); });
        LocationRow("Scoreboard after submitting leaderboard entry", vm.PopupLocationsNoMiddle(),
                    vm.GetLeaderboardScoreboardLocation(),
                    [&vm](PopupLocation n) { vm.SetLeaderboardScoreboardLocation(n); });
        LocationRow("Active challenge indicator", vm.PopupLocationsNoMiddle(), vm.GetActiveChallengeLocation(),
                    [&vm](PopupLocation n) { vm.SetActiveChallengeLocation(n); });
        LocationRow("Progress indicator", vm.PopupLocationsNoMiddle(), vm.GetProgressTrackerLocation(),
                    [&vm](PopupLocation n) { vm.SetProgressTrackerLocation(n); });
        LocationRow("Informational notifications", vm.PopupLocations(), vm.GetMessageLocation(),
                    [&vm](PopupLocation n) { vm.SetMessageLocation(n); });
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Screenshot Location");
    std::string sLocation = ToUtf8(vm.ScreenshotLocation());
    ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() * 2.0f);
    ImGui::InputText("##location", sLocation.data(), sLocation.size() + 1, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("..."))
        RunAfterFrame([&vm]() { vm.BrowseLocation(); }); // shows a (modal) file dialog

    ImGui::Spacing();
    const float fButtonWidth = ImGui::GetFontSize() * 5.0f;
    AlignButtonsRight(2, fButtonWidth);
    if (ImGui::Button("OK", ImVec2(fButtonWidth, 0.0f)))
        Close(DialogResult::OK);
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(fButtonWidth, 0.0f)) || IsEscapePressed())
        Close(DialogResult::Cancel);
}

} // namespace imgui
} // namespace ui
} // namespace ra
