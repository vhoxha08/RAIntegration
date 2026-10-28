#include "SmallDialogs.hh"

#include "services/ServiceLocator.hh"

#include "ui/viewmodels/WindowManager.hh"
#include "ui/wx/WxApp.hh"
#include "ui/wx/WxGrid.hh"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::MemoryNotesViewModel;
using ra::ui::viewmodels::MemoryRegionsViewModel;
using ra::ui::viewmodels::TriggerSummaryViewModel;

// ---- Memory Notes ----------------------------------------------------------

MemoryNotesDialog::MemoryNotesDialog(MemoryNotesViewModel& vmMemoryNotes) :
    WxDialog(vmMemoryNotes, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX)
{
    auto& vm = vmMemoryNotes;
    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    auto* pFilter = new wxFlexGridSizer(3, wxSize(6, 4));
    pFilter->AddGrowableCol(1);
    pFilter->Add(new wxStaticText(this, wxID_ANY, "Search:"), 0, wxALIGN_CENTER_VERTICAL);
    auto* pSearch = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_pBinder.BindText(pSearch, vm, MemoryNotesViewModel::FilterValueProperty);
    pSearch->Bind(wxEVT_TEXT_ENTER, [&vm](wxCommandEvent&) { vm.ApplyFilter(); });
    pSearch->Bind(wxEVT_KEY_DOWN, [&vm](wxKeyEvent& event) {
        if (event.GetKeyCode() != WXK_ESCAPE)
        {
            event.Skip();
            return;
        }
        vm.SetFilterValue(L"");
        vm.ResetFilter();
    });
    pFilter->Add(pSearch, 1, wxEXPAND);
    auto* pApply = new wxButton(this, wxID_ANY, "Filter");
    pApply->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.ApplyFilter(); });
    pFilter->Add(pApply, 0, wxEXPAND);

    auto* pResultsRow = new wxBoxSizer(wxHORIZONTAL);
    pResultsRow->Add(new wxStaticText(this, wxID_ANY, "Results:"), 0, wxRIGHT, 4);
    auto* pCount = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_pBinder.BindLabel(pCount, vm, MemoryNotesViewModel::ResultCountProperty);
    pResultsRow->Add(pCount, 1);
    pFilter->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
    auto* pMiddle = new wxBoxSizer(wxHORIZONTAL);
    pMiddle->Add(pResultsRow, 1, wxALIGN_CENTER_VERTICAL);
    auto* pUnpublished = new wxCheckBox(this, wxID_ANY, "Only Unpublished");
    m_pBinder.BindCheck(pUnpublished, vm, MemoryNotesViewModel::OnlyUnpublishedFilterProperty);
    pMiddle->Add(pUnpublished, 0, wxALIGN_CENTER_VERTICAL);
    pFilter->Add(pMiddle, 1, wxEXPAND);
    auto* pReset = new wxButton(this, wxID_ANY, "Reset");
    pReset->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.ResetFilter(); });
    pFilter->Add(pReset, 0, wxEXPAND);
    pSizer->Add(pFilter, 0, wxEXPAND | wxALL, 8);

    auto* pNotes = new Grid(this, wxDV_SINGLE | wxDV_ROW_LINES);
    auto pAddress = GridColumn::Text(L"Address", MemoryNotesViewModel::MemoryNoteViewModel::LabelProperty, 100);
    pAddress.pTextColorProperty = &MemoryNotesViewModel::MemoryNoteViewModel::BookmarkColorProperty;
    pNotes->AddColumn(std::move(pAddress));
    auto pNote = GridColumn::Text(L"Note", MemoryNotesViewModel::MemoryNoteViewModel::NoteProperty, 360);
    pNote.pTextColorProperty = &MemoryNotesViewModel::MemoryNoteViewModel::BookmarkColorProperty;
    pNotes->AddColumn(std::move(pNote));
    pNotes->BindIsSelected(MemoryNotesViewModel::MemoryNoteViewModel::IsSelectedProperty);
    pNotes->BindItems(vm.Notes());
    pNotes->SetDoubleClickHandler([&vm](gsl::index nIndex) {
        // like the Win32 dialog: double clicking a note shows it in the memory inspector
        const auto* pItem = vm.Notes().GetItemAt(nIndex);
        if (pItem)
        {
            auto& pMemoryInspector =
                ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector;
            pMemoryInspector.SetCurrentAddress(pItem->nAddress);
            if (!pMemoryInspector.IsVisible())
                pMemoryInspector.Show();
        }
    });
    pNotes->SetMinSize(FromDIP(wxSize(480, 320)));
    pSizer->Add(pNotes, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    auto* pBookmark = new wxButton(this, wxID_ANY, "Bookmark Selected");
    pBookmark->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.BookmarkSelected(); });
    pButtons->Add(pBookmark);
    pButtons->AddStretchSpacer();
    auto* pPublish = new wxButton(this, wxID_ANY, "Publish");
    pPublish->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.PublishSelected(); });
    m_pBinder.BindEnabled(pPublish, vm, MemoryNotesViewModel::CanPublishCurrentAddressNoteProperty);
    pButtons->Add(pPublish, 0, wxRIGHT, 4);
    auto* pRevert = new wxButton(this, wxID_ANY, "Revert");
    pRevert->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.RevertSelected(); });
    m_pBinder.BindEnabled(pRevert, vm, MemoryNotesViewModel::CanRevertCurrentAddressNoteProperty);
    pButtons->Add(pRevert);
    pSizer->Add(pButtons, 0, wxEXPAND | wxALL, 8);

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Code Notes");
    UpdateControls();
}

// ---- Memory Regions --------------------------------------------------------

MemoryRegionsDialog::MemoryRegionsDialog(MemoryRegionsViewModel& vmMemoryRegions) :
    WxDialog(vmMemoryRegions, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto& vm = vmMemoryRegions;
    using Region = MemoryRegionsViewModel::MemoryRegionViewModel;
    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    // only custom regions can be edited; invalid ranges get a warning sign
    auto IsCustom = [](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        return vItems.GetItemValue(nIndex, Region::IsCustomProperty);
    };
    auto* pRegions = new Grid(this, wxDV_SINGLE | wxDV_ROW_LINES);
    auto pLabel = GridColumn::Text(L"Address", Region::LabelProperty, 200, true);
    pLabel.fIsEditable = IsCustom;
    pRegions->AddColumn(std::move(pLabel));
    auto pRange = GridColumn::Text(L"Range", Region::RangeProperty, 200, true);
    pRange.fIsEditable = IsCustom;
    pRange.fGetText = [](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        std::wstring sRange = vItems.GetItemValue(nIndex, Region::RangeProperty);
        if (vItems.GetItemValue(nIndex, Region::IsInvalidProperty))
            sRange.append(L"\U000026A0"); // Warning Sign
        return sRange;
    };
    pRegions->AddColumn(std::move(pRange));
    pRegions->BindIsSelected(Region::IsSelectedProperty);
    pRegions->BindItems(vm.Regions());
    pRegions->SetPrimaryEditColumn(0); // label
    pRegions->SetMinSize(FromDIP(wxSize(440, 300)));
    pSizer->Add(pRegions, 1, wxEXPAND | wxALL, 8);

    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    auto* pNew = new wxButton(this, wxID_ANY, "New");
    pNew->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.AddNewRegion(); });
    pButtons->Add(pNew, 0, wxRIGHT, 4);
    auto* pDelete = new wxButton(this, wxID_ANY, "Delete");
    pDelete->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.RemoveRegion(); });
    m_pBinder.BindEnabled(pDelete, vm, MemoryRegionsViewModel::CanRemoveRegionProperty);
    pButtons->Add(pDelete);
    pButtons->AddStretchSpacer();
    auto* pOK = new wxButton(this, wxID_OK);
    pOK->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CloseWithResult(DialogResult::OK); }); // saves the regions
    m_pBinder.BindEnabled(pOK, vm, MemoryRegionsViewModel::CanSaveProperty);
    pButtons->Add(pOK, 0, wxRIGHT, 4);
    pButtons->Add(new wxButton(this, wxID_CANCEL));
    pSizer->Add(pButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Memory Regions");
    UpdateControls();
}

// ---- New Asset -------------------------------------------------------------

NewAssetDialog::NewAssetDialog(ra::ui::viewmodels::NewAssetViewModel& vmNewAsset) : WxDialog(vmNewAsset)
{
    SetTitle("New Asset");

    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    pSizer->Add(new wxStaticText(this, wxID_ANY, "What type of asset would you like to create?"), 0, wxALL, 10);

    // the view model sets the dialog result, which closes the dialog
    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    auto* pAchievement = new wxButton(this, wxID_ANY, "Achievement");
    pAchievement->SetDefault();
    pAchievement->Bind(wxEVT_BUTTON, [&vmNewAsset](wxCommandEvent&) { vmNewAsset.NewAchievement(); });
    pButtons->Add(pAchievement, 0, wxRIGHT, 4);
    auto* pLeaderboard = new wxButton(this, wxID_ANY, "Leaderboard");
    pLeaderboard->Bind(wxEVT_BUTTON, [&vmNewAsset](wxCommandEvent&) { vmNewAsset.NewLeaderboard(); });
    pButtons->Add(pLeaderboard);
    pButtons->AddStretchSpacer();
    pButtons->Add(new wxButton(this, wxID_CANCEL), 0, wxLEFT, 16);
    pSizer->Add(pButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizerAndFit(pSizer);
}

// ---- Progress --------------------------------------------------------------

ProgressDialog::ProgressDialog(ra::ui::viewmodels::ProgressViewModel& vmProgress) :
    WxDialog(vmProgress),
    m_vmProgress(vmProgress)
{
    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    auto* pMessage = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(320, -1)),
                                      wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END);
    m_pBinder.BindLabel(pMessage, vmProgress, ra::ui::viewmodels::ProgressViewModel::MessageProperty);
    pSizer->Add(pMessage, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    m_pGauge = new wxGauge(this, wxID_ANY, 100);
    pSizer->Add(m_pGauge, 0, wxEXPAND | wxALL, 10);

    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    pButtons->AddStretchSpacer();
    pButtons->Add(new wxButton(this, wxID_CANCEL)); // closes with DialogResult::Cancel
    pSizer->Add(pButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizerAndFit(pSizer);
    UpdateControls();
}

void ProgressDialog::UpdateControls()
{
    m_pBinder.Update();
    m_pGauge->SetValue(std::clamp(m_vmProgress.GetProgress(), 0, 100));
}

// ---- Trigger Summary -------------------------------------------------------

TriggerSummaryDialog::TriggerSummaryDialog(TriggerSummaryViewModel& vmTriggerSummary) :
    WxDialog(vmTriggerSummary, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    using Clause = TriggerSummaryViewModel::TriggerClauseViewModel;
    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    auto* pClauses = new Grid(this, wxDV_SINGLE | wxDV_ROW_LINES);
    auto pIndices = GridColumn::Text(L"Indices", Clause::IndicesProperty, 80);
    pIndices.nAlignment = wxALIGN_RIGHT;
    pClauses->AddColumn(std::move(pIndices));
    auto pReference = GridColumn::Text(L"Reference", Clause::ReferenceProperty, 240);
    pReference.fGetTooltip = GridColumn::TooltipFrom(Clause::ReferenceProperty);
    pClauses->AddColumn(std::move(pReference));
    pClauses->AddColumn(GridColumn::Text(L"Operation", Clause::OperationProperty, 140));
    auto pTarget = GridColumn::Text(L"Target", Clause::TargetProperty, 240);
    pTarget.fGetTooltip = GridColumn::TooltipFrom(Clause::TargetProperty);
    pClauses->AddColumn(std::move(pTarget));
    pClauses->AddColumn(GridColumn::Text(L"Tally", Clause::TallyProperty, 60));
    pClauses->BindRowColor(Clause::ColorProperty);
    pClauses->BindItems(vmTriggerSummary.Clauses());
    pClauses->SetMinSize(FromDIP(wxSize(780, 360)));
    pSizer->Add(pClauses, 1, wxEXPAND | wxALL, 8);

    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    pButtons->AddStretchSpacer();
    auto* pOK = new wxButton(this, wxID_OK);
    pOK->SetDefault();
    pOK->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CloseWithResult(DialogResult::OK); });
    pButtons->Add(pOK);
    pSizer->Add(pButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Trigger Summary");
}

} // namespace wx
} // namespace ui
} // namespace ra
