#ifndef RA_UI_IMGUI_RICH_PRESENCE_VIEW_HH
#define RA_UI_IMGUI_RICH_PRESENCE_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/RichPresenceMonitorViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 RichPresenceDialog.
/// </summary>
class RichPresenceView : public ImGuiWindowView
{
public:
    explicit RichPresenceView(ra::ui::viewmodels::RichPresenceMonitorViewModel& vmRichPresence) noexcept
        : ImGuiWindowView(vmRichPresence), m_vmRichPresence(vmRichPresence)
    {
    }

    void RenderContents() override;
    ImVec2 GetInitialSize() const override;

private:
    ra::ui::viewmodels::RichPresenceMonitorViewModel& m_vmRichPresence;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_RICH_PRESENCE_VIEW_HH
