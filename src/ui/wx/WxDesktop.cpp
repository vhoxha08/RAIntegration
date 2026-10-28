#include "WxDesktop.hh"

#include "ui/viewmodels/FileDialogViewModel.hh"
#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/wx/AssetEditorDialog.hh"
#include "ui/wx/AssetListDialog.hh"
#include "ui/wx/BrokenAchievementsDialog.hh"
#include "ui/wx/GameChecksumDialog.hh"
#include "ui/wx/LoginDialog.hh"
#include "ui/wx/MemoryBookmarksDialog.hh"
#include "ui/wx/MemoryInspectorDialog.hh"
#include "ui/wx/OverlaySettingsDialog.hh"
#include "ui/wx/PointerFinderDialog.hh"
#include "ui/wx/PointerInspectorDialog.hh"
#include "ui/wx/RichPresenceDialog.hh"
#include "ui/wx/SmallDialogs.hh"
#include "ui/wx/UnknownGameDialog.hh"
#include "ui/wx/WxApp.hh"
#include "ui/wx/WxDialog.hh"

#include <wx/app.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include <condition_variable>
#include <mutex>

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::FileDialogViewModel;
using ra::ui::viewmodels::MessageBoxViewModel;

WxDialog* WxDesktop::CreateDialog(WindowViewModelBase& vmViewModel)
{
    if (auto* vmLogin = dynamic_cast<ra::ui::viewmodels::LoginViewModel*>(&vmViewModel))
        return new LoginDialog(*vmLogin);

    if (auto* vmOverlaySettings = dynamic_cast<ra::ui::viewmodels::OverlaySettingsViewModel*>(&vmViewModel))
        return new OverlaySettingsDialog(*vmOverlaySettings);

    if (auto* vmChecksum = dynamic_cast<ra::ui::viewmodels::GameChecksumViewModel*>(&vmViewModel))
        return new GameChecksumDialog(*vmChecksum);

    if (auto* vmUnknownGame = dynamic_cast<ra::ui::viewmodels::UnknownGameViewModel*>(&vmViewModel))
        return new UnknownGameDialog(*vmUnknownGame);

    if (auto* vmRichPresence = dynamic_cast<ra::ui::viewmodels::RichPresenceMonitorViewModel*>(&vmViewModel))
        return new RichPresenceDialog(*vmRichPresence);

    if (auto* vmBroken = dynamic_cast<ra::ui::viewmodels::BrokenAchievementsViewModel*>(&vmViewModel))
        return new BrokenAchievementsDialog(*vmBroken);

    if (auto* vmMemoryInspector = dynamic_cast<ra::ui::viewmodels::MemoryInspectorViewModel*>(&vmViewModel))
        return new MemoryInspectorDialog(*vmMemoryInspector);

    if (auto* vmBookmarks = dynamic_cast<ra::ui::viewmodels::MemoryBookmarksViewModel*>(&vmViewModel))
        return new MemoryBookmarksDialog(*vmBookmarks);

    if (auto* vmPointerFinder = dynamic_cast<ra::ui::viewmodels::PointerFinderViewModel*>(&vmViewModel))
        return new PointerFinderDialog(*vmPointerFinder);

    if (auto* vmPointerInspector = dynamic_cast<ra::ui::viewmodels::PointerInspectorViewModel*>(&vmViewModel))
        return new PointerInspectorDialog(*vmPointerInspector);

    if (auto* vmAssetEditor = dynamic_cast<ra::ui::viewmodels::AssetEditorViewModel*>(&vmViewModel))
        return new AssetEditorDialog(*vmAssetEditor);

    if (auto* vmAssetList = dynamic_cast<ra::ui::viewmodels::AssetListViewModel*>(&vmViewModel))
        return new AssetListDialog(*vmAssetList);

    if (auto* vmMemoryNotes = dynamic_cast<ra::ui::viewmodels::MemoryNotesViewModel*>(&vmViewModel))
        return new MemoryNotesDialog(*vmMemoryNotes);

    if (auto* vmMemoryRegions = dynamic_cast<ra::ui::viewmodels::MemoryRegionsViewModel*>(&vmViewModel))
        return new MemoryRegionsDialog(*vmMemoryRegions);

    if (auto* vmNewAsset = dynamic_cast<ra::ui::viewmodels::NewAssetViewModel*>(&vmViewModel))
        return new NewAssetDialog(*vmNewAsset);

    if (auto* vmProgress = dynamic_cast<ra::ui::viewmodels::ProgressViewModel*>(&vmViewModel))
        return new ProgressDialog(*vmProgress);

    if (auto* vmTriggerSummary = dynamic_cast<ra::ui::viewmodels::TriggerSummaryViewModel*>(&vmViewModel))
        return new TriggerSummaryDialog(*vmTriggerSummary);

    return nullptr;
}

WxDialog* WxDesktop::FindWindow(const WindowViewModelBase& vmViewModel)
{
    for (auto* pWindow : wxTopLevelWindows)
    {
        auto* pDialog = dynamic_cast<WxDialog*>(pWindow);
        if (pDialog != nullptr && &pDialog->GetViewModel() == &vmViewModel && !pDialog->IsBeingDeleted())
            return pDialog;
    }

    return nullptr;
}

void WxDesktop::ShowWindow(WindowViewModelBase& vmViewModel) const
{
    if (!IsOnUIThread())
    {
        PostToUIThread([this, &vmViewModel]() { ShowWindow(vmViewModel); });
        return;
    }

    if (auto* pExisting = FindWindow(vmViewModel))
    {
        pExisting->Show();
        pExisting->Raise();
        return;
    }

    WxDialog* pDialog = EnsureWxInitialized() ? CreateDialog(vmViewModel) : nullptr;
    if (pDialog == nullptr)
    {
        HeadlessDesktop::ShowWindow(vmViewModel);
        return;
    }

    vmViewModel.SetDialogResult(ra::ui::DialogResult::None);
    pDialog->SetModeless();
    pDialog->Show();
    vmViewModel.SetIsVisible(true);
}

ra::ui::DialogResult WxDesktop::ShowModal(WindowViewModelBase& vmViewModel) const
{
    if (IsOnUIThread())
        return ShowModalOnUIThread(vmViewModel);

    // like the Win32 desktop: show it on the UI thread and wait for it to be closed
    std::mutex mutex;
    std::condition_variable condition;
    bool bDone = false;
    ra::ui::DialogResult nResult = ra::ui::DialogResult::None;

    PostToUIThread([this, &vmViewModel, &mutex, &condition, &bDone, &nResult]() {
        const auto nModalResult = ShowModalOnUIThread(vmViewModel);

        std::lock_guard<std::mutex> lock(mutex);
        nResult = nModalResult;
        bDone = true;
        condition.notify_all();
    });

    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&bDone]() { return bDone; });
    return nResult;
}

ra::ui::DialogResult WxDesktop::ShowModal(WindowViewModelBase& vmViewModel,
                                          [[maybe_unused]] const WindowViewModelBase& vmParentWindow) const
{
    return ShowModal(vmViewModel);
}

ra::ui::DialogResult WxDesktop::ShowModalOnUIThread(WindowViewModelBase& vmViewModel) const
{
    if (!EnsureWxInitialized())
        return HeadlessDesktop::ShowModal(vmViewModel);

    if (dynamic_cast<MessageBoxViewModel*>(&vmViewModel) != nullptr)
        return ShowMessageBox(vmViewModel);

    if (dynamic_cast<FileDialogViewModel*>(&vmViewModel) != nullptr)
        return ShowFileDialog(vmViewModel);

    WxDialog* pDialog = CreateDialog(vmViewModel);
    if (pDialog == nullptr)
        return HeadlessDesktop::ShowModal(vmViewModel);

    vmViewModel.SetDialogResult(ra::ui::DialogResult::None);
    vmViewModel.SetIsVisible(true);
    pDialog->CentreOnScreen();
    pDialog->ShowModal();
    vmViewModel.SetIsVisible(false);

    // not Destroy(): that defers the deletion to the next idle pass, and modal view models usually live on the
    // caller's stack, so they may be gone by then
    delete pDialog;

    return vmViewModel.GetDialogResult();
}

ra::ui::DialogResult WxDesktop::ShowMessageBox(WindowViewModelBase& vmViewModel)
{
    auto& vmMessageBox = dynamic_cast<MessageBoxViewModel&>(vmViewModel);

    long nStyle = wxCENTRE;
    switch (vmMessageBox.GetButtons())
    {
        case MessageBoxViewModel::Buttons::OK: nStyle |= wxOK; break;
        case MessageBoxViewModel::Buttons::OKCancel: nStyle |= wxOK | wxCANCEL; break;
        case MessageBoxViewModel::Buttons::YesNo: nStyle |= wxYES_NO; break;
        case MessageBoxViewModel::Buttons::YesNoCancel: nStyle |= wxYES_NO | wxCANCEL; break;
        case MessageBoxViewModel::Buttons::RetryCancel: nStyle |= wxOK | wxCANCEL; break;
    }

    switch (vmMessageBox.GetIcon())
    {
        case MessageBoxViewModel::Icon::Info: nStyle |= wxICON_INFORMATION; break;
        case MessageBoxViewModel::Icon::Warning: nStyle |= wxICON_WARNING; break;
        case MessageBoxViewModel::Icon::Error: nStyle |= wxICON_ERROR; break;
        default: nStyle |= wxICON_NONE; break;
    }

    // like the Win32 task dialog: the header is the main instruction, the message the details
    const auto& sHeader = vmMessageBox.GetHeader();
    const wxString sMain = ToWx(sHeader.empty() ? vmMessageBox.GetMessage() : sHeader);
    wxMessageDialog pDialog(nullptr, sMain, ToWx(vmMessageBox.GetWindowTitle()), nStyle);
    if (!sHeader.empty())
        pDialog.SetExtendedMessage(ToWx(vmMessageBox.GetMessage()));
    if (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::RetryCancel)
        pDialog.SetOKCancelLabels("Retry", wxID_CANCEL);

    ra::ui::DialogResult nResult = ra::ui::DialogResult::Cancel;
    switch (pDialog.ShowModal())
    {
        case wxID_OK:
            nResult = (vmMessageBox.GetButtons() == MessageBoxViewModel::Buttons::RetryCancel)
                          ? ra::ui::DialogResult::Retry
                          : ra::ui::DialogResult::OK;
            break;
        case wxID_YES: nResult = ra::ui::DialogResult::Yes; break;
        case wxID_NO: nResult = ra::ui::DialogResult::No; break;
        default: nResult = ra::ui::DialogResult::Cancel; break;
    }

    vmMessageBox.SetDialogResult(nResult);
    return nResult;
}

ra::ui::DialogResult WxDesktop::ShowFileDialog(WindowViewModelBase& vmViewModel)
{
    auto& vmFileDialog = dynamic_cast<FileDialogViewModel&>(vmViewModel);
    const wxString sTitle = ToWx(vmFileDialog.GetWindowTitle());
    const wxString sDirectory = ToWx(vmFileDialog.GetInitialDirectory());

    wxString sPath;
    if (vmFileDialog.GetMode() == FileDialogViewModel::Mode::Folder)
    {
        wxDirDialog pDialog(nullptr, sTitle, sDirectory, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (pDialog.ShowModal() == wxID_OK)
            sPath = pDialog.GetPath();
    }
    else
    {
        // file types are stored as "*.ext1;*.ext2", which is also wxWidgets' wildcard syntax
        wxString sWildcard;
        for (const auto& pType : vmFileDialog.GetFileTypes())
        {
            if (!sWildcard.empty())
                sWildcard += "|";
            sWildcard += ToWx(pType.first) + "|" + ToWx(pType.second);
        }
        if (sWildcard.empty())
            sWildcard = wxFileSelectorDefaultWildcardStr;

        long nStyle = 0;
        if (vmFileDialog.GetMode() == FileDialogViewModel::Mode::Save)
        {
            nStyle = wxFD_SAVE;
            if (vmFileDialog.GetOverwritePrompt())
                nStyle |= wxFD_OVERWRITE_PROMPT;
        }
        else
        {
            nStyle = wxFD_OPEN | wxFD_FILE_MUST_EXIST;
        }

        wxFileDialog pDialog(nullptr, sTitle, sDirectory, ToWx(vmFileDialog.GetFileName()), sWildcard, nStyle);
        if (pDialog.ShowModal() == wxID_OK)
            sPath = pDialog.GetPath();
    }

    if (sPath.empty())
    {
        vmFileDialog.SetDialogResult(ra::ui::DialogResult::Cancel);
        return ra::ui::DialogResult::Cancel;
    }

    vmFileDialog.SetFileName(FromWx(sPath));
    vmFileDialog.SetDialogResult(ra::ui::DialogResult::OK);
    return ra::ui::DialogResult::OK;
}

void WxDesktop::CloseWindow(WindowViewModelBase& vmViewModel) const
{
    if (!IsOnUIThread())
    {
        PostToUIThread([this, &vmViewModel]() { CloseWindow(vmViewModel); });
        return;
    }

    if (auto* pDialog = FindWindow(vmViewModel))
        pDialog->Close();
}

void WxDesktop::Shutdown()
{
    // destroy the toolkit's windows before the view models they show go away
    for (auto* pWindow : wxTopLevelWindows)
    {
        if (auto* pDialog = dynamic_cast<WxDialog*>(pWindow))
            pDialog->Destroy();
    }
    if (wxTheApp)
        wxTheApp->ProcessIdle(); // performs the delayed destruction

    HeadlessDesktop::Shutdown();
}

} // namespace wx
} // namespace ui
} // namespace ra
