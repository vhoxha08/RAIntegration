#ifndef RA_UI_WX_GAME_CHECKSUM_DIALOG_HH
#define RA_UI_WX_GAME_CHECKSUM_DIALOG_HH
#pragma once

#include "ui/viewmodels/GameChecksumViewModel.hh"
#include "ui/wx/WxDialog.hh"

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 GameChecksumDialog.
/// </summary>
class GameChecksumDialog : public WxDialog
{
public:
    explicit GameChecksumDialog(ra::ui::viewmodels::GameChecksumViewModel& vmChecksum);
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_GAME_CHECKSUM_DIALOG_HH
