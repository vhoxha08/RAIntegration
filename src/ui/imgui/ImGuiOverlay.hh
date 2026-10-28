#ifndef RA_UI_IMGUI_OVERLAY_HH
#define RA_UI_IMGUI_OVERLAY_HH
#pragma once

#include <functional>

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// Draws the overlay (popups, scoreboards, the in-game overlay) over the game area of the host's ImGui frame.
/// Must be called inside an ImGui frame, always from the same thread (the render thread).
/// </summary>
void RenderOverlay();

/// <summary>
/// Whether this is the thread that draws the overlay, which owns ImGui and the textures. It may not be the UI
/// thread (the host can render on its own thread). Until the first frame the UI thread is assumed.
/// </summary>
bool IsOnRenderThread();

/// <summary>
/// Runs fAction on the render thread: right away when called from it, otherwise at the start of the next frame.
/// </summary>
void RunOnRenderThread(std::function<void()> fAction);

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_OVERLAY_HH
