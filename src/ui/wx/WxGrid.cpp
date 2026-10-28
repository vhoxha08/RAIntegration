#include "WxGrid.hh"

#include "ui/wx/WxApp.hh"
#include "ui/wx/WxBinding.hh"

#include <wx/app.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/thread.h>

#include <algorithm>

namespace ra {
namespace ui {
namespace wx {

// ---- columns ---------------------------------------------------------------

GridColumn GridColumn::Text(const std::wstring& sHeader, const StringModelProperty& pProperty, int nWidth,
                            bool bEditable)
{
    GridColumn pColumn;
    pColumn.sHeader = sHeader;
    pColumn.nWidth = nWidth;
    pColumn.fGetText = [&pProperty](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        return vItems.GetItemValue(nIndex, pProperty);
    };
    if (bEditable)
    {
        pColumn.fSetText = [&pProperty](ViewModelCollectionBase& vItems, gsl::index nIndex, const std::wstring& sValue) {
            vItems.SetItemValue(nIndex, pProperty, sValue);
            return true;
        };
    }
    return pColumn;
}

GridColumn GridColumn::Number(const std::wstring& sHeader, const IntModelProperty& pProperty, int nWidth,
                              bool bEditable)
{
    GridColumn pColumn;
    pColumn.sHeader = sHeader;
    pColumn.nWidth = nWidth;
    pColumn.nAlignment = wxALIGN_RIGHT;
    pColumn.fGetText = [&pProperty](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        return std::to_wstring(vItems.GetItemValue(nIndex, pProperty));
    };
    if (bEditable)
    {
        pColumn.fSetText = [&pProperty](ViewModelCollectionBase& vItems, gsl::index nIndex, const std::wstring& sValue) {
            wchar_t* pEnd = nullptr;
            const long nValue = std::wcstol(sValue.c_str(), &pEnd, 10);
            if (pEnd == sValue.c_str() || *pEnd != L'\0')
                return false;
            vItems.SetItemValue(nIndex, pProperty, gsl::narrow_cast<int>(nValue));
            return true;
        };
    }
    return pColumn;
}

GridColumn GridColumn::Lookup(const std::wstring& sHeader, const IntModelProperty& pProperty,
                              const ra::ui::viewmodels::LookupItemViewModelCollection& vChoices, int nWidth,
                              bool bEditable)
{
    GridColumn pColumn;
    pColumn.sHeader = sHeader;
    pColumn.nWidth = nWidth;
    pColumn.fGetText = [&pProperty, &vChoices](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        return vChoices.GetLabelForId(vItems.GetItemValue(nIndex, pProperty));
    };
    if (bEditable)
    {
        pColumn.nKind = Kind::Choice;
        pColumn.pChoiceProperty = &pProperty;
        pColumn.pChoices = &vChoices;
    }
    return pColumn;
}

GridColumn GridColumn::Check(const std::wstring& sHeader, const BoolModelProperty& pProperty, int nWidth)
{
    GridColumn pColumn;
    pColumn.sHeader = sHeader;
    pColumn.nWidth = nWidth;
    pColumn.nKind = Kind::Check;
    pColumn.nAlignment = wxALIGN_CENTER;
    pColumn.pCheckProperty = &pProperty;
    return pColumn;
}

GridColumn GridColumn::BoolText(const std::wstring& sHeader, const BoolModelProperty& pProperty,
                                const std::wstring& sTrue, const std::wstring& sFalse, int nWidth)
{
    GridColumn pColumn;
    pColumn.sHeader = sHeader;
    pColumn.nWidth = nWidth;
    pColumn.fGetText = [&pProperty, sTrue, sFalse](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        return vItems.GetItemValue(nIndex, pProperty) ? sTrue : sFalse;
    };
    return pColumn;
}

std::function<std::wstring(const ViewModelCollectionBase&, gsl::index)> GridColumn::TooltipFrom(
    const StringModelProperty& pProperty)
{
    return [&pProperty](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        std::wstring sTooltip = vItems.GetItemValue(nIndex, pProperty);

        // like the Win32 grid: show the first 20 lines of long texts
        size_t nCount = 0;
        size_t nTruncateIndex = 0;
        auto nScan = sTooltip.find('\n');
        while (nScan != std::wstring::npos)
        {
            if (++nCount == 20)
            {
                nTruncateIndex = nScan;
            }
            else if (nCount == 25)
            {
                sTooltip.resize(nTruncateIndex);
                sTooltip.append(L"\n...");
                break;
            }
            nScan = sTooltip.find('\n', nScan + 1);
        }

        return sTooltip;
    };
}

GridColumn GridColumn::Custom(const std::wstring& sHeader, int nWidth,
                              std::function<std::wstring(const ViewModelCollectionBase&, gsl::index)> fGetText,
                              std::function<bool(ViewModelCollectionBase&, gsl::index, const std::wstring&)> fSetText)
{
    GridColumn pColumn;
    pColumn.sHeader = sHeader;
    pColumn.nWidth = nWidth;
    pColumn.fGetText = std::move(fGetText);
    pColumn.fSetText = std::move(fSetText);
    return pColumn;
}

// ---- model -----------------------------------------------------------------

class Grid::Model : public wxDataViewVirtualListModel
{
public:
    explicit Model(Grid& pGrid) : m_pGrid(pGrid) {}

    unsigned int GetColumnCount() const override { return gsl::narrow_cast<unsigned int>(m_pGrid.m_vColumns.size()); }

    wxString GetColumnType(unsigned int nColumn) const override
    {
        return (m_pGrid.m_vColumns.at(nColumn).nKind == GridColumn::Kind::Check) ? "bool" : "string";
    }

    bool IsValidRow(unsigned int nRow) const
    {
        return m_pGrid.m_pItems != nullptr && nRow < m_pGrid.m_pItems->Count();
    }

    void GetValueByRow(wxVariant& vValue, unsigned int nRow, unsigned int nColumn) const override
    {
        const auto& pColumn = m_pGrid.m_vColumns.at(nColumn);
        if (!IsValidRow(nRow))
        {
            vValue = (pColumn.nKind == GridColumn::Kind::Check) ? wxVariant(false) : wxVariant(wxString());
            return;
        }

        const auto& vItems = *m_pGrid.m_pItems;
        switch (pColumn.nKind)
        {
            case GridColumn::Kind::Check:
                vValue = vItems.GetItemValue(nRow, *pColumn.pCheckProperty);
                break;

            case GridColumn::Kind::Choice:
                vValue = ToWx(pColumn.pChoices->GetLabelForId(vItems.GetItemValue(nRow, *pColumn.pChoiceProperty)));
                break;

            default:
                vValue = ToWx(pColumn.fGetText ? pColumn.fGetText(vItems, nRow) : std::wstring());
                break;
        }
    }

    bool SetValueByRow(const wxVariant& vValue, unsigned int nRow, unsigned int nColumn) override
    {
        const auto& pColumn = m_pGrid.m_vColumns.at(nColumn);
        if (!IsValidRow(nRow))
            return false;

        auto& vItems = *m_pGrid.m_pItems;
        switch (pColumn.nKind)
        {
            case GridColumn::Kind::Check:
                vItems.SetItemValue(nRow, *pColumn.pCheckProperty, vValue.GetBool());
                return true;

            case GridColumn::Kind::Choice:
            {
                const std::wstring sLabel = FromWx(vValue.GetString());
                for (const auto& pChoice : *pColumn.pChoices)
                {
                    if (pChoice.GetLabel() == sLabel)
                    {
                        if (pColumn.fSetChoice)
                            pColumn.fSetChoice(vItems, nRow, pChoice.GetId());
                        else
                            vItems.SetItemValue(nRow, *pColumn.pChoiceProperty, pChoice.GetId());
                        return true;
                    }
                }
                return false;
            }

            default:
            {
                if (!pColumn.fSetText)
                    return false;

                // the setter may show a message box; don't do that inside the editor's event
                const std::wstring sValue = FromWx(vValue.GetString());
                auto fSetText = pColumn.fSetText;
                auto* pItems = m_pGrid.m_pItems;
                m_pGrid.CallAfter([this, fSetText, pItems, nRow, sValue]() {
                    if (m_pGrid.m_pItems == pItems && nRow < pItems->Count() && !fSetText(*pItems, nRow, sValue))
                        RowChanged(nRow); // rejected: show the current value again
                });
                return true;
            }
        }
    }

    bool IsEnabledByRow(unsigned int nRow, unsigned int nColumn) const override
    {
        const auto& pColumn = m_pGrid.m_vColumns.at(nColumn);
        if (!pColumn.fIsEditable || !IsValidRow(nRow))
            return true;

        return pColumn.fIsEditable(*m_pGrid.m_pItems, nRow);
    }

    bool GetAttrByRow(unsigned int nRow, unsigned int nColumn, wxDataViewItemAttr& pAttr) const override
    {
        if (!IsValidRow(nRow))
            return false;

        const auto& vItems = *m_pGrid.m_pItems;
        auto ToColour = [](uint32_t nColor) {
            return wxColour((nColor >> 16) & 0xFF, (nColor >> 8) & 0xFF, nColor & 0xFF);
        };

        // the toolkit's colors are meant for light lists; keep the text readable in dark mode too
        wxColour pBackground = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX);
        wxColour pText = wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT);
        bool bChanged = false;
        if (m_pGrid.m_pRowColorProperty)
        {
            const auto nColor = ra::to_unsigned(vItems.GetItemValue(nRow, *m_pGrid.m_pRowColorProperty));
            if ((nColor >> 24) != 0)
            {
                pBackground = ToColour(nColor);
                pAttr.SetBackgroundColour(pBackground);
                bChanged = true;
            }
        }

        const auto& pColumn = m_pGrid.m_vColumns.at(nColumn);
        if (pColumn.pTextColorProperty)
        {
            const auto nColor = ra::to_unsigned(vItems.GetItemValue(nRow, *pColumn.pTextColorProperty));
            if ((nColor >> 24) != 0)
            {
                pText = ToColour(nColor);
                bChanged = true;
            }
        }

        if (bChanged)
            pAttr.SetColour(ReadableTextColour(pText, pBackground));

        return bChanged;
    }

private:
    Grid& m_pGrid;
};

// ---- grid ------------------------------------------------------------------

Grid::Grid(wxWindow* pParent, long nStyle) :
    wxDataViewCtrl(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, nStyle),
    m_nStyle(nStyle)
{
    Bind(wxEVT_CHAR_HOOK, &Grid::OnCharHook, this);

    // tooltips per cell: the native list has none, so follow the mouse and set the control's tooltip
    for (wxWindow* pWindow : {static_cast<wxWindow*>(this), GetMainWindow()})
    {
        if (pWindow == nullptr)
            continue;
        pWindow->Bind(wxEVT_MOTION, &Grid::OnMouseMove, this);
        pWindow->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
            m_nTooltipRow = -1;
            m_nTooltipColumn = -1;
            UnsetToolTip();
            event.Skip();
        });
    }

    m_pModel = new Model(*this);
    AssociateModel(m_pModel);
    m_pModel->DecRef(); // the control holds the reference

    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxDataViewEvent&) { OnSelectionChanged(); });
    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [this](wxDataViewEvent& event) {
        if (m_fContextMenu && event.GetItem().IsOk())
            m_fContextMenu(gsl::narrow_cast<gsl::index>(m_pModel->GetRow(event.GetItem())));
    });
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &Grid::OnItemActivated, this);
}

Grid::~Grid()
{
    if (m_pItems != nullptr)
    {
        try
        {
            m_pItems->RemoveNotifyTarget(*this);
        }
        catch (...)
        {
        }
    }
}

void Grid::AddColumn(GridColumn&& pColumn)
{
    const auto nIndex = gsl::narrow_cast<unsigned int>(m_vColumns.size());
    const int nWidth = (pColumn.nWidth > 0) ? FromDIP(pColumn.nWidth) : wxCOL_WIDTH_AUTOSIZE;
    const int nFlags = wxDATAVIEW_COL_RESIZABLE;

    wxDataViewRenderer* pRenderer = nullptr;
    switch (pColumn.nKind)
    {
        case GridColumn::Kind::Check:
            pRenderer = new wxDataViewToggleRenderer("bool", wxDATAVIEW_CELL_ACTIVATABLE, pColumn.nAlignment);
            break;

        case GridColumn::Kind::Choice:
            // drawn as text: native pop-up button cells are expensive to draw (macOS renders each one through
            // SwiftUI), and rows with changing values are redrawn every frame. Activating the cell shows a menu.
            pRenderer = new wxDataViewTextRenderer("string", wxDATAVIEW_CELL_INERT, pColumn.nAlignment);
            break;

        default:
            pRenderer = new wxDataViewTextRenderer("string",
                                                   pColumn.IsReadOnly() ? wxDATAVIEW_CELL_INERT : wxDATAVIEW_CELL_EDITABLE,
                                                   pColumn.nAlignment);
            break;
    }

    AppendColumn(new wxDataViewColumn(ToWx(pColumn.sHeader), pRenderer, nIndex, nWidth, pColumn.nAlignment, nFlags));
    m_vColumns.push_back(std::move(pColumn));
}

void Grid::BindItems(ViewModelCollectionBase& vmItems)
{
    if (m_pItems != nullptr)
        m_pItems->RemoveNotifyTarget(*this);

    m_pItems = &vmItems;
    m_pItems->AddNotifyTarget(*this);
    Reset();
}

void Grid::OnItemActivated(wxDataViewEvent& event)
{
    if (!event.GetItem().IsOk() || m_pItems == nullptr)
        return;

    const auto nIndex = gsl::narrow_cast<gsl::index>(m_pModel->GetRow(event.GetItem()));
    const auto* pDataViewColumn = event.GetDataViewColumn();
    if (pDataViewColumn != nullptr)
    {
        const auto nColumn = pDataViewColumn->GetModelColumn();
        if (nColumn < m_vColumns.size())
        {
            const auto& pColumn = m_vColumns.at(nColumn);
            if (pColumn.nKind == GridColumn::Kind::Choice &&
                (!pColumn.fIsEditable || pColumn.fIsEditable(*m_pItems, nIndex)))
            {
                ShowChoiceMenu(nIndex, pColumn);
                return;
            }
        }
    }

    if (m_fDoubleClick)
        m_fDoubleClick(nIndex);
}

void Grid::ShowChoiceMenu(gsl::index nIndex, const GridColumn& pColumn)
{
    const int nCurrent = m_pItems->GetItemValue(nIndex, *pColumn.pChoiceProperty);

    wxMenu pMenu;
    std::vector<int> vIds;
    for (const auto& pChoice : *pColumn.pChoices)
    {
        const int nMenuId = wxID_HIGHEST + 1 + gsl::narrow_cast<int>(vIds.size());
        pMenu.AppendCheckItem(nMenuId, ToWx(pChoice.GetLabel()))->Check(pChoice.GetId() == nCurrent);
        vIds.push_back(pChoice.GetId());
    }

    const int nSelection = GetPopupMenuSelectionFromUser(pMenu);
    const int nChoice = nSelection - (wxID_HIGHEST + 1);
    if (nChoice < 0 || gsl::narrow_cast<size_t>(nChoice) >= vIds.size() || m_pItems == nullptr ||
        nIndex >= gsl::narrow_cast<gsl::index>(m_pItems->Count()))
    {
        return;
    }

    const int nValue = vIds.at(gsl::narrow_cast<size_t>(nChoice));
    if (pColumn.fSetChoice)
        pColumn.fSetChoice(*m_pItems, nIndex, nValue);
    else
        m_pItems->SetItemValue(nIndex, *pColumn.pChoiceProperty, nValue);
}

void Grid::OnMouseMove(wxMouseEvent& event)
{
    event.Skip();
    if (m_pItems == nullptr)
        return;

    wxDataViewItem pItem;
    wxDataViewColumn* pColumn = nullptr;
    wxPoint pPosition = event.GetPosition();
    if (event.GetEventObject() != this)
        pPosition = ScreenToClient(static_cast<wxWindow*>(event.GetEventObject())->ClientToScreen(pPosition));
    HitTest(pPosition, pItem, pColumn);

    gsl::index nRow = -1;
    int nColumn = -1;
    if (pItem.IsOk() && pColumn != nullptr)
    {
        nRow = gsl::narrow_cast<gsl::index>(m_pModel->GetRow(pItem));
        nColumn = gsl::narrow_cast<int>(pColumn->GetModelColumn());
    }

    if (nRow == m_nTooltipRow && nColumn == m_nTooltipColumn)
        return;

    m_nTooltipRow = nRow;
    m_nTooltipColumn = nColumn;

    std::wstring sTooltip;
    if (nRow >= 0 && nColumn >= 0 && nColumn < gsl::narrow_cast<int>(m_vColumns.size()) &&
        nRow < gsl::narrow_cast<gsl::index>(m_pItems->Count()))
    {
        const auto& pGridColumn = m_vColumns.at(gsl::narrow_cast<size_t>(nColumn));
        if (pGridColumn.fGetTooltip)
            sTooltip = pGridColumn.fGetTooltip(*m_pItems, nRow);
    }

    if (sTooltip.empty())
        UnsetToolTip();
    else
        SetToolTip(ToWx(sTooltip));
}

void Grid::OnCharHook(wxKeyEvent& event)
{
    // the Win32 GridBinding's keys (Ctrl on Windows and Linux, Cmd on macOS)
    const int nKey = event.GetKeyCode();
    if (event.ControlDown() && !event.AltDown() && !event.ShiftDown())
    {
        switch (nKey)
        {
            case 'A': // select all
                if ((m_nStyle & wxDV_MULTIPLE) != 0)
                {
                    SelectAll();
                    OnSelectionChanged(); // programmatic changes don't raise the event
                }
                return;

            case 'D': // deselect all
                UnselectAll();
                OnSelectionChanged();
                return;

            case 'C':
                if (m_fCopy)
                {
                    m_fCopy();
                    return;
                }
                break;

            case 'V':
                if (m_fPaste)
                {
                    m_fPaste();
                    return;
                }
                break;
        }
    }
    else if (!event.HasAnyModifiers())
    {
        const auto nIndex = GetSelectedIndex();
        if ((nKey == WXK_RETURN || nKey == WXK_NUMPAD_ENTER) && m_fDoubleClick && nIndex >= 0)
        {
            m_fDoubleClick(nIndex);
            return;
        }

        if (nKey == WXK_F2 && nIndex >= 0 && m_nPrimaryEditColumn >= 0 &&
            m_nPrimaryEditColumn < gsl::narrow_cast<int>(m_vColumns.size()) &&
            !m_vColumns.at(gsl::narrow_cast<size_t>(m_nPrimaryEditColumn)).IsReadOnly())
        {
            EditItem(m_pModel->GetItem(gsl::narrow_cast<unsigned int>(nIndex)),
                     GetColumn(gsl::narrow_cast<unsigned int>(m_nPrimaryEditColumn)));
            return;
        }
    }

    event.Skip();
}

gsl::index Grid::GetSelectedIndex() const
{
    const wxDataViewItem pItem = GetSelection();
    return pItem.IsOk() ? gsl::narrow_cast<gsl::index>(m_pModel->GetRow(pItem)) : -1;
}

void Grid::RefreshItems()
{
    if (m_pItems == nullptr)
        return;

    for (unsigned int nRow = 0; nRow < m_pModel->GetCount(); ++nRow)
        m_pModel->RowChanged(nRow);
}

void Grid::EnsureIndexVisible(gsl::index nIndex)
{
    if (nIndex >= 0 && gsl::narrow_cast<unsigned int>(nIndex) < m_pModel->GetCount())
        EnsureVisible(m_pModel->GetItem(gsl::narrow_cast<unsigned int>(nIndex)));
}

void Grid::Reset()
{
    m_bResetPending = false;
    m_vChangedDuringUpdate.clear();
    m_bSyncingSelection = true;
    m_pModel->Reset(m_pItems ? gsl::narrow_cast<unsigned int>(m_pItems->Count()) : 0);
    m_bSyncingSelection = false;
    SyncSelectionFromItems();
}

void Grid::RowChanged(gsl::index nIndex)
{
    if (m_bResetPending)
        return;

    if (m_nUpdateDepth > 0)
    {
        // redrawn when the update ends
        if (m_vChangedDuringUpdate.empty() || m_vChangedDuringUpdate.back() != nIndex)
            m_vChangedDuringUpdate.push_back(nIndex);
        return;
    }

    if (nIndex >= 0 && gsl::narrow_cast<unsigned int>(nIndex) < m_pModel->GetCount())
        m_pModel->RowChanged(gsl::narrow_cast<unsigned int>(nIndex));
}

void Grid::SyncSelectionFromItems()
{
    if (m_pIsSelectedProperty == nullptr || m_pItems == nullptr)
        return;

    m_bSyncingSelection = true;
    wxDataViewItemArray vSelected;
    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_pItems->Count()); ++i)
    {
        if (m_pItems->GetItemValue(i, *m_pIsSelectedProperty))
            vSelected.push_back(m_pModel->GetItem(gsl::narrow_cast<unsigned int>(i)));
    }
    SetSelections(vSelected);
    m_bSyncingSelection = false;
}

void Grid::OnSelectionChanged()
{
    if (m_bSyncingSelection || m_pItems == nullptr)
        return;

    if (m_fSelectionChanged)
    {
        wxDataViewItemArray vItems;
        GetSelections(vItems);
        std::vector<gsl::index> vIndices;
        for (const auto& pItem : vItems)
            vIndices.push_back(gsl::narrow_cast<gsl::index>(m_pModel->GetRow(pItem)));
        std::sort(vIndices.begin(), vIndices.end());

        m_bSyncingSelection = true;
        m_fSelectionChanged(vIndices);
        m_bSyncingSelection = false;
        return;
    }

    if (m_pIsSelectedProperty == nullptr)
        return;

    std::vector<bool> vSelected(m_pItems->Count(), false);
    wxDataViewItemArray vItems;
    GetSelections(vItems);
    for (const auto& pItem : vItems)
    {
        const auto nRow = m_pModel->GetRow(pItem);
        if (nRow < vSelected.size())
            vSelected.at(nRow) = true;
    }

    m_bSyncingSelection = true;
    for (size_t i = 0; i < vSelected.size(); ++i)
    {
        const auto nIndex = gsl::narrow_cast<gsl::index>(i);
        if (m_pItems->GetItemValue(nIndex, *m_pIsSelectedProperty) != vSelected.at(i))
            m_pItems->SetItemValue(nIndex, *m_pIsSelectedProperty, vSelected.at(i));
    }
    m_bSyncingSelection = false;
}

bool Grid::ScheduleResetIfNotMainThread()
{
    // collections may change on other threads (i.e. when a server response arrives), but the control may only
    // be changed on the main thread: refresh everything there, once
    if (wxIsMainThread())
        return false;

    if (!m_bResetScheduled.exchange(true))
    {
        CallAfter([this]() {
            m_bResetScheduled = false;
            if (m_nUpdateDepth == 0)
                Reset();
            else
                m_bResetPending = true;
        });
    }

    return true;
}

void Grid::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (ScheduleResetIfNotMainThread())
        return;

    if (m_pIsSelectedProperty && args.Property == *m_pIsSelectedProperty)
    {
        if (m_nUpdateDepth > 0)
            m_bSelectionChangedDuringUpdate = true;
        else if (!m_bSyncingSelection && !m_bResetPending)
        {
            m_bSyncingSelection = true;
            const auto pItem = m_pModel->GetItem(gsl::narrow_cast<unsigned int>(nIndex));
            if (args.tNewValue)
                Select(pItem);
            else
                Unselect(pItem);
            m_bSyncingSelection = false;
        }
        return;
    }

    RowChanged(nIndex);
}

void Grid::OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs&)
{
    if (ScheduleResetIfNotMainThread())
        return;

    RowChanged(nIndex);
}

void Grid::OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs&)
{
    if (ScheduleResetIfNotMainThread())
        return;

    RowChanged(nIndex);
}

void Grid::OnViewModelAdded(gsl::index)
{
    if (ScheduleResetIfNotMainThread())
        return;

    if (m_nUpdateDepth > 0)
        m_bResetPending = true;
    else
        Reset();
}

void Grid::OnViewModelRemoved(gsl::index)
{
    if (ScheduleResetIfNotMainThread())
        return;

    if (m_nUpdateDepth > 0)
        m_bResetPending = true;
    else
        Reset();
}

void Grid::OnViewModelChanged(gsl::index nIndex)
{
    if (!ScheduleResetIfNotMainThread())
        RowChanged(nIndex);
}

void Grid::OnBeginViewModelCollectionUpdate()
{
    if (!ScheduleResetIfNotMainThread())
        ++m_nUpdateDepth;
}

void Grid::OnEndViewModelCollectionUpdate()
{
    if (ScheduleResetIfNotMainThread())
        return;

    if (m_nUpdateDepth > 0 && --m_nUpdateDepth == 0)
    {
        std::vector<gsl::index> vChanged;
        vChanged.swap(m_vChangedDuringUpdate);

        if (m_bResetPending)
        {
            Reset();
        }
        else
        {
            std::sort(vChanged.begin(), vChanged.end());
            vChanged.erase(std::unique(vChanged.begin(), vChanged.end()), vChanged.end());
            for (const auto nIndex : vChanged)
                RowChanged(nIndex);

            if (m_bSelectionChangedDuringUpdate)
                SyncSelectionFromItems();
        }
        m_bSelectionChangedDuringUpdate = false;
    }
}

} // namespace wx
} // namespace ui
} // namespace ra
