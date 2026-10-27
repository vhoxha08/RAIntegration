#ifndef RA_UI_IMGUI_CONTROLS_HH
#define RA_UI_IMGUI_CONTROLS_HH
#pragma once

#include "ui/viewmodels/LookupItemViewModel.hh"

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// Drop down list of a lookup collection (the counterpart of the Win32 ComboBoxBinding).
/// </summary>
/// <returns><c>true</c> if the user picked a different item; <paramref name="nSelectedId" /> is updated.</returns>
bool LookupCombo(const char* sId, const ra::ui::viewmodels::LookupItemViewModelCollection& vItems, int& nSelectedId);

/// <summary>
/// Whether Escape was pressed while the current window (or one of its children) has focus.
/// </summary>
bool IsEscapePressed();

/// <summary>
/// Moves the cursor so <paramref name="nButtons" /> buttons of <paramref name="fButtonWidth" /> end at the right edge
/// (but not left of the current position).
/// </summary>
void AlignButtonsRight(int nButtons, float fButtonWidth);

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_CONTROLS_HH
