#ifndef RA_UI_WX_ASSET_EDITOR_DIALOG_HH
#define RA_UI_WX_ASSET_EDITOR_DIALOG_HH
#pragma once

#include "ui/viewmodels/AssetEditorViewModel.hh"
#include "ui/wx/WxDialog.hh"

#include <wx/timer.h>

class wxCheckBox;
class wxSpinButton;
class wxStaticBitmap;
class wxStaticText;
class wxTextCtrl;

namespace ra {
namespace ui {
namespace wx {

class Grid;

/// <summary>
/// wxWidgets version of the Win32 AssetEditorDialog (the achievement / leaderboard editor).
/// </summary>
class AssetEditorDialog : public WxDialog
{
public:
    explicit AssetEditorDialog(ra::ui::viewmodels::AssetEditorViewModel& vmAssetEditor);

protected:
    void UpdateControls() override;

private:
    wxSizer* CreateHeader();
    wxSizer* CreateTrigger();

    void UpdatePoints();
    void UpdateBadge();
    void ApplyBadgeText();
    void OnActiveClicked();
    void ShowConditionAddressInInspector(gsl::index nIndex);

    ra::ui::viewmodels::AssetEditorViewModel& m_vmAssetEditor;
    Binder m_pBinder;

    wxCheckBox* m_pActive = nullptr;
    LookupChoice* m_pPoints = nullptr;
    wxTextCtrl* m_pBadge = nullptr;
    wxSpinButton* m_pBadgeSpin = nullptr;
    wxStaticBitmap* m_pBadgeImage = nullptr;
    std::wstring m_sDisplayedBadge;
    bool m_bBadgeImageLoaded = false;
    wxTimer m_pBadgeTimer; // checks for a downloaded badge
    wxStaticText* m_pErrorIndicator = nullptr;
    Grid* m_pGroups = nullptr;
    Grid* m_pConditions = nullptr;
    int m_nEnsureVisibleGroup = -1;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_ASSET_EDITOR_DIALOG_HH
