#ifndef RA_UI_IMGUI_WINDOW_VIEW_HH
#define RA_UI_IMGUI_WINDOW_VIEW_HH
#pragma once

#include "ui/WindowViewModelBase.hh"

#include <imgui.h>

#include <functional>
#include <string>

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// Dear ImGui view of a WindowViewModelBase (the counterpart of a Win32 dialog class).
/// </summary>
class ImGuiWindowView
{
public:
    explicit ImGuiWindowView(WindowViewModelBase& vmWindow) noexcept : m_vmWindow(vmWindow) {}
    virtual ~ImGuiWindowView() noexcept = default;
    ImGuiWindowView(const ImGuiWindowView&) noexcept = delete;
    ImGuiWindowView& operator=(const ImGuiWindowView&) noexcept = delete;
    ImGuiWindowView(ImGuiWindowView&&) noexcept = delete;
    ImGuiWindowView& operator=(ImGuiWindowView&&) noexcept = delete;

    /// <summary>
    /// Draws the window contents (between ImGui::Begin and ImGui::End).
    /// </summary>
    virtual void RenderContents() = 0;

    /// <summary>
    /// Called when the user closes the window with its title bar button.
    /// </summary>
    virtual void OnCloseRequested() { Close(DialogResult::Cancel); }

    /// <summary>
    /// Initial size of a modeless window. Zero sizes the window to its contents.
    /// </summary>
    virtual ImVec2 GetInitialSize() const { return ImVec2(0.0f, 0.0f); }

    WindowViewModelBase& GetViewModel() noexcept { return m_vmWindow; }

    /// <summary>
    /// Whether the window was closed by the view, or by the view model setting a dialog result (like the
    /// Win32 dialogs, which close when the result changes).
    /// </summary>
    bool IsClosed() const { return m_bClosed || m_vmWindow.GetDialogResult() != DialogResult::None; }

protected:
    void Close(DialogResult nResult)
    {
        m_vmWindow.SetDialogResult(nResult);
        m_bClosed = true;
    }

    /// <summary>
    /// Runs an action after the current frame. Actions may show other (modal) windows, which must not happen
    /// while a frame is being built.
    /// </summary>
    static void RunAfterFrame(std::function<void()> fAction);

    WindowViewModelBase& m_vmWindow;

private:
    bool m_bClosed = false;
};

/// <summary>
/// Converts between the toolkit's wide strings and ImGui's UTF-8 strings.
/// </summary>
std::string ToUtf8(const std::wstring& sText);
std::wstring FromUtf8(const std::string& sText);

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_WINDOW_VIEW_HH
