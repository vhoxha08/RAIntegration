#include "WxApp.hh"

#ifdef RA_WX_TEST_HOOKS
#include "ui/wx/WxTestHooks.hh"
#endif

#include "services/IFileSystem.hh"
#include "services/ServiceLocator.hh"

#include "ui/EditorTheme.hh"
#include "ui/imgui/ImGuiSurface.hh"

#include <wx/app.h>
#include <wx/dcscreen.h>
#include <wx/font.h>
#include <wx/image.h>
#include <wx/init.h>
#include <wx/log.h>
#include <wx/settings.h>
#include <wx/string.h>

#include <map>
#include <tuple>

namespace ra {
namespace ui {
namespace wx {

namespace {

class ToolkitApp : public wxApp
{
public:
    // the host's command line is not for us
    bool OnInit() override { return true; }
};

enum class WxState
{
    NotStarted,
    Running,
    Failed,
    Stopped,
};

WxState s_nState = WxState::NotStarted;

} // namespace

bool EnsureWxInitialized()
{
    if (s_nState == WxState::Running)
        return true;
    if (s_nState != WxState::NotStarted)
        return false;

#ifdef __WXOSX__
    // the host (SDL) already owns NSApplication and its event loop: run as a "plugin" so wxWidgets doesn't
    // replace the application delegate or try to run the application itself
    wxApp::sm_isEmbedded = true;
#endif

    wxApp::SetInstance(new ToolkitApp());

    static int s_nArgc = 1;
    static char s_sArg0[] = "RAIntegration";
    static char* s_pArgv[] = {s_sArg0, nullptr};
    if (!wxEntryStart(s_nArgc, s_pArgv) || !wxTheApp->CallOnInit())
    {
        s_nState = WxState::Failed;
        return false;
    }

    s_nState = WxState::Running;

    // badges and other images are PNG/JPEG files
    wxInitAllImageHandlers();

    // the editor colors default to a light theme; follow the system's dark mode unless they come from a file
    if (wxSystemSettings::GetAppearance().IsDark() && ra::services::ServiceLocator::Exists<ra::ui::EditorTheme>())
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        if (pFileSystem.GetFileSize(pFileSystem.BaseDirectory() + L"Overlay\\editor_theme.json") < 0 &&
            pFileSystem.GetFileSize(pFileSystem.BaseDirectory() + L"Overlay/editor_theme.json") < 0)
        {
            ra::services::ServiceLocator::GetMutable<ra::ui::EditorTheme>().ApplyDarkColors();
        }
    }

    // the toolkit reports problems itself; wxWidgets' own messages go to the log instead of a dialog
    delete wxLog::SetActiveTarget(new wxLogStderr());

    // surfaces painted by native windows (i.e. the memory viewer) lay out text with the native fonts
    ra::ui::imgui::ImGuiSurface::SetTextMeasurer(
        [](const std::string& sFace, float fSize, bool bBold, const std::wstring& sText) {
            wxScreenDC dc;
            dc.SetFont(GetSurfaceFont(sFace, fSize, bBold));
            wxCoord nWidth = 0, nHeight = 0;
            dc.GetTextExtent(ToWx(sText), &nWidth, &nHeight);
            return ra::ui::Size{nWidth, nHeight};
        });

#ifdef RA_WX_TEST_HOOKS
    StartTestHooks();
#endif

    return true;
}

void PumpWxEvents()
{
    if (s_nState != WxState::Running || wxTheApp == nullptr)
        return;

    wxTheApp->ProcessPendingEvents();
    wxTheApp->ProcessIdle();
}

void ShutdownWx()
{
    if (s_nState != WxState::Running)
        return;

    ra::ui::imgui::ImGuiSurface::SetTextMeasurer(nullptr);
    wxTheApp->OnExit();
    wxEntryCleanup();
    s_nState = WxState::Stopped;
}

const wxFont& GetSurfaceFont(const std::string& sFace, float fSize, bool bBold)
{
    static std::map<std::tuple<std::string, int, bool>, wxFont> s_mFonts;

    const auto pKey = std::make_tuple(sFace, static_cast<int>(fSize), bBold);
    auto pIter = s_mFonts.find(pKey);
    if (pIter != s_mFonts.end())
        return pIter->second;

    // the toolkit asks for Windows fonts; use the platform's equivalents
    const bool bMonospace = sFace == "Consolas" || sFace.find("Courier") != std::string::npos ||
                            sFace.find("Mono") != std::string::npos;
    wxFontInfo pInfo(wxSize(0, static_cast<int>(fSize)));
    pInfo.Family(bMonospace ? wxFONTFAMILY_TELETYPE : wxFONTFAMILY_SWISS).Bold(bBold);

    return s_mFonts.emplace(pKey, wxFont(pInfo)).first->second;
}

wxString ToWx(const std::wstring& sText) { return wxString(sText.c_str(), sText.length()); }

std::wstring FromWx(const wxString& sText) { return sText.ToStdWstring(); }

} // namespace wx
} // namespace ui
} // namespace ra
