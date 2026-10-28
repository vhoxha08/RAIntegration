#ifndef RA_UI_WX_MEMORY_BOOKMARKS_DIALOG_HH
#define RA_UI_WX_MEMORY_BOOKMARKS_DIALOG_HH
#pragma once

#include "ui/viewmodels/MemoryBookmarksViewModel.hh"
#include "ui/wx/WxDialog.hh"

namespace ra {
namespace ui {
namespace wx {

class Grid;

/// <summary>
/// wxWidgets version of the Win32 MemoryBookmarksDialog.
/// </summary>
class MemoryBookmarksDialog : public WxDialog
{
public:
    explicit MemoryBookmarksDialog(ra::ui::viewmodels::MemoryBookmarksViewModel& vmMemoryBookmarks);

protected:
    void UpdateControls() override { m_pBinder.Update(); }

private:
    Binder m_pBinder;
    Grid* m_pGrid = nullptr;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_MEMORY_BOOKMARKS_DIALOG_HH
