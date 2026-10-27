#include "UnknownGameDialog.hh"

#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

UnknownGameDialog::UnknownGameDialog(ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame) :
    WxDialog(vmUnknownGame),
    m_vmUnknownGame(vmUnknownGame)
{
    auto& vm = vmUnknownGame;
    Watch(vm.GameTitles());

    constexpr int nWidth = 420;
    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    m_pHeader = new wxStaticText(this, wxID_ANY, wxEmptyString);
    pSizer->Add(m_pHeader, 0, wxALL, 10);

    auto* pInfoRow = new wxBoxSizer(wxHORIZONTAL);
    auto* pInfo = new wxFlexGridSizer(2, wxSize(10, 2));
    auto AddInfo = [this, pInfo](const char* sLabel) {
        pInfo->Add(new wxStaticText(this, wxID_ANY, sLabel));
        auto* pValue = new wxStaticText(this, wxID_ANY, wxEmptyString);
        pInfo->Add(pValue);
        return pValue;
    };
    m_pName = AddInfo("Name:");
    m_pSystem = AddInfo("System:");
    m_pHash = AddInfo("RA Hash:");
    pInfoRow->Add(pInfo, 1);
    auto* pCopy = new wxButton(this, wxID_ANY, "Copy Hash");
    pCopy->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.CopyChecksumToClipboard(); });
    pInfoRow->Add(pCopy, 0, wxALIGN_BOTTOM);
    pSizer->Add(pInfoRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 14);

    pSizer->Add(new wxStaticText(this, wxID_ANY,
                                 "Select an existing title for the game from the following list of known titles:"),
                0, wxLEFT | wxRIGHT | wxTOP, 10);
    m_pGames = new LookupChoice(this, [&vm](int nId) { vm.SetSelectedGameId(nId); });
    m_pGames->SetMinSize(wxSize(nWidth, -1));
    pSizer->Add(m_pGames, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto* pFilterRow = new wxBoxSizer(wxHORIZONTAL);
    pFilterRow->Add(new wxStaticText(this, wxID_ANY, "Filter:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    m_pFilter = new wxTextCtrl(this, wxID_ANY);
    m_pFilter->Bind(wxEVT_TEXT, [this, &vm](wxCommandEvent&) { vm.SetFilterText(FromWx(m_pFilter->GetValue())); });
    pFilterRow->Add(m_pFilter, 1, wxALIGN_CENTER_VERTICAL);
    m_pResults = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(110, -1),
                                  wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    pFilterRow->Add(m_pResults, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    pSizer->Add(pFilterRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    pSizer->Add(new wxStaticText(this, wxID_ANY, "Or provide an appropriate name to create a new entry:"), 0,
                wxLEFT | wxRIGHT | wxTOP, 10);
    m_pNewName = new wxTextCtrl(this, wxID_ANY);
    m_pNewName->Bind(wxEVT_TEXT,
                     [this, &vm](wxCommandEvent&) { vm.SetNewGameName(FromWx(m_pNewName->GetValue())); });
    pSizer->Add(m_pNewName, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    auto* pHint = new wxStaticText(this, wxID_ANY,
                                   "Then press 'Link' to associate the hash to the game, or 'Test' to load the game "
                                   "without registering the hash.");
    pHint->Wrap(nWidth);
    pSizer->Add(pHint, 0, wxALL, 10);

    // Associate and BeginTest set the dialog result (closing the window) when they succeed
    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    m_pLink = new wxButton(this, wxID_ANY, "Link");
    m_pLink->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.Associate(); });
    pButtons->Add(m_pLink);
    pButtons->AddStretchSpacer();
    m_pTest = new wxButton(this, wxID_OK, "Test");
    m_pTest->SetDefault();
    m_pTest->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.BeginTest(); });
    pButtons->Add(m_pTest, 0, wxRIGHT, 6);
    pButtons->Add(new wxButton(this, wxID_CANCEL));
    pSizer->Add(pButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizerAndFit(pSizer);
    UpdateControls();
}

void UnknownGameDialog::UpdateControls()
{
    auto& vm = m_vmUnknownGame;
    SetLabelIfChanged(*m_pHeader, vm.GetProblemHeader());
    SetLabelIfChanged(*m_pName, vm.GetEstimatedGameName());
    SetLabelIfChanged(*m_pSystem, vm.GetSystemName());
    SetLabelIfChanged(*m_pHash, vm.GetChecksum());
    SetLabelIfChanged(*m_pResults, L"Results: " + vm.GetFilterResults());

    m_pGames->Update(vm.GameTitles(), vm.GetSelectedGameId());
    SetTextIfChanged(*m_pFilter, vm.GetFilterText());
    SetTextIfChanged(*m_pNewName, vm.GetNewGameName());

    const bool bEnabled = vm.IsSelectedGameEnabled();
    m_pGames->Enable(bEnabled);
    m_pFilter->Enable(bEnabled);
    m_pNewName->Enable(bEnabled);
    m_pTest->Enable(bEnabled);
    m_pLink->Enable(vm.IsAssociateEnabled());
}

} // namespace wx
} // namespace ui
} // namespace ra
