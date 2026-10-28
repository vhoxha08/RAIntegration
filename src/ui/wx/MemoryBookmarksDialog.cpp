#include "MemoryBookmarksDialog.hh"

#include "ui/wx/MemoryWatchColumns.hh"
#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/sizer.h>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::MemoryBookmarksViewModel;
using ra::ui::viewmodels::MemoryWatchListViewModel;
using ra::ui::viewmodels::MemoryWatchViewModel;
using BookmarkViewModel = MemoryBookmarksViewModel::MemoryBookmarkViewModel;

MemoryBookmarksDialog::MemoryBookmarksDialog(MemoryBookmarksViewModel& vmMemoryBookmarks) :
    WxDialog(vmMemoryBookmarks, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX)
{
    auto& vm = vmMemoryBookmarks;
    auto& vmList = vm.Bookmarks();
    Watch(vmList);

    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    auto* pTop = new wxBoxSizer(wxHORIZONTAL);
    auto* pSave = new wxButton(this, wxID_ANY, "Save As");
    pSave->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.SaveBookmarkFile(); });
    pTop->Add(pSave, 0, wxRIGHT, 4);
    auto* pLoad = new wxButton(this, wxID_ANY, "Load");
    pLoad->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.LoadBookmarkFile(); });
    pTop->Add(pLoad);
    pTop->AddStretchSpacer();
    auto* pClearChanges = new wxButton(this, wxID_ANY, "Clear All Changes");
    pClearChanges->Bind(wxEVT_BUTTON, [&vmList](wxCommandEvent&) { vmList.ClearAllChanges(); });
    pTop->Add(pClearChanges);
    pSizer->Add(pTop, 0, wxEXPAND | wxALL, 8);

    m_pGrid = new Grid(this);
    auto pDescription = GridColumn::Text(L"Description", MemoryWatchViewModel::DescriptionProperty, 180, true);
    m_pGrid->AddColumn(std::move(pDescription));
    auto pAddress = MemoryWatchAddressColumn(L"Address", MemoryWatchViewModel::AddressProperty);
    pAddress.fGetTooltip = GridColumn::TooltipFrom(MemoryWatchViewModel::RealNoteProperty);
    m_pGrid->AddColumn(std::move(pAddress));
    auto pSize = GridColumn::Lookup(L"Size", MemoryWatchViewModel::SizeProperty, vmList.Sizes(), 96, true);
    m_pGrid->AddColumn(std::move(pSize));
    m_pGrid->AddColumn(MemoryWatchFormatColumn(L"Format", MemoryWatchViewModel::FormatProperty, vmList.Formats()));
    m_pGrid->AddColumn(MemoryWatchValueColumn(L"Value", MemoryWatchViewModel::CurrentValueProperty, true));
    m_pGrid->AddColumn(MemoryWatchValueColumn(L"Prior", MemoryWatchViewModel::PreviousValueProperty, false));
    m_pGrid->AddColumn(GridColumn::Number(L"Changes", MemoryWatchViewModel::ChangesProperty, 64));
    m_pGrid->AddColumn(GridColumn::Lookup(L"Behavior", BookmarkViewModel::BehaviorProperty, vm.Behaviors(), 80, true));
    m_pGrid->BindIsSelected(BookmarkViewModel::IsSelectedProperty);
    m_pGrid->BindRowColor(MemoryWatchViewModel::RowColorProperty);
    m_pGrid->BindItems(vmList.Items());
    m_pGrid->SetPrimaryEditColumn(0); // description
    m_pGrid->SetMinSize(FromDIP(wxSize(720, 240)));
    pSizer->Add(m_pGrid, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

    auto* pBottom = new wxBoxSizer(wxHORIZONTAL);
    auto AddButton = [this, pBottom, &vmList](const char* sLabel, std::function<void()> fAction, int nSpaceAfter) {
        auto* pButton = new wxButton(this, wxID_ANY, sLabel);
        pButton->Bind(wxEVT_BUTTON, [fAction](wxCommandEvent&) { fAction(); });
        m_pBinder.BindEnabled(pButton, vmList, MemoryWatchListViewModel::HasSelectionProperty);
        pBottom->Add(pButton, 0, wxRIGHT, nSpaceAfter);
        return pButton;
    };
    AddButton("Remove Selected", [&vm]() { vm.RemoveSelectedBookmarks(); }, 0);
    pBottom->AddStretchSpacer();
    auto* pPause = AddButton("Pause", [&vm]() { vm.TogglePauseSelected(); }, 4);
    m_pBinder.BindLabel(pPause, vm, MemoryBookmarksViewModel::PauseButtonTextProperty);
    auto* pFreeze = AddButton("Freeze", [&vm]() { vm.ToggleFreezeSelected(); }, 0);
    m_pBinder.BindLabel(pFreeze, vm, MemoryBookmarksViewModel::FreezeButtonTextProperty);
    pBottom->AddStretchSpacer();
    AddButton("Move Up", [&vm]() { vm.MoveSelectedBookmarksUp(); }, 4);
    AddButton("Move Down", [&vm]() { vm.MoveSelectedBookmarksDown(); }, 0);
    pSizer->Add(pBottom, 0, wxEXPAND | wxALL, 8);

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Memory Bookmarks");
    UpdateControls();
}

} // namespace wx
} // namespace ui
} // namespace ra
