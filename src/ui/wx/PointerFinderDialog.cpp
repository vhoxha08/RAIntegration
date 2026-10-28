#include "PointerFinderDialog.hh"

#include "services/ServiceLocator.hh"

#include "ui/viewmodels/WindowManager.hh"
#include "ui/wx/MemoryViewerControl.hh"
#include "ui/wx/WxApp.hh"
#include "ui/wx/WxGrid.hh"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::PointerFinderViewModel;
using PotentialPointer = PointerFinderViewModel::PotentialPointerViewModel;
using StateViewModel = PointerFinderViewModel::StateViewModel;

PointerFinderDialog::PointerFinderDialog(PointerFinderViewModel& vmPointerFinder) :
    WxDialog(vmPointerFinder, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX)
{
    auto& vm = vmPointerFinder;
    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    // Results
    auto* pResultsBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Results");
    auto* pResultsParent = pResultsBox->GetStaticBox();
    auto* pLeft = new wxBoxSizer(wxVERTICAL);
    auto* pCountRow = new wxBoxSizer(wxHORIZONTAL);
    pCountRow->Add(new wxStaticText(pResultsParent, wxID_ANY, "Count:"), 0, wxRIGHT, 4);
    auto* pCount = new wxStaticText(pResultsParent, wxID_ANY, "0");
    m_pBinder.BindLabel(pCount, vm, PointerFinderViewModel::ResultCountTextProperty);
    pCountRow->Add(pCount, 1);
    pLeft->Add(pCountRow, 0, wxEXPAND | wxBOTTOM, 4);
    auto* pSearchType = new LookupChoice(pResultsParent);
    m_pBinder.BindChoice(pSearchType, vm.SearchTypes(), vm, PointerFinderViewModel::SearchTypeProperty);
    pLeft->Add(pSearchType, 0, wxEXPAND | wxBOTTOM, 4);
    auto* pFind = new wxButton(pResultsParent, wxID_ANY, "Find");
    pFind->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.Find(); });
    pLeft->Add(pFind, 0, wxEXPAND);
    pLeft->AddStretchSpacer();
    auto* pBookmark = new wxButton(pResultsParent, wxID_ANY, "Bookmark Selected");
    pBookmark->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.BookmarkSelected(); });
    pLeft->Add(pBookmark, 0, wxEXPAND | wxBOTTOM, 2);
    auto* pCopy = new wxButton(pResultsParent, wxID_ANY, "Copy Chain");
    pCopy->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.CopySelectedToClipboard(); });
    pLeft->Add(pCopy, 0, wxEXPAND);
    pResultsBox->Add(pLeft, 0, wxEXPAND | wxALL, 4);

    m_pResults = new Grid(pResultsParent, wxDV_SINGLE | wxDV_ROW_LINES);
    m_pResults->AddColumn(GridColumn::Text(L"Address", PotentialPointer::PointerAddressProperty, 100));
    m_pResults->AddColumn(GridColumn::Text(L"Offset", PotentialPointer::OffsetProperty, 60));
    m_pResults->AddColumn(GridColumn::Text(L"State 1", PotentialPointer::PointerValue1Property, 80));
    m_pResults->AddColumn(GridColumn::Text(L"State 2", PotentialPointer::PointerValue2Property, 80));
    m_pResults->AddColumn(GridColumn::Text(L"State 3", PotentialPointer::PointerValue3Property, 80));
    m_pResults->AddColumn(GridColumn::Text(L"State 4", PotentialPointer::PointerValue4Property, 80));
    m_pResults->BindIsSelected(PotentialPointer::IsSelectedProperty);
    m_pResults->BindItems(vm.PotentialPointers());
    m_pResults->SetDoubleClickHandler([&vm](gsl::index nIndex) {
        // like the Win32 dialog: double clicking a pointer shows it in the memory inspector
        const auto* pItem = vm.PotentialPointers().GetItemAt(nIndex);
        if (pItem != nullptr && !pItem->GetOffset().empty())
        {
            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector.SetCurrentAddress(
                pItem->GetRawAddress());
        }
    });
    m_pResults->SetMinSize(FromDIP(wxSize(560, 150)));
    pResultsBox->Add(m_pResults, 1, wxEXPAND | wxALL, 4);
    pSizer->Add(pResultsBox, 1, wxEXPAND | wxALL, 8);

    // States
    for (size_t i = 0; i < vm.States().size(); ++i)
    {
        auto& vmState = vm.States().at(i);
        auto* pStateBox = new wxStaticBoxSizer(wxHORIZONTAL, this, wxString::Format("State %zu", i + 1));
        auto* pParent = pStateBox->GetStaticBox();

        auto* pStateLeft = new wxBoxSizer(wxVERTICAL);
        pStateLeft->Add(new wxStaticText(pParent, wxID_ANY, "Address:"));
        auto* pAddress = new wxTextCtrl(pParent, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(120, -1)));
        m_pBinder.BindText(pAddress, vmState, StateViewModel::AddressProperty);
        m_pBinder.BindEnabled(pAddress, vmState, StateViewModel::CanCaptureProperty);
        pStateLeft->Add(pAddress, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
        auto* pCapture = new wxButton(pParent, wxID_ANY, "Capture");
        pCapture->Bind(wxEVT_BUTTON, [&vmState](wxCommandEvent&) { vmState.ToggleCapture(); });
        m_pBinder.BindLabel(pCapture, vmState, StateViewModel::CaptureButtonTextProperty);
        pStateLeft->Add(pCapture, 0, wxEXPAND);
        pStateBox->Add(pStateLeft, 0, wxALL, 4);

        auto* pViewer = new MemoryViewerControl(pParent, vmState.Viewer());
        pViewer->SetMinSize(FromDIP(wxSize(-1, 84)));
        pStateBox->Add(pViewer, 1, wxEXPAND | wxALL, 4);
        pSizer->Add(pStateBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        Watch(vmState);
    }

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Pointer Finder");
    UpdateControls();
}

} // namespace wx
} // namespace ui
} // namespace ra
