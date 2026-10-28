#include "RichPresenceDialog.hh"

#include "ui/wx/WxApp.hh"

#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ra {
namespace ui {
namespace wx {

RichPresenceDialog::RichPresenceDialog(ra::ui::viewmodels::RichPresenceMonitorViewModel& vmRichPresence) :
    WxDialog(vmRichPresence, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_vmRichPresence(vmRichPresence)
{
    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    m_pText = new wxStaticText(this, wxID_ANY, wxEmptyString);
    pSizer->Add(m_pText, 1, wxEXPAND | wxALL, 10);
    SetSizer(pSizer);
    SetClientSize(FromDIP(wxSize(320, 80)));

    // rewrap when resized
    Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
        UpdateControls();
        event.Skip();
    });

    RememberSizeAndPosition("Rich Presence Monitor");
    UpdateControls();
}

void RichPresenceDialog::UpdateControls()
{
    const wxString sText = ToWx(m_vmRichPresence.GetDisplayString());
    m_pText->SetLabel(sText);
    m_pText->Wrap(GetClientSize().GetWidth() - FromDIP(20));
}

} // namespace wx
} // namespace ui
} // namespace ra
