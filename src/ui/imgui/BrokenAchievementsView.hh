#ifndef RA_UI_IMGUI_BROKEN_ACHIEVEMENTS_VIEW_HH
#define RA_UI_IMGUI_BROKEN_ACHIEVEMENTS_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/BrokenAchievementsViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 BrokenAchievementsDialog.
/// </summary>
class BrokenAchievementsView : public ImGuiWindowView
{
public:
    explicit BrokenAchievementsView(ra::ui::viewmodels::BrokenAchievementsViewModel& vmBrokenAchievements) noexcept
        : ImGuiWindowView(vmBrokenAchievements), m_vmBrokenAchievements(vmBrokenAchievements)
    {
    }

    void RenderContents() override;

private:
    ra::ui::viewmodels::BrokenAchievementsViewModel& m_vmBrokenAchievements;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_BROKEN_ACHIEVEMENTS_VIEW_HH
