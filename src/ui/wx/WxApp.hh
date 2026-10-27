#ifndef RA_UI_WX_APP_HH
#define RA_UI_WX_APP_HH
#pragma once

#include <string>

class wxFont;
class wxString;

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// Starts wxWidgets inside the host application (the host owns the main loop). Safe to call repeatedly.
/// </summary>
/// <returns><c>false</c> if wxWidgets could not be initialized.</returns>
bool EnsureWxInitialized();

/// <summary>
/// Handles wxWidgets' queued and idle work. Called from the host's main loop (_RA_ProcessUIThreadActions);
/// the native events themselves are dispatched by the host's event pump.
/// </summary>
void PumpWxEvents();

/// <summary>
/// Shuts wxWidgets down. All toolkit windows must have been destroyed.
/// </summary>
void ShutdownWx();

/// <summary>
/// Converts between the toolkit's wide strings and wxString.
/// </summary>
wxString ToWx(const std::wstring& sText);

/// <summary>
/// The native font for a toolkit font (face name as passed to ISurface::LoadFont, size in pixels).
/// </summary>
const wxFont& GetSurfaceFont(const std::string& sFace, float fSize, bool bBold);
std::wstring FromWx(const wxString& sText);

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_APP_HH
