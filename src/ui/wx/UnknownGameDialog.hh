#ifndef RA_UI_WX_UNKNOWN_GAME_DIALOG_HH
#define RA_UI_WX_UNKNOWN_GAME_DIALOG_HH
#pragma once

#include "ui/viewmodels/UnknownGameViewModel.hh"
#include "ui/wx/WxDialog.hh"

class wxButton;
class wxStaticText;
class wxTextCtrl;

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 UnknownGameDialog.
/// </summary>
class UnknownGameDialog : public WxDialog
{
public:
    explicit UnknownGameDialog(ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame);

protected:
    void UpdateControls() override;

private:
    ra::ui::viewmodels::UnknownGameViewModel& m_vmUnknownGame;
    wxStaticText* m_pHeader = nullptr;
    wxStaticText* m_pName = nullptr;
    wxStaticText* m_pSystem = nullptr;
    wxStaticText* m_pHash = nullptr;
    LookupChoice* m_pGames = nullptr;
    wxTextCtrl* m_pFilter = nullptr;
    wxStaticText* m_pResults = nullptr;
    wxTextCtrl* m_pNewName = nullptr;
    wxButton* m_pLink = nullptr;
    wxButton* m_pTest = nullptr;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_UNKNOWN_GAME_DIALOG_HH
