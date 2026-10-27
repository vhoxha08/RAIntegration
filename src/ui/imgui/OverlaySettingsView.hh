#ifndef RA_UI_IMGUI_OVERLAY_SETTINGS_VIEW_HH
#define RA_UI_IMGUI_OVERLAY_SETTINGS_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/OverlaySettingsViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 OverlaySettingsDialog.
/// </summary>
class OverlaySettingsView : public ImGuiWindowView
{
public:
    explicit OverlaySettingsView(ra::ui::viewmodels::OverlaySettingsViewModel& vmSettings) noexcept
        : ImGuiWindowView(vmSettings), m_vmSettings(vmSettings)
    {
    }

    void RenderContents() override;

private:
    ra::ui::viewmodels::OverlaySettingsViewModel& m_vmSettings;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_OVERLAY_SETTINGS_VIEW_HH
