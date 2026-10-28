#ifndef RA_UI_WX_SMALL_DIALOGS_HH
#define RA_UI_WX_SMALL_DIALOGS_HH
#pragma once

#include "ui/viewmodels/MemoryNotesViewModel.hh"
#include "ui/viewmodels/MemoryRegionsViewModel.hh"
#include "ui/viewmodels/NewAssetViewModel.hh"
#include "ui/viewmodels/ProgressViewModel.hh"
#include "ui/viewmodels/TriggerSummaryViewModel.hh"
#include "ui/wx/WxDialog.hh"

class wxGauge;

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// wxWidgets version of the Win32 MemoryNotesDialog (the code notes list).
/// </summary>
class MemoryNotesDialog : public WxDialog
{
public:
    explicit MemoryNotesDialog(ra::ui::viewmodels::MemoryNotesViewModel& vmMemoryNotes);

protected:
    void UpdateControls() override { m_pBinder.Update(); }

private:
    Binder m_pBinder;
};

/// <summary>
/// wxWidgets version of the Win32 MemoryRegionsDialog.
/// </summary>
class MemoryRegionsDialog : public WxDialog
{
public:
    explicit MemoryRegionsDialog(ra::ui::viewmodels::MemoryRegionsViewModel& vmMemoryRegions);

protected:
    void UpdateControls() override { m_pBinder.Update(); }

private:
    Binder m_pBinder;
};

/// <summary>
/// wxWidgets version of the Win32 NewAssetDialog.
/// </summary>
class NewAssetDialog : public WxDialog
{
public:
    explicit NewAssetDialog(ra::ui::viewmodels::NewAssetViewModel& vmNewAsset);
};

/// <summary>
/// wxWidgets version of the Win32 ProgressDialog.
/// </summary>
class ProgressDialog : public WxDialog
{
public:
    explicit ProgressDialog(ra::ui::viewmodels::ProgressViewModel& vmProgress);

protected:
    void UpdateControls() override;

private:
    Binder m_pBinder;
    ra::ui::viewmodels::ProgressViewModel& m_vmProgress;
    wxGauge* m_pGauge = nullptr;
};

/// <summary>
/// wxWidgets version of the Win32 TriggerSummaryDialog.
/// </summary>
class TriggerSummaryDialog : public WxDialog
{
public:
    explicit TriggerSummaryDialog(ra::ui::viewmodels::TriggerSummaryViewModel& vmTriggerSummary);
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_SMALL_DIALOGS_HH
