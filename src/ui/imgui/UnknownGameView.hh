#ifndef RA_UI_IMGUI_UNKNOWN_GAME_VIEW_HH
#define RA_UI_IMGUI_UNKNOWN_GAME_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/UnknownGameViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 UnknownGameDialog.
/// </summary>
class UnknownGameView : public ImGuiWindowView
{
public:
    explicit UnknownGameView(ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        : ImGuiWindowView(vmUnknownGame),
          m_vmUnknownGame(vmUnknownGame),
          m_sFilter(ToUtf8(vmUnknownGame.GetFilterText())),
          m_sNewName(ToUtf8(vmUnknownGame.GetNewGameName()))
    {
    }

    void RenderContents() override;

    // the queued link/test action refers to this view; don't close while it is pending
    void OnCloseRequested() override
    {
        if (!m_bBusy)
            Close(DialogResult::Cancel);
    }

private:
    ra::ui::viewmodels::UnknownGameViewModel& m_vmUnknownGame;
    std::string m_sFilter;
    std::string m_sNewName;
    bool m_bBusy = false;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_UNKNOWN_GAME_VIEW_HH
