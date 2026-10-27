#include "WxApp.hh"

#ifdef RA_WX_TEST_HOOKS
#include "ui/wx/WxTestHooks.hh"
#endif

#include <wx/app.h>
#include <wx/init.h>
#include <wx/string.h>

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

    wxTheApp->OnExit();
    wxEntryCleanup();
    s_nState = WxState::Stopped;
}

wxString ToWx(const std::wstring& sText) { return wxString(sText.c_str(), sText.length()); }

std::wstring FromWx(const wxString& sText) { return sText.ToStdWstring(); }

} // namespace wx
} // namespace ui
} // namespace ra
