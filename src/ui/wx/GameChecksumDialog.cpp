#include "GameChecksumDialog.hh"

#include "ui/wx/WxApp.hh"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

GameChecksumDialog::GameChecksumDialog(ra::ui::viewmodels::GameChecksumViewModel& vmChecksum) : WxDialog(vmChecksum)
{
    SetTitle("ROM Checksum"); // the Win32 dialog's caption comes from its resource

    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    pSizer->Add(new wxStaticText(this, wxID_ANY, "RetroAchievements Hash:"), 0, wxLEFT | wxRIGHT | wxTOP, 10);

    // selectable, so it can also be copied by hand
    auto* pHash = new wxTextCtrl(this, wxID_ANY, ToWx(vmChecksum.GetChecksum()), wxDefaultPosition, wxSize(300, -1),
                                 wxTE_READONLY | wxBORDER_NONE);
    pHash->SetBackgroundColour(GetBackgroundColour());
    pSizer->Add(pHash, 0, wxEXPAND | wxALL, 10);

    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    auto* pCopy = new wxButton(this, wxID_ANY, "Copy To Clipboard");
    pCopy->Bind(wxEVT_BUTTON, [&vmChecksum](wxCommandEvent&) { vmChecksum.CopyChecksumToClipboard(); });
    pButtons->Add(pCopy);
    pButtons->AddStretchSpacer();
    auto* pOK = new wxButton(this, wxID_OK);
    pOK->SetDefault();
    pOK->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { CloseWithResult(DialogResult::OK); });
    pButtons->Add(pOK);
    pSizer->Add(pButtons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizerAndFit(pSizer);
}

} // namespace wx
} // namespace ui
} // namespace ra
