#ifndef RA_UI_IMGUI_OVERLAY_HH
#define RA_UI_IMGUI_OVERLAY_HH
#pragma once

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// Draws the overlay (popups, scoreboards, the in-game overlay) over the game area of the host's ImGui frame.
/// Must be called inside an ImGui frame on the UI thread.
/// </summary>
void RenderOverlay();

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_OVERLAY_HH
