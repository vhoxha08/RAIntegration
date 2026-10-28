#include "MemoryInspectorDialog.hh"

#include "ui/wx/MemoryViewerControl.hh"
#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dataview.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::MemoryInspectorViewModel;
using ra::ui::viewmodels::MemorySearchViewModel;

// MemorySearchViewModel keeps SEARCH_ROWS_DISPLAYED (9) rows starting at ScrollOffset; the list shows one less
constexpr int VISIBLE_RESULT_ROWS = 8;

// ---- results model ---------------------------------------------------------

/// <summary>
/// Virtual list of the search results. Like the Win32 list, the view model only holds the rows that are visible
/// (starting at ScrollOffset); rows outside of that window are shown from the raw results.
/// </summary>
class MemoryInspectorDialog::ResultsModel : public wxDataViewVirtualListModel
{
public:
    explicit ResultsModel(MemorySearchViewModel& vmSearch) : m_vmSearch(vmSearch) {}

    unsigned int GetColumnCount() const override { return 3; }
    wxString GetColumnType(unsigned int) const override { return "string"; }

    const MemorySearchViewModel::SearchResultViewModel* GetVisibleRow(unsigned int nRow) const
    {
        const auto nOffset = m_vmSearch.GetScrollOffset();
        if (nRow < nOffset)
            return nullptr;

        return m_vmSearch.Results().GetItemAt(gsl::narrow_cast<gsl::index>(nRow - nOffset));
    }

    void GetValueByRow(wxVariant& vValue, unsigned int nRow, unsigned int nColumn) const override
    {
        if (const auto* pRow = GetVisibleRow(nRow))
        {
            switch (nColumn)
            {
                case 0: vValue = ToWx(pRow->GetAddress()); return;
                case 1: vValue = ToWx(pRow->GetCurrentValue()); return;
                default: vValue = ToWx(pRow->GetDescription()); return;
            }
        }

        // not loaded yet (scrolling); the next update fills it in
        vValue = wxString();
    }

    bool GetAttrByRow(unsigned int nRow, unsigned int nColumn, wxDataViewItemAttr& pAttr) const override
    {
        const auto* pRow = GetVisibleRow(nRow);
        if (pRow == nullptr)
            return false;

        // the toolkit's colors are meant for light lists; keep the text readable in dark mode too
        wxColour pBackground = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
        wxColour pText = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
        bool bChanged = false;
        const uint32_t nRowColor = pRow->GetRowColor().ARGB;
        if ((nRowColor >> 24) != 0)
        {
            pBackground = wxColour((nRowColor >> 16) & 0xFF, (nRowColor >> 8) & 0xFF, nRowColor & 0xFF);
            pAttr.SetBackgroundColour(pBackground);
            bChanged = true;
        }

        const uint32_t nTextColor = pRow->GetDescriptionColor().ARGB;
        if (nColumn == 2 && (nTextColor >> 24) != 0)
        {
            pText = wxColour((nTextColor >> 16) & 0xFF, (nTextColor >> 8) & 0xFF, nTextColor & 0xFF);
            bChanged = true;
        }

        if (bChanged)
            pAttr.SetColour(ReadableTextColour(pText, pBackground));

        return bChanged;
    }

    bool SetValueByRow(const wxVariant&, unsigned int, unsigned int) override { return false; }

private:
    MemorySearchViewModel& m_vmSearch;
};

// ---- dialog ----------------------------------------------------------------

MemoryInspectorDialog::MemoryInspectorDialog(MemoryInspectorViewModel& vmMemoryInspector) :
    WxDialog(vmMemoryInspector, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX),
    m_vmMemoryInspector(vmMemoryInspector)
{
    Watch(vmMemoryInspector.Search());
    Watch(vmMemoryInspector.Search().Results());
    Watch(vmMemoryInspector.Viewer());

    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    pSizer->Add(CreateSearchControls(), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    pSizer->Add(CreateResultsControls(), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    pSizer->Add(CreateNotesControls(), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    pSizer->Add(CreateViewerControls(), 1, wxEXPAND | wxALL, 8);
    SetSizer(pSizer);

    SetMinSize(FromDIP(wxSize(640, 600)));
    SetSize(FromDIP(wxSize(700, 760)));

    // the results list doesn't report scrolling; follow its position like the Win32 list's scroll events did
    m_pScrollTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { SyncScrollOffset(); });
    m_pScrollTimer.Start(100);

    RememberSizeAndPosition("Memory Inspector");
    UpdateControls();
}

MemoryInspectorDialog::~MemoryInspectorDialog()
{
    m_pScrollTimer.Stop();
}

wxSizer* MemoryInspectorDialog::CreateSearchControls()
{
    auto& vmSearch = m_vmMemoryInspector.Search();
    auto* pRow = new wxBoxSizer(wxHORIZONTAL);

    // New Search
    auto* pSearchBox = new wxStaticBoxSizer(wxVERTICAL, this, "New Search");
    auto* pSearchParent = pSearchBox->GetStaticBox();
    auto* pRangeRow = new wxBoxSizer(wxHORIZONTAL);
    m_pRanges = new LookupChoice(pSearchParent, [&vmSearch](int nId) { vmSearch.SetPredefinedFilterRange(nId); });
    m_pRanges->SetMinSize(FromDIP(wxSize(90, -1))); // the labels include the address ranges; don't size to them
    pRangeRow->Add(m_pRanges, 0, wxRIGHT, 4);
    m_pRange = new wxTextCtrl(pSearchParent, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(110, -1)));
    m_pRange->Bind(wxEVT_TEXT, [this, &vmSearch](wxCommandEvent&) {
        vmSearch.SetFilterRange(FromWx(m_pRange->GetValue()));
    });
    pRangeRow->Add(m_pRange, 1);
    pSearchBox->Add(pRangeRow, 0, wxEXPAND | wxALL, 4);

    auto* pTypeRow = new wxBoxSizer(wxHORIZONTAL);
    m_pSearchType = new LookupChoice(pSearchParent, [&vmSearch](int nId) {
        vmSearch.SetSearchType(ra::itoe<ra::services::SearchType>(nId));
    });
    pTypeRow->Add(m_pSearchType, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 4);
    m_pAligned = new wxCheckBox(pSearchParent, wxID_ANY, "Aligned");
    m_pAligned->Bind(wxEVT_CHECKBOX, [&vmSearch](wxCommandEvent& event) { vmSearch.SetAligned(event.IsChecked()); });
    pTypeRow->Add(m_pAligned, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 4);
    pTypeRow->AddStretchSpacer();
    m_pNewSearch = new wxButton(pSearchParent, wxID_ANY, "New Search");
    m_pNewSearch->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.BeginNewSearch(); });
    pTypeRow->Add(m_pNewSearch, 0, wxALIGN_CENTER_VERTICAL);
    pSearchBox->Add(pTypeRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);
    pRow->Add(pSearchBox, 0, wxEXPAND | wxRIGHT, 6);

    // Filter
    auto* pFilterBox = new wxStaticBoxSizer(wxVERTICAL, this, "Filter");
    auto* pFilterParent = pFilterBox->GetStaticBox();
    auto* pFilterGrid = new wxFlexGridSizer(2, wxSize(4, 4));
    pFilterGrid->AddGrowableCol(0);

    auto* pComparisonRow = new wxBoxSizer(wxHORIZONTAL);
    m_pComparison = new LookupChoice(pFilterParent, [&vmSearch](int nId) {
        vmSearch.SetComparisonType(ra::itoe<ComparisonType>(nId));
    });
    pComparisonRow->Add(m_pComparison, 0, wxRIGHT, 4);
    m_pValueType = new LookupChoice(pFilterParent, [&vmSearch](int nId) {
        vmSearch.SetValueType(ra::itoe<ra::services::SearchFilterType>(nId));
    });
    pComparisonRow->Add(m_pValueType, 1);
    pFilterGrid->Add(pComparisonRow, 1, wxEXPAND);
    m_pFilterOnce = new wxButton(pFilterParent, wxID_ANY, "Filter Once");
    m_pFilterOnce->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.ApplyFilter(); });
    pFilterGrid->Add(m_pFilterOnce, 0, wxEXPAND);

    m_pFilterValue = new wxTextCtrl(pFilterParent, wxID_ANY);
    m_pFilterValue->Bind(wxEVT_TEXT, [this, &vmSearch](wxCommandEvent&) {
        vmSearch.SetFilterValue(FromWx(m_pFilterValue->GetValue()));
    });
    pFilterGrid->Add(m_pFilterValue, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    m_pContinuousFilter = new wxButton(pFilterParent, wxID_ANY, "Continuous Filter");
    m_pContinuousFilter->SetMinSize(wxSize(GetTextExtent("Continuous Filter").GetWidth() + FromDIP(32), -1));
    m_pContinuousFilter->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.ToggleContinuousFilter(); });
    pFilterGrid->Add(m_pContinuousFilter, 0, wxEXPAND);
    pFilterBox->Add(pFilterGrid, 1, wxEXPAND | wxALL, 4);
    pRow->Add(pFilterBox, 1, wxEXPAND);

    return pRow;
}

wxSizer* MemoryInspectorDialog::CreateResultsControls()
{
    auto& vmSearch = m_vmMemoryInspector.Search();
    auto* pBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Results");
    auto* pParent = pBox->GetStaticBox();

    auto* pLeft = new wxBoxSizer(wxVERTICAL);
    auto* pInfo = new wxFlexGridSizer(2, wxSize(4, 2));
    pInfo->Add(new wxStaticText(pParent, wxID_ANY, "Count:"));
    m_pResultCount = new wxStaticText(pParent, wxID_ANY, "0");
    pInfo->Add(m_pResultCount);
    pInfo->Add(new wxStaticText(pParent, wxID_ANY, "Filter:"));
    m_pFilterSummary = new wxStaticText(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(110, -1)),
                                        wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END);
    pInfo->Add(m_pFilterSummary);
    pLeft->Add(pInfo, 0, wxBOTTOM, 4);

    auto* pPageRow = new wxBoxSizer(wxHORIZONTAL);
    m_pPreviousPage = new wxButton(pParent, wxID_ANY, "<<", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_pPreviousPage->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.PreviousPage(); });
    pPageRow->Add(m_pPreviousPage);
    m_pPage = new wxStaticText(pParent, wxID_ANY, "0/0", wxDefaultPosition, wxDefaultSize,
                               wxALIGN_CENTRE_HORIZONTAL | wxST_NO_AUTORESIZE);
    pPageRow->Add(m_pPage, 1, wxALIGN_CENTER_VERTICAL);
    m_pNextPage = new wxButton(pParent, wxID_ANY, ">>", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_pNextPage->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.NextPage(); });
    pPageRow->Add(m_pNextPage);
    pLeft->Add(pPageRow, 0, wxEXPAND);

    m_pExclude = new wxButton(pParent, wxID_ANY, "Exclude Selected");
    m_pExclude->Bind(wxEVT_BUTTON, [this, &vmSearch](wxCommandEvent&) {
        vmSearch.ExcludeSelected();
        m_bSyncingSelection = true;
        m_pResults->UnselectAll();
        m_bSyncingSelection = false;
    });
    pLeft->Add(m_pExclude, 0, wxEXPAND | wxTOP, 2);
    m_pBookmark = new wxButton(pParent, wxID_ANY, "Bookmark Selected");
    m_pBookmark->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.BookmarkSelected(); });
    pLeft->Add(m_pBookmark, 0, wxEXPAND | wxTOP, 2);

    auto* pFileRow = new wxBoxSizer(wxHORIZONTAL);
    m_pExport = new wxButton(pParent, wxID_ANY, "Export");
    m_pExport->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.ExportResults(); });
    pFileRow->Add(m_pExport, 1, wxRIGHT, 2);
    m_pImport = new wxButton(pParent, wxID_ANY, "Import");
    m_pImport->Bind(wxEVT_BUTTON, [&vmSearch](wxCommandEvent&) { vmSearch.ImportResults(); });
    pFileRow->Add(m_pImport, 1);
    pLeft->Add(pFileRow, 0, wxEXPAND | wxTOP, 2);
    pBox->Add(pLeft, 0, wxALL, 4);

    m_pResults = new wxDataViewCtrl(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxDV_MULTIPLE | wxDV_ROW_LINES | wxDV_NO_HEADER);
    m_pResultsModel = new ResultsModel(vmSearch);
    m_pResults->AssociateModel(m_pResultsModel);
    m_pResultsModel->DecRef(); // the control holds the reference
    m_pResults->AppendTextColumn("Address", 0, wxDATAVIEW_CELL_INERT, FromDIP(80));
    m_pResults->AppendTextColumn("Value", 1, wxDATAVIEW_CELL_INERT, FromDIP(64));
    m_pResults->AppendTextColumn("Description", 2, wxDATAVIEW_CELL_INERT, FromDIP(240));
    m_pResults->SetMinSize(wxSize(-1, m_pResults->GetCharHeight() * VISIBLE_RESULT_ROWS * 3 / 2));
    m_pResults->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxDataViewEvent&) { OnResultsSelectionChanged(); });
    pBox->Add(m_pResults, 1, wxEXPAND | wxALL, 4);

    return pBox;
}

wxSizer* MemoryInspectorDialog::CreateNotesControls()
{
    auto& vm = m_vmMemoryInspector;
    auto* pBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Memory Notes");
    auto* pParent = pBox->GetStaticBox();

    auto* pLeft = new wxBoxSizer(wxVERTICAL);
    pLeft->Add(new wxStaticText(pParent, wxID_ANY, "Address:"));
    auto* pAddressRow = new wxBoxSizer(wxHORIZONTAL);
    m_pAddress = new wxTextCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(100, -1)));
    m_pAddress->Bind(wxEVT_TEXT, [this, &vm](wxCommandEvent&) {
        vm.SetCurrentAddressText(FromWx(m_pAddress->GetValue()));
    });
    m_pAddress->Bind(wxEVT_KEY_DOWN, [&vm](wxKeyEvent& event) {
        // like the Win32 dialog, up/down in the address field move between notes
        if (event.GetKeyCode() == WXK_UP)
            vm.PreviousNote();
        else if (event.GetKeyCode() == WXK_DOWN)
            vm.NextNote();
        else
            event.Skip();
    });
    pAddressRow->Add(m_pAddress, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    auto* pNotesList = new wxButton(pParent, wxID_ANY, "...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    pNotesList->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.OpenNotesList(); });
    pAddressRow->Add(pNotesList, 0, wxALIGN_CENTER_VERTICAL);
    pLeft->Add(pAddressRow, 0, wxEXPAND | wxTOP, 2);
    auto* pAddBookmark = new wxButton(pParent, wxID_ANY, "Add Bookmark");
    pAddBookmark->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.BookmarkCurrentAddress(); });
    pLeft->Add(pAddBookmark, 0, wxEXPAND | wxTOP, 4);
    pBox->Add(pLeft, 0, wxALL, 4);

    m_pNote = new wxTextCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition,
                             wxSize(-1, GetCharHeight() * 5), wxTE_MULTILINE);
    m_pNote->Bind(wxEVT_TEXT, [this, &vm](wxCommandEvent&) { vm.SetCurrentAddressNote(FromWx(m_pNote->GetValue())); });
    pBox->Add(m_pNote, 1, wxEXPAND | wxALL, 4);

    auto* pRight = new wxBoxSizer(wxVERTICAL);
    m_pPublish = new wxButton(pParent, wxID_ANY, "Publish");
    m_pPublish->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.PublishCurrentAddressNote(); });
    pRight->Add(m_pPublish, 0, wxEXPAND);
    m_pRevert = new wxButton(pParent, wxID_ANY, "Revert");
    m_pRevert->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.RevertCurrentAddressNote(); });
    pRight->Add(m_pRevert, 0, wxEXPAND | wxTOP, 2);
    pBox->Add(pRight, 0, wxALL, 4);

    return pBox;
}

wxSizer* MemoryInspectorDialog::CreateViewerControls()
{
    auto& vmViewer = m_vmMemoryInspector.Viewer();
    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    auto* pOptions = new wxBoxSizer(wxHORIZONTAL);
    pOptions->Add(new wxStaticText(this, wxID_ANY, "Memory View:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    const std::array<std::pair<const char*, ra::data::Memory::Size>, 4> vSizes{{
        {"8-bit", ra::data::Memory::Size::EightBit},
        {"16-bit", ra::data::Memory::Size::SixteenBit},
        {"32-bit", ra::data::Memory::Size::ThirtyTwoBit},
        {"32-bit BE", ra::data::Memory::Size::ThirtyTwoBitBigEndian},
    }};
    for (size_t i = 0; i < vSizes.size(); ++i)
    {
        const auto nSize = vSizes.at(i).second;
        auto* pRadio = new wxRadioButton(this, wxID_ANY, vSizes.at(i).first, wxDefaultPosition, wxDefaultSize,
                                         i == 0 ? wxRB_GROUP : 0);
        pRadio->Bind(wxEVT_RADIOBUTTON, [&vmViewer, nSize](wxCommandEvent&) { vmViewer.SetSize(nSize); });
        pOptions->Add(pRadio, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        m_vSizes.at(i) = pRadio;
    }
    pOptions->AddStretchSpacer();

    // like the Win32 dialog, double clicking a bit toggles it
    m_pBitsLabel = new wxStaticText(this, wxID_ANY, "Bits:");
    pOptions->Add(m_pBitsLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    for (int nBit = 7; nBit >= 0; --nBit)
    {
        auto* pBit = new wxStaticText(this, wxID_ANY, "0");
        pBit->SetFont(wxFontInfo().Family(wxFONTFAMILY_TELETYPE));
        pBit->SetToolTip(wxString::Format("Bit %d (double click to toggle)", nBit));
        pBit->Bind(wxEVT_LEFT_DCLICK, [this, nBit](wxMouseEvent&) {
            m_vmMemoryInspector.ToggleBit(nBit);
            m_pViewer->Refresh(false);
        });
        pOptions->Add(pBit, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
        m_vBits.at(gsl::narrow_cast<size_t>(7 - nBit)) = pBit;
    }
    pSizer->Add(pOptions, 0, wxEXPAND | wxBOTTOM, 4);

    m_pViewer = new MemoryViewerControl(this, vmViewer);
    m_pViewer->SetMinSize(FromDIP(wxSize(-1, 120)));
    pSizer->Add(m_pViewer, 1, wxEXPAND);

    return pSizer;
}

void MemoryInspectorDialog::UpdateControls()
{
    auto& vm = m_vmMemoryInspector;
    auto& vmSearch = vm.Search();

    // search
    m_pRanges->Update(vmSearch.PredefinedFilterRanges(), vmSearch.GetPredefinedFilterRange());
    SetTextIfChanged(*m_pRange, vmSearch.GetFilterRange());
    m_pSearchType->Update(vmSearch.SearchTypes(), ra::etoi(vmSearch.GetSearchType()));
    m_pAligned->SetValue(vmSearch.IsAligned());
    m_pAligned->Enable(vmSearch.CanAlign() && vmSearch.CanBeginNewSearch());
    const bool bCanBeginNewSearch = vmSearch.CanBeginNewSearch();
    m_pRanges->Enable(bCanBeginNewSearch);
    m_pRange->Enable(bCanBeginNewSearch);
    m_pSearchType->Enable(bCanBeginNewSearch);
    m_pNewSearch->Enable(bCanBeginNewSearch);
    m_pImport->Enable(bCanBeginNewSearch);

    const bool bCanFilter = vmSearch.CanFilter();
    m_pComparison->Update(vmSearch.ComparisonTypes(), ra::etoi(vmSearch.GetComparisonType()));
    m_pValueType->Update(vmSearch.ValueTypes(), ra::etoi(vmSearch.GetValueType()));
    m_pComparison->Enable(bCanFilter);
    m_pValueType->Enable(bCanFilter);
    m_pFilterOnce->Enable(bCanFilter);
    m_pExport->Enable(bCanFilter);
    SetTextIfChanged(*m_pFilterValue, vmSearch.GetFilterValue());
    m_pFilterValue->Enable(vmSearch.IsFilterValueEditable());
    SetLabelIfChanged(*m_pContinuousFilter, vmSearch.GetContinuousFilterLabel());
    m_pContinuousFilter->Enable(vmSearch.CanContinuousFilter());

    // results
    SetLabelIfChanged(*m_pResultCount, vmSearch.GetResultCountText());
    SetLabelIfChanged(*m_pFilterSummary, vmSearch.GetFilterSummary());
    SetLabelIfChanged(*m_pPage, vmSearch.GetSelectedPage());
    m_pPreviousPage->Enable(vmSearch.CanGoToPreviousPage());
    m_pNextPage->Enable(vmSearch.CanGoToNextPage());
    m_pExclude->Enable(vmSearch.HasSelection());
    m_pBookmark->Enable(vmSearch.HasSelection());
    UpdateResults();

    // notes
    SetTextIfChanged(*m_pAddress, vm.GetCurrentAddressText());
    SetTextIfChanged(*m_pNote, vm.GetCurrentAddressNote());
    m_pNote->Enable(vm.CanEditCurrentAddressNote());
    m_pNote->SetEditable(!vm.IsCurrentAddressNoteReadOnly());
    m_pPublish->Enable(vm.CanPublishCurrentAddressNote());
    m_pRevert->Enable(vm.CanRevertCurrentAddressNote());

    // viewer
    const auto nSize = vm.Viewer().GetSize();
    const std::array<ra::data::Memory::Size, 4> vSizes{ra::data::Memory::Size::EightBit,
                                                       ra::data::Memory::Size::SixteenBit,
                                                       ra::data::Memory::Size::ThirtyTwoBit,
                                                       ra::data::Memory::Size::ThirtyTwoBitBigEndian};
    for (size_t i = 0; i < vSizes.size(); ++i)
    {
        if (vSizes.at(i) == nSize && !m_vSizes.at(i)->GetValue())
            m_vSizes.at(i)->SetValue(true);
    }

    // " 0 0 0 0 0 0 0 0": bit 7 first
    const std::wstring& sBits = vm.GetCurrentAddressBits();
    const bool bShowBits = vm.AreCurrentBitsVisible() && sBits.length() >= 16;
    m_pBitsLabel->Show(bShowBits);
    for (size_t i = 0; i < m_vBits.size(); ++i)
    {
        m_vBits.at(i)->Show(bShowBits);
        if (bShowBits)
            SetLabelIfChanged(*m_vBits.at(i), std::wstring(1, sBits.at(i * 2 + 1)));
    }
    m_pBitsLabel->GetContainingSizer()->Layout();

    // the viewer repaints when its view model asks for it (OnRepaintMemoryViewer)
}

void MemoryInspectorDialog::UpdateResults()
{
    auto& vmSearch = m_vmMemoryInspector.Search();
    const auto nTotal = vmSearch.GetScrollMaximum();
    if (m_pResultsModel->GetCount() != nTotal)
    {
        m_bSyncingSelection = true;
        m_pResultsModel->Reset(nTotal);
        m_bSyncingSelection = false;
    }
    else
    {
        // the visible rows' values change every frame
        const auto nOffset = vmSearch.GetScrollOffset();
        const auto nVisible = gsl::narrow_cast<unsigned int>(vmSearch.Results().Count());
        for (unsigned int nRow = nOffset; nRow < nOffset + nVisible && nRow < nTotal; ++nRow)
            m_pResultsModel->RowChanged(nRow);
    }

    // the view model clears the selection when the current address moves away from it
    if (!vmSearch.HasSelection() && m_pResults->GetSelectedItemsCount() > 0)
    {
        m_bSyncingSelection = true;
        m_pResults->UnselectAll();
        m_bSyncingSelection = false;
    }
}

void MemoryInspectorDialog::SyncScrollOffset()
{
    if (!IsShown() || m_pResultsModel->GetCount() == 0)
        return;

    const wxDataViewItem pTop = m_pResults->GetTopItem();
    if (!pTop.IsOk())
        return;

    auto& vmSearch = m_vmMemoryInspector.Search();
    const auto nTotal = vmSearch.GetScrollMaximum();
    const auto nMaxOffset = (nTotal > VISIBLE_RESULT_ROWS) ? nTotal - VISIBLE_RESULT_ROWS : 0U;
    const auto nTop = std::min(m_pResultsModel->GetRow(pTop), nMaxOffset);
    if (nTop != vmSearch.GetScrollOffset())
        vmSearch.SetScrollOffset(nTop); // updates the rows, which triggers UpdateControls
}

void MemoryInspectorDialog::OnResultsSelectionChanged()
{
    if (m_bSyncingSelection)
        return;

    auto& vmSearch = m_vmMemoryInspector.Search();
    const auto nTotal = gsl::narrow_cast<gsl::index>(vmSearch.GetScrollMaximum());
    if (nTotal == 0)
        return;

    wxDataViewItemArray vSelected;
    m_pResults->GetSelections(vSelected);

    std::vector<unsigned int> vRows;
    vRows.reserve(vSelected.size());
    for (const auto& pItem : vSelected)
        vRows.push_back(m_pResultsModel->GetRow(pItem));
    std::sort(vRows.begin(), vRows.end());

    // mirror the selection into the view model, a contiguous range at a time
    vmSearch.SelectRange(0, nTotal - 1, false);
    for (size_t i = 0; i < vRows.size();)
    {
        size_t j = i;
        while (j + 1 < vRows.size() && vRows.at(j + 1) == vRows.at(j) + 1)
            ++j;
        vmSearch.SelectRange(vRows.at(i), vRows.at(j), true);
        i = j + 1;
    }

    // like the Win32 list, selecting a single row makes it the current address
    if (vRows.size() == 1)
    {
        ra::services::SearchResult pResult{};
        if (vmSearch.GetResult(vRows.front(), pResult))
            m_vmMemoryInspector.SetCurrentAddress(pResult.nAddress);
    }
}

} // namespace wx
} // namespace ui
} // namespace ra
