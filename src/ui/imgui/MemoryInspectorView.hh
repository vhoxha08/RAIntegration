#ifndef RA_UI_IMGUI_MEMORY_INSPECTOR_VIEW_HH
#define RA_UI_IMGUI_MEMORY_INSPECTOR_VIEW_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/imgui/MemoryViewerWidget.hh"
#include "ui/viewmodels/MemoryInspectorViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 MemoryInspectorDialog.
/// </summary>
class MemoryInspectorView : public ImGuiWindowView
{
public:
    explicit MemoryInspectorView(ra::ui::viewmodels::MemoryInspectorViewModel& vmMemoryInspector) noexcept
        : ImGuiWindowView(vmMemoryInspector),
          m_vmMemoryInspector(vmMemoryInspector),
          m_pViewer(vmMemoryInspector.Viewer())
    {
    }

    void RenderContents() override;
    ImVec2 GetInitialSize() const override;

private:
    void RenderSearch();
    void RenderResults();
    void RenderNotes();
    void RenderViewerOptions();

    ra::ui::viewmodels::MemoryInspectorViewModel& m_vmMemoryInspector;
    MemoryViewerWidget m_pViewer;
    gsl::index m_nSelectionAnchor = -1;
    bool m_bNotesExpanded = false;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_MEMORY_INSPECTOR_VIEW_HH
