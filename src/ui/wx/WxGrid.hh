#ifndef RA_UI_WX_GRID_HH
#define RA_UI_WX_GRID_HH
#pragma once

#include "ui/ViewModelCollection.hh"
#include "ui/viewmodels/LookupItemViewModel.hh"

#include <wx/dataview.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// A column of a Grid (the counterpart of the Win32 GridColumnBinding classes).
/// </summary>
struct GridColumn
{
    enum class Kind
    {
        Text,   // shown as text, edited with a text box
        Check,  // a check box bound to a bool property
        Choice, // shown as the label of a lookup item, edited by picking from a menu
    };

    std::wstring sHeader;
    int nWidth = 80; // DIPs; 0 = take the remaining width
    wxAlignment nAlignment = wxALIGN_LEFT;
    Kind nKind = Kind::Text;

    // Text: the displayed text and (if editable) how to apply an edit; returns false to reject it
    std::function<std::wstring(const ViewModelCollectionBase&, gsl::index)> fGetText;
    std::function<bool(ViewModelCollectionBase&, gsl::index, const std::wstring&)> fSetText;

    // Check
    const BoolModelProperty* pCheckProperty = nullptr;

    // Choice
    const IntModelProperty* pChoiceProperty = nullptr;
    const ra::ui::viewmodels::LookupItemViewModelCollection* pChoices = nullptr;
    std::function<void(ViewModelCollectionBase&, gsl::index, int)> fSetChoice; // optional: instead of the property

    // optional: whether a row can be edited, text color per row (ARGB, alpha 0 = default), tooltip
    std::function<bool(const ViewModelCollectionBase&, gsl::index)> fIsEditable;
    std::function<std::wstring(const ViewModelCollectionBase&, gsl::index)> fGetTooltip;
    const IntModelProperty* pTextColorProperty = nullptr;

    bool IsReadOnly() const noexcept
    {
        return (nKind == Kind::Text) ? !fSetText : false;
    }

    // factories for the common bindings
    static GridColumn Text(const std::wstring& sHeader, const StringModelProperty& pProperty, int nWidth,
                           bool bEditable = false);
    static GridColumn Number(const std::wstring& sHeader, const IntModelProperty& pProperty, int nWidth,
                             bool bEditable = false);
    static GridColumn Lookup(const std::wstring& sHeader, const IntModelProperty& pProperty,
                             const ra::ui::viewmodels::LookupItemViewModelCollection& vChoices, int nWidth,
                             bool bEditable = false);
    static GridColumn Check(const std::wstring& sHeader, const BoolModelProperty& pProperty, int nWidth = 24);
    static GridColumn BoolText(const std::wstring& sHeader, const BoolModelProperty& pProperty,
                               const std::wstring& sTrue, const std::wstring& sFalse, int nWidth);
    /// <summary>
    /// Tooltip showing a (possibly multi-line) text property, at most 20 lines (GridColumnBinding::BindTooltip).
    /// </summary>
    static std::function<std::wstring(const ViewModelCollectionBase&, gsl::index)> TooltipFrom(
        const StringModelProperty& pProperty);

    static GridColumn Custom(const std::wstring& sHeader, int nWidth,
                             std::function<std::wstring(const ViewModelCollectionBase&, gsl::index)> fGetText,
                             std::function<bool(ViewModelCollectionBase&, gsl::index, const std::wstring&)> fSetText = nullptr);
};

/// <summary>
/// A list/grid of the items of a view model collection (the counterpart of the Win32 GridBinding), shown with
/// wxDataViewCtrl (NSTableView on macOS).
/// </summary>
class Grid : public wxDataViewCtrl, protected ViewModelCollectionBase::NotifyTarget
{
public:
    Grid(wxWindow* pParent, long nStyle = wxDV_MULTIPLE | wxDV_ROW_LINES);
    ~Grid() override;
    Grid(const Grid&) noexcept = delete;
    Grid& operator=(const Grid&) noexcept = delete;
    Grid(Grid&&) noexcept = delete;
    Grid& operator=(Grid&&) noexcept = delete;

    void AddColumn(GridColumn&& pColumn);

    /// <summary>
    /// Shows the items of <paramref name="vmItems" />. Call after adding the columns.
    /// </summary>
    void BindItems(ViewModelCollectionBase& vmItems);

    /// <summary>
    /// Keeps the selection in sync with a bool property of the items.
    /// </summary>
    void BindIsSelected(const BoolModelProperty& pProperty) noexcept { m_pIsSelectedProperty = &pProperty; }

    /// <summary>
    /// Colors rows with an int (ARGB) property of the items (alpha 0 = default).
    /// </summary>
    void BindRowColor(const IntModelProperty& pProperty) noexcept { m_pRowColorProperty = &pProperty; }

    void SetDoubleClickHandler(std::function<void(gsl::index)> fHandler) { m_fDoubleClick = std::move(fHandler); }

    /// <summary>
    /// Called with the (sorted) selected indices when the user changes the selection, instead of setting the
    /// IsSelected property of the items (for view models that track the selection themselves).
    /// </summary>
    void SetSelectionHandler(std::function<void(const std::vector<gsl::index>&)> fHandler)
    {
        m_fSelectionChanged = std::move(fHandler);
    }

    /// <summary>
    /// Ctrl/Cmd+C and Ctrl/Cmd+V (the Win32 GridBinding's copy and paste handlers).
    /// </summary>
    void SetCopyHandler(std::function<void()> fHandler) { m_fCopy = std::move(fHandler); }
    void SetPasteHandler(std::function<void()> fHandler) { m_fPaste = std::move(fHandler); }

    /// <summary>
    /// The column F2 edits (like the Win32 GridBinding's primary edit column).
    /// </summary>
    void SetPrimaryEditColumn(unsigned int nColumn) noexcept { m_nPrimaryEditColumn = static_cast<int>(nColumn); }

    /// <summary>
    /// Called with the item index when the user right clicks (or Ctrl-clicks) an item.
    /// </summary>
    void SetContextMenuHandler(std::function<void(gsl::index)> fHandler) { m_fContextMenu = std::move(fHandler); }

    /// <summary>
    /// Index of the (first) selected item, or -1.
    /// </summary>
    gsl::index GetSelectedIndex() const;

    /// <summary>
    /// Refreshes all rows (for columns that depend on state outside of the items).
    /// </summary>
    void RefreshItems();

    /// <summary>
    /// Scrolls the item at <paramref name="nIndex" /> into view.
    /// </summary>
    void EnsureIndexVisible(gsl::index nIndex);

protected:
    void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs&) override;
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs&) override;
    void OnViewModelAdded(gsl::index) override;
    void OnViewModelRemoved(gsl::index) override;
    void OnViewModelChanged(gsl::index nIndex) override;
    void OnBeginViewModelCollectionUpdate() override;
    void OnEndViewModelCollectionUpdate() override;

private:
    class Model;
    friend class Model;

    void RowChanged(gsl::index nIndex);
    void Reset();
    void OnItemActivated(wxDataViewEvent& event);
    void ShowChoiceMenu(gsl::index nIndex, const GridColumn& pColumn);
    void OnCharHook(wxKeyEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    bool ScheduleResetIfNotMainThread();
    void SyncSelectionFromItems();
    void OnSelectionChanged();

    ViewModelCollectionBase* m_pItems = nullptr;
    Model* m_pModel = nullptr;
    std::vector<GridColumn> m_vColumns;
    const BoolModelProperty* m_pIsSelectedProperty = nullptr;
    const IntModelProperty* m_pRowColorProperty = nullptr;
    std::function<void(gsl::index)> m_fDoubleClick;
    std::function<void(const std::vector<gsl::index>&)> m_fSelectionChanged;
    std::function<void(gsl::index)> m_fContextMenu;
    std::function<void()> m_fCopy;
    std::function<void()> m_fPaste;
    int m_nPrimaryEditColumn = -1;
    gsl::index m_nTooltipRow = -1;
    int m_nTooltipColumn = -1;
    long m_nStyle = 0;
    int m_nUpdateDepth = 0;
    bool m_bResetPending = false;
    std::vector<gsl::index> m_vChangedDuringUpdate;
    bool m_bSelectionChangedDuringUpdate = false;
    bool m_bSyncingSelection = false;
    std::atomic<bool> m_bResetScheduled{false};
};

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_GRID_HH
