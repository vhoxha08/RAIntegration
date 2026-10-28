#include "PointerInspectorDialog.hh"

#include "ui/wx/MemoryWatchColumns.hh"
#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::MemoryWatchListViewModel;
using ra::ui::viewmodels::PointerInspectorViewModel;
using StructField = PointerInspectorViewModel::StructFieldViewModel;

PointerInspectorDialog::PointerInspectorDialog(PointerInspectorViewModel& vmPointerInspector) :
    WxDialog(vmPointerInspector, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX)
{
    auto& vm = vmPointerInspector;
    auto& vmFields = vm.Fields();
    Watch(vmFields);
    Watch(vm.KnownPointers());
    Watch(vm.Nodes());

    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    auto* pHeader = new wxFlexGridSizer(4, wxSize(6, 4));
    pHeader->AddGrowableCol(3);
    pHeader->Add(new wxStaticText(this, wxID_ANY, "Root Address:"), 0, wxALIGN_CENTER_VERTICAL);
    auto* pAddress = new LookupChoice(this);
    pAddress->SetMinSize(FromDIP(wxSize(120, -1)));
    m_pBinder.BindChoice(pAddress, vm.KnownPointers(), vm, PointerInspectorViewModel::CurrentAddressProperty);
    pHeader->Add(pAddress, 0, wxALIGN_CENTER_VERTICAL);
    pHeader->Add(new wxStaticText(this, wxID_ANY, "Viewing Node:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    auto* pNode = new LookupChoice(this);
    m_pBinder.BindChoice(pNode, vm.Nodes(), vm, PointerInspectorViewModel::SelectedNodeProperty);
    pHeader->Add(pNode, 1, wxEXPAND);
    pSizer->Add(pHeader, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* pDescriptionRow = new wxBoxSizer(wxHORIZONTAL);
    pDescriptionRow->Add(new wxStaticText(this, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    auto* pDescription = new wxTextCtrl(this, wxID_ANY);
    m_pBinder.BindText(pDescription, vm, PointerInspectorViewModel::CurrentAddressNoteProperty);
    pDescriptionRow->Add(pDescription, 1);
    pSizer->Add(pDescriptionRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* pChain = new Grid(this, wxDV_SINGLE | wxDV_ROW_LINES);
    pChain->AddColumn(GridColumn::Text(L"Offset", StructField::OffsetProperty, 80));
    pChain->AddColumn(MemoryWatchAddressColumn(L"Address", StructField::AddressProperty));
    pChain->AddColumn(MemoryWatchValueColumn(L"Value", StructField::CurrentValueProperty, false));
    pChain->AddColumn(GridColumn::Text(L"Description", StructField::DescriptionProperty, 300));
    pChain->BindRowColor(StructField::RowColorProperty);
    pChain->BindItems(vm.PointerChain());
    pChain->SetMinSize(FromDIP(wxSize(560, 100)));
    pSizer->Add(pChain, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* pFields = new Grid(this);
    auto pOffset = GridColumn::Text(L"Offset", StructField::OffsetProperty, 80, true);
    pFields->AddColumn(std::move(pOffset));
    pFields->AddColumn(GridColumn::Text(L"Description", StructField::DescriptionProperty, 170));
    pFields->AddColumn(GridColumn::Lookup(L"Size", StructField::SizeProperty, vmFields.Sizes(), 96, true));
    pFields->AddColumn(MemoryWatchFormatColumn(L"Format", StructField::FormatProperty, vmFields.Formats()));
    pFields->AddColumn(MemoryWatchValueColumn(L"Value", StructField::CurrentValueProperty, true));
    pFields->BindIsSelected(StructField::IsSelectedProperty);
    pFields->BindRowColor(StructField::RowColorProperty);
    pFields->BindItems(vmFields.Items());
    pFields->SetMinSize(FromDIP(wxSize(560, 180)));
    pSizer->Add(pFields, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* pNote = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(-1, 90)),
                                 wxTE_MULTILINE);
    m_pBinder.BindText(pNote, vm, PointerInspectorViewModel::CurrentFieldNoteProperty);
    m_pBinder.BindEnabled(pNote, vmFields, MemoryWatchListViewModel::HasSingleSelectionProperty);
    pSizer->Add(pNote, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    auto AddButton = [this, pButtons](const char* sLabel, std::function<void()> fAction) {
        auto* pButton = new wxButton(this, wxID_ANY, sLabel);
        pButton->Bind(wxEVT_BUTTON, [fAction](wxCommandEvent&) { fAction(); });
        pButtons->Add(pButton, 0, wxRIGHT, 4);
        return pButton;
    };
    m_pBinder.BindEnabled(AddButton("Bookmark Selected", [&vm]() { vm.BookmarkCurrentField(); }), vmFields,
                          MemoryWatchListViewModel::HasSelectionProperty);
    m_pBinder.BindEnabled(AddButton("Copy Chain", [&vm]() { vm.CopyDefinition(); }), vmFields,
                          MemoryWatchListViewModel::HasSingleSelectionProperty);
    pButtons->AddStretchSpacer();
    m_pBinder.BindEnabled(AddButton("Remove Field", [&vm]() { vm.RemoveSelectedField(); }), vmFields,
                          MemoryWatchListViewModel::HasSingleSelectionProperty);
    m_pBinder.BindEnabled(AddButton("New Field", [&vm]() { vm.NewField(); }), vm,
                          PointerInspectorViewModel::HasSelectedNodeProperty);
    pSizer->Add(pButtons, 0, wxEXPAND | wxALL, 8);

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Pointer Inspector");
    UpdateControls();
}

} // namespace wx
} // namespace ui
} // namespace ra
