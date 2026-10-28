#ifndef RA_UI_WX_TEST_HOOKS_HH
#define RA_UI_WX_TEST_HOOKS_HH
#pragma once

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// Test automation for the native windows (RA_WX_TEST_HOOKS builds only). When RA_WX_TEST_SCRIPT is set, runs its
/// steps on a 100ms timer: "tick action args;..." with actions
///   capture prefix     saves each visible window of the process to prefix_N.png
///   click label        presses the button titled label in the frontmost window
///   type text          sets the text of the focused text field
///   selectrow n [t]    selects row n of the first table (list) in a visible window (whose title starts with t)
///   list               logs the visible windows and their buttons
/// </summary>
void StartTestHooks();

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_TEST_HOOKS_HH
