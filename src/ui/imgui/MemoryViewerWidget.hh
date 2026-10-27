#ifndef RA_UI_IMGUI_MEMORY_VIEWER_WIDGET_HH
#define RA_UI_IMGUI_MEMORY_VIEWER_WIDGET_HH
#pragma once

#include "ui/viewmodels/MemoryViewerViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// ImGui version of the Win32 MemoryViewerControl (and its MemoryViewerControlBinding).
/// </summary>
class MemoryViewerWidget
{
public:
    explicit MemoryViewerWidget(ra::ui::viewmodels::MemoryViewerViewModel& vmViewer) noexcept : m_vmViewer(vmViewer) {}
    ~MemoryViewerWidget() noexcept;
    MemoryViewerWidget(const MemoryViewerWidget&) noexcept = delete;
    MemoryViewerWidget& operator=(const MemoryViewerWidget&) noexcept = delete;
    MemoryViewerWidget(MemoryViewerWidget&&) noexcept = delete;
    MemoryViewerWidget& operator=(MemoryViewerWidget&&) noexcept = delete;

    /// <summary>
    /// Draws the viewer filling the available width and <paramref name="fHeight" /> pixels high.
    /// </summary>
    void Render(float fHeight);

private:
    void SetFocused(bool bFocused);
    bool HandleNavigation();
    bool HandleShortcut();

    ra::ui::viewmodels::MemoryViewerViewModel& m_vmViewer;
    int m_nWidth = 0;
    int m_nHeight = 0;
    bool m_bFocused = false;
};

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_MEMORY_VIEWER_WIDGET_HH
