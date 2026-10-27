#ifndef RA_UI_WX_RICH_PRESENCE_DIALOG_HH
#define RA_UI_WX_RICH_PRESENCE_DIALOG_HH
#pragma once

#include "ui/viewmodels/RichPresenceMonitorViewModel.hh"
#include "ui/wx/WxDialog.hh"

class wxStaticText;

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 RichPresenceDialog.
/// </summary>
class RichPresenceDialog : public WxDialog
{
public:
    explicit RichPresenceDialog(ra::ui::viewmodels::RichPresenceMonitorViewModel& vmRichPresence);

protected:
    void UpdateControls() override;

private:
    ra::ui::viewmodels::RichPresenceMonitorViewModel& m_vmRichPresence;
    wxStaticText* m_pText = nullptr;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_RICH_PRESENCE_DIALOG_HH
