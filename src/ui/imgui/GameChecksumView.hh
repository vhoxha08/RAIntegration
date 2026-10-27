#ifndef RA_UI_IMGUI_GAME_CHECKSUM_VIEW_HH
#define RA_UI_IMGUI_GAME_CHECKSUM_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/GameChecksumViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 GameChecksumDialog.
/// </summary>
class GameChecksumView : public ImGuiWindowView
{
public:
    explicit GameChecksumView(ra::ui::viewmodels::GameChecksumViewModel& vmChecksum) noexcept
        : ImGuiWindowView(vmChecksum), m_vmChecksum(vmChecksum)
    {
        vmChecksum.SetWindowTitle(L"ROM Checksum"); // the Win32 dialog's caption comes from its resource
    }

    void RenderContents() override;

private:
    ra::ui::viewmodels::GameChecksumViewModel& m_vmChecksum;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_GAME_CHECKSUM_VIEW_HH
