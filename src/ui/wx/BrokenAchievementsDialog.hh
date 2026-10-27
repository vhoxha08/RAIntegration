#ifndef RA_UI_WX_BROKEN_ACHIEVEMENTS_DIALOG_HH
#define RA_UI_WX_BROKEN_ACHIEVEMENTS_DIALOG_HH
#pragma once

#include "ui/viewmodels/BrokenAchievementsViewModel.hh"
#include "ui/wx/WxDialog.hh"

class wxListView;

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 BrokenAchievementsDialog.
/// </summary>
class BrokenAchievementsDialog : public WxDialog
{
public:
    explicit BrokenAchievementsDialog(ra::ui::viewmodels::BrokenAchievementsViewModel& vmBrokenAchievements);

protected:
    void UpdateControls() override;

private:
    ra::ui::viewmodels::BrokenAchievementsViewModel& m_vmBrokenAchievements;
    wxListView* m_pList = nullptr;
    bool m_bUpdating = false;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_BROKEN_ACHIEVEMENTS_DIALOG_HH
