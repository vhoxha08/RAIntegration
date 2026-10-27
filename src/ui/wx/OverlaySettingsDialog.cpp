#include "OverlaySettingsDialog.hh"

#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::PopupLocation;

OverlaySettingsDialog::OverlaySettingsDialog(ra::ui::viewmodels::OverlaySettingsViewModel& vmSettings) :
    WxDialog(vmSettings),
    m_vmSettings(vmSettings)
{
    auto& vm = vmSettings;
    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    auto* pGrid = new wxFlexGridSizer(2, wxSize(12, 6));
    pGrid->AddGrowableCol(0);

    auto AddLocation = [this, pGrid](const char* sLabel, const ra::ui::viewmodels::LookupItemViewModelCollection& vItems,
                                     std::function<PopupLocation()> fGet, std::function<void(PopupLocation)> fSet) {
        pGrid->Add(new wxStaticText(this, wxID_ANY, sLabel), 0, wxALIGN_CENTER_VERTICAL);
        auto* pChoice = new LookupChoice(this, [fSet](int nId) { fSet(ra::itoe<PopupLocation>(nId)); });
        pGrid->Add(pChoice, 0, wxEXPAND);
        m_vLocations.push_back({pChoice, &vItems, std::move(fGet)});
    };
    auto AddCheckBox = [this, pGrid](const char* sLabel, std::function<void(bool)> fSet) {
        auto* pCheckBox = new wxCheckBox(this, wxID_ANY, sLabel);
        pCheckBox->Bind(wxEVT_CHECKBOX, [fSet](wxCommandEvent& event) { fSet(event.IsChecked()); });
        pGrid->Add(pCheckBox, 0, wxLEFT, 12);
        pGrid->AddSpacer(0);
        return pCheckBox;
    };

    AddLocation("Achievement triggered notification", vm.PopupLocations(),
                [&vm]() { return vm.GetAchievementTriggerLocation(); },
                [&vm](PopupLocation n) { vm.SetAchievementTriggerLocation(n); });
    m_pScreenshotTrigger = AddCheckBox("Capture achievement triggered screenshot",
                                       [&vm](bool b) { vm.SetScreenshotAchievementTrigger(b); });
    AddLocation("Game mastery notification", vm.PopupLocations(), [&vm]() { return vm.GetMasteryLocation(); },
                [&vm](PopupLocation n) { vm.SetMasteryLocation(n); });
    m_pScreenshotMastery = AddCheckBox("Capture game mastery screenshot",
                                       [&vm](bool b) { vm.SetScreenshotMastery(b); });
    AddLocation("Leaderboard started notification", vm.PopupLocations(),
                [&vm]() { return vm.GetLeaderboardStartedLocation(); },
                [&vm](PopupLocation n) { vm.SetLeaderboardStartedLocation(n); });
    AddLocation("Leaderboard canceled notification", vm.PopupLocations(),
                [&vm]() { return vm.GetLeaderboardCanceledLocation(); },
                [&vm](PopupLocation n) { vm.SetLeaderboardCanceledLocation(n); });
    AddLocation("Active leaderboard time/score values", vm.PopupLocations(),
                [&vm]() { return vm.GetLeaderboardTrackerLocation(); },
                [&vm](PopupLocation n) { vm.SetLeaderboardTrackerLocation(n); });
    AddLocation("Scoreboard after submitting leaderboard entry", vm.PopupLocationsNoMiddle(),
                [&vm]() { return vm.GetLeaderboardScoreboardLocation(); },
                [&vm](PopupLocation n) { vm.SetLeaderboardScoreboardLocation(n); });
    AddLocation("Active challenge indicator", vm.PopupLocationsNoMiddle(),
                [&vm]() { return vm.GetActiveChallengeLocation(); },
                [&vm](PopupLocation n) { vm.SetActiveChallengeLocation(n); });
    AddLocation("Progress indicator", vm.PopupLocationsNoMiddle(), [&vm]() { return vm.GetProgressTrackerLocation(); },
                [&vm](PopupLocation n) { vm.SetProgressTrackerLocation(n); });
    AddLocation("Informational notifications", vm.PopupLocations(), [&vm]() { return vm.GetMessageLocation(); },
                [&vm](PopupLocation n) { vm.SetMessageLocation(n); });
    pSizer->Add(pGrid, 0, wxEXPAND | wxALL, 10);

    pSizer->Add(new wxStaticText(this, wxID_ANY, "Screenshot Location"), 0, wxLEFT | wxRIGHT, 10);
    auto* pLocationRow = new wxBoxSizer(wxHORIZONTAL);
    m_pScreenshotLocation = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                           wxTE_READONLY);
    pLocationRow->Add(m_pScreenshotLocation, 1, wxALIGN_CENTER_VERTICAL);
    auto* pBrowse = new wxButton(this, wxID_ANY, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    pBrowse->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.BrowseLocation(); });
    pLocationRow->Add(pBrowse, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 4);
    pSizer->Add(pLocationRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

    auto* pButtons = new wxStdDialogButtonSizer();
    auto* pOK = new wxButton(this, wxID_OK);
    pOK->SetDefault();
    pOK->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CloseWithResult(DialogResult::OK); });
    pButtons->AddButton(pOK);
    pButtons->AddButton(new wxButton(this, wxID_CANCEL));
    pButtons->Realize();
    pSizer->Add(pButtons, 0, wxEXPAND | wxALL, 10);

    SetSizerAndFit(pSizer);
    UpdateControls();
}

void OverlaySettingsDialog::UpdateControls()
{
    for (const auto& pRow : m_vLocations)
        pRow.pChoice->Update(*pRow.pItems, ra::etoi(pRow.fGet()));

    m_pScreenshotTrigger->SetValue(m_vmSettings.ScreenshotAchievementTrigger());
    m_pScreenshotMastery->SetValue(m_vmSettings.ScreenshotMastery());
    SetTextIfChanged(*m_pScreenshotLocation, m_vmSettings.ScreenshotLocation());
}

} // namespace wx
} // namespace ui
} // namespace ra
