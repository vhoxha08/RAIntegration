#ifndef RA_UI_WX_OVERLAY_SETTINGS_DIALOG_HH
#define RA_UI_WX_OVERLAY_SETTINGS_DIALOG_HH
#pragma once

#include "ui/viewmodels/OverlaySettingsViewModel.hh"
#include "ui/wx/WxDialog.hh"

#include <vector>

class wxCheckBox;
class wxTextCtrl;

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 OverlaySettingsDialog.
/// </summary>
class OverlaySettingsDialog : public WxDialog
{
public:
    explicit OverlaySettingsDialog(ra::ui::viewmodels::OverlaySettingsViewModel& vmSettings);

protected:
    void UpdateControls() override;

private:
    struct LocationRow
    {
        LookupChoice* pChoice;
        const ra::ui::viewmodels::LookupItemViewModelCollection* pItems;
        std::function<ra::ui::viewmodels::PopupLocation()> fGet;
    };

    ra::ui::viewmodels::OverlaySettingsViewModel& m_vmSettings;
    std::vector<LocationRow> m_vLocations;
    wxCheckBox* m_pScreenshotTrigger = nullptr;
    wxCheckBox* m_pScreenshotMastery = nullptr;
    wxTextCtrl* m_pScreenshotLocation = nullptr;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_OVERLAY_SETTINGS_DIALOG_HH
