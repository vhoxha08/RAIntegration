#ifndef RA_UI_IMGUI_MESSAGE_BOX_VIEW_HH
#define RA_UI_IMGUI_MESSAGE_BOX_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/MessageBoxViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 MessageBoxDialog.
/// </summary>
class MessageBoxView : public ImGuiWindowView
{
public:
    explicit MessageBoxView(ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) noexcept;

    void RenderContents() override;
    void OnCloseRequested() override;

private:
    ra::ui::viewmodels::MessageBoxViewModel& m_vmMessageBox;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_MESSAGE_BOX_VIEW_HH
