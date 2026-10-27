#ifndef RA_UI_WX_MEMORY_INSPECTOR_DIALOG_HH
#define RA_UI_WX_MEMORY_INSPECTOR_DIALOG_HH
#pragma once

#include "ui/viewmodels/MemoryInspectorViewModel.hh"
#include "ui/wx/WxDialog.hh"

#include <wx/timer.h>

#include <array>

class wxButton;
class wxCheckBox;
class wxDataViewCtrl;
class wxRadioButton;
class wxStaticText;
class wxTextCtrl;

namespace ra {
namespace ui {
namespace wx {

class MemoryViewerControl;

/// <summary>
/// wxWidgets version of the Win32 MemoryInspectorDialog.
/// </summary>
class MemoryInspectorDialog : public WxDialog
{
public:
    explicit MemoryInspectorDialog(ra::ui::viewmodels::MemoryInspectorViewModel& vmMemoryInspector);
    ~MemoryInspectorDialog() override;
    MemoryInspectorDialog(const MemoryInspectorDialog&) noexcept = delete;
    MemoryInspectorDialog& operator=(const MemoryInspectorDialog&) noexcept = delete;
    MemoryInspectorDialog(MemoryInspectorDialog&&) noexcept = delete;
    MemoryInspectorDialog& operator=(MemoryInspectorDialog&&) noexcept = delete;

protected:
    void UpdateControls() override;

private:
    class ResultsModel;

    wxSizer* CreateSearchControls();
    wxSizer* CreateResultsControls();
    wxSizer* CreateNotesControls();
    wxSizer* CreateViewerControls();

    void UpdateResults();
    void SyncScrollOffset();
    void OnResultsSelectionChanged();

    ra::ui::viewmodels::MemoryInspectorViewModel& m_vmMemoryInspector;

    // search
    LookupChoice* m_pRanges = nullptr;
    wxTextCtrl* m_pRange = nullptr;
    LookupChoice* m_pSearchType = nullptr;
    wxCheckBox* m_pAligned = nullptr;
    wxButton* m_pNewSearch = nullptr;
    LookupChoice* m_pComparison = nullptr;
    LookupChoice* m_pValueType = nullptr;
    wxTextCtrl* m_pFilterValue = nullptr;
    wxButton* m_pFilterOnce = nullptr;
    wxButton* m_pContinuousFilter = nullptr;

    // results
    wxStaticText* m_pResultCount = nullptr;
    wxStaticText* m_pFilterSummary = nullptr;
    wxStaticText* m_pPage = nullptr;
    wxButton* m_pPreviousPage = nullptr;
    wxButton* m_pNextPage = nullptr;
    wxButton* m_pExclude = nullptr;
    wxButton* m_pBookmark = nullptr;
    wxButton* m_pExport = nullptr;
    wxButton* m_pImport = nullptr;
    wxDataViewCtrl* m_pResults = nullptr;
    ResultsModel* m_pResultsModel = nullptr;
    bool m_bSyncingSelection = false;

    // notes
    wxTextCtrl* m_pAddress = nullptr;
    wxTextCtrl* m_pNote = nullptr;
    wxButton* m_pPublish = nullptr;
    wxButton* m_pRevert = nullptr;

    // viewer
    std::array<wxRadioButton*, 4> m_vSizes{};
    std::array<wxStaticText*, 8> m_vBits{};
    wxStaticText* m_pBitsLabel = nullptr;
    MemoryViewerControl* m_pViewer = nullptr;

    wxTimer m_pScrollTimer;
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_MEMORY_INSPECTOR_DIALOG_HH
