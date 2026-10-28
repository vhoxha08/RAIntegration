#ifndef RA_UI_WX_POINTER_INSPECTOR_DIALOG_HH
#define RA_UI_WX_POINTER_INSPECTOR_DIALOG_HH
#pragma once

#include "ui/viewmodels/PointerInspectorViewModel.hh"
#include "ui/wx/WxDialog.hh"

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 PointerInspectorDialog.
/// </summary>
class PointerInspectorDialog : public WxDialog
{
public:
    explicit PointerInspectorDialog(ra::ui::viewmodels::PointerInspectorViewModel& vmPointerInspector);

protected:
    void UpdateControls() override { m_pBinder.Update(); }

private:
    Binder m_pBinder;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_POINTER_INSPECTOR_DIALOG_HH
