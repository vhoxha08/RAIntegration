#ifndef RA_UI_WX_POINTER_FINDER_DIALOG_HH
#define RA_UI_WX_POINTER_FINDER_DIALOG_HH
#pragma once

#include "ui/viewmodels/PointerFinderViewModel.hh"
#include "ui/wx/WxDialog.hh"

namespace ra {
namespace ui {
namespace wx {

class Grid;

/// <summary>
/// wxWidgets version of the Win32 PointerFinderDialog.
/// </summary>
class PointerFinderDialog : public WxDialog
{
public:
    explicit PointerFinderDialog(ra::ui::viewmodels::PointerFinderViewModel& vmPointerFinder);

protected:
    void UpdateControls() override { m_pBinder.Update(); }

private:
    Binder m_pBinder;
    Grid* m_pResults = nullptr;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_POINTER_FINDER_DIALOG_HH
