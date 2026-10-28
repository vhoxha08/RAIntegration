#ifndef RA_UI_WX_ASSET_LIST_DIALOG_HH
#define RA_UI_WX_ASSET_LIST_DIALOG_HH
#pragma once

#include "ui/viewmodels/AssetListViewModel.hh"
#include "ui/wx/WxDialog.hh"

namespace ra {
namespace ui {
namespace wx {

class Grid;

/// <summary>
/// wxWidgets version of the Win32 AssetListDialog.
/// </summary>
class AssetListDialog : public WxDialog
{
public:
    explicit AssetListDialog(ra::ui::viewmodels::AssetListViewModel& vmAssetList);

protected:
    void UpdateControls() override { m_pBinder.Update(); }

private:
    Binder m_pBinder;
    Grid* m_pAssets = nullptr;
    int m_nEnsureVisibleIndex = -1;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_ASSET_LIST_DIALOG_HH
