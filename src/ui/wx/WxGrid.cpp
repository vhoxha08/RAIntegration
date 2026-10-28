#include "WxGrid.hh"

#include "ui/wx/WxApp.hh"

#include <wx/app.h>
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
        bool bChanged = false;
        if (m_pGrid.m_pRowColorProperty)
        {
            const auto nColor = ra::to_unsigned(vItems.GetItemValue(nRow, *m_pGrid.m_pRowColorProperty));
            if ((nColor >> 24) != 0)
            {
                pAttr.SetBackgroundColour(wxColour((nColor >> 16) & 0xFF, (nColor >> 8) & 0xFF, nColor & 0xFF));
                bChanged = true;
            }
        }

        const auto& pColumn = m_pGrid.m_vColumns.at(nColumn);
        if (pColumn.pTextColorProperty)
        {
            const auto nColor = ra::to_unsigned(vItems.GetItemValue(nRow, *pColumn.pTextColorProperty));
            if ((nColor >> 24) != 0)
            {
                pAttr.SetColour(wxColour((nColor >> 16) & 0xFF, (nColor >> 8) & 0xFF, nColor & 0xFF));
                bChanged = true;
            }
        }

        return bChanged;
    }

private:
    Grid& m_pGrid;
};

// ---- grid ------------------------------------------------------------------

Grid::Grid(wxWindow* pParent, long nStyle) : wxDataViewCtrl(pParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, nStyle)
{
    m_pModel = new Model(*this);
    AssociateModel(m_pModel);
    m_pModel->DecRef(); // the control holds the reference

    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this](wxDataViewEvent&) { OnSelectionChanged(); });
    Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [this](wxDataViewEvent& event) {
        if (m_fContextMenu && event.GetItem().IsOk())
            m_fContextMenu(gsl::narrow_cast<gsl::index>(m_pModel->GetRow(event.GetItem())));
    });
    Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, [this](wxDataViewEvent& event) {
        if (m_fDoubleClick && event.GetItem().IsOk())
            m_fDoubleClick(gsl::narrow_cast<gsl::index>(m_pModel->GetRow(event.GetItem())));
    });
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
        {
            wxArrayString vLabels;
            for (const auto& pChoice : *pColumn.pChoices)
                vLabels.Add(ToWx(pChoice.GetLabel()));
            pRenderer = new wxDataViewChoiceRenderer(vLabels, wxDATAVIEW_CELL_EDITABLE, pColumn.nAlignment);
            break;
        }

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
    m_bSyncingSelection = true;
    m_pModel->Reset(m_pItems ? gsl::narrow_cast<unsigned int>(m_pItems->Count()) : 0);
    m_bSyncingSelection = false;
    SyncSelectionFromItems();
}

void Grid::RowChanged(gsl::index nIndex)
{
    if (m_nUpdateDepth > 0 || m_bResetPending)
        return;

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
        if (!m_bSyncingSelection && m_nUpdateDepth == 0 && !m_bResetPending)
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
        if (m_bResetPending)
            Reset();
        else
            RefreshItems();
    }
}

} // namespace wx
} // namespace ui
} // namespace ra
