#include "BrokenAchievementsDialog.hh"

#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ra {
namespace ui {
namespace wx {

BrokenAchievementsDialog::BrokenAchievementsDialog(
    ra::ui::viewmodels::BrokenAchievementsViewModel& vmBrokenAchievements) :
    WxDialog(vmBrokenAchievements, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_vmBrokenAchievements(vmBrokenAchievements)
{
    auto& vm = vmBrokenAchievements;
    Watch(vm.Achievements());

    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    pSizer->Add(new wxStaticText(this, wxID_ANY, "Which achievement would you like to report?"), 0, wxALL, 10);

    // single selection list; the view model keeps one achievement selected
    m_pList = new wxListView(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(620, 320)),
                             wxLC_REPORT | wxLC_SINGLE_SEL);
    m_pList->AppendColumn("Title", wxLIST_FORMAT_LEFT, FromDIP(180));
    m_pList->AppendColumn("Description", wxLIST_FORMAT_LEFT, FromDIP(340));
    m_pList->AppendColumn("Achieved", wxLIST_FORMAT_LEFT, FromDIP(70));
    m_pList->Bind(wxEVT_LIST_ITEM_SELECTED, [this, &vm](wxListEvent& event) {
        if (m_bUpdating)
            return;
        if (auto* pItem = vm.Achievements().GetItemAt(event.GetIndex()))
            pItem->SetSelected(true);
    });
    pSizer->Add(m_pList, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    auto* pButtons = new wxStdDialogButtonSizer();
    auto* pReport = new wxButton(this, wxID_OK, "Report Problem");
    pReport->SetDefault();
    pReport->Bind(wxEVT_BUTTON, [this, &vm](wxCommandEvent&) {
        // opens the report page in the browser, or shows an error message box
        if (vm.Submit())
            CloseWithResult(DialogResult::OK);
    });
    pButtons->AddButton(pReport);
    pButtons->AddButton(new wxButton(this, wxID_CANCEL));
    pButtons->Realize();
    pSizer->Add(pButtons, 0, wxEXPAND | wxALL, 10);

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Report Broken Achievements");
    UpdateControls();
}

void BrokenAchievementsDialog::UpdateControls()
{
    m_bUpdating = true;

    auto& vAchievements = m_vmBrokenAchievements.Achievements();
    const auto nCount = gsl::narrow_cast<long>(vAchievements.Count());
    if (m_pList->GetItemCount() != nCount)
    {
        m_pList->DeleteAllItems();
        for (long i = 0; i < nCount; ++i)
            m_pList->InsertItem(i, wxEmptyString);
    }

    for (long i = 0; i < nCount; ++i)
    {
        const auto* pItem = vAchievements.GetItemAt(i);
        if (pItem == nullptr)
            continue;

        m_pList->SetItem(i, 0, ToWx(pItem->GetLabel()));
        m_pList->SetItem(i, 1, ToWx(pItem->GetDescription()));
        m_pList->SetItem(i, 2, pItem->IsAchieved() ? "Yes" : "No");
        if (m_pList->IsSelected(i) != pItem->IsSelected())
            m_pList->Select(i, pItem->IsSelected());
    }

    m_bUpdating = false;
}

} // namespace wx
} // namespace ui
} // namespace ra
