#include "WxBinding.hh"

#include "ui/wx/WxApp.hh"

#include <wx/textentry.h>
#include <wx/window.h>

#include <algorithm>

namespace ra {
namespace ui {
namespace wx {

ChangeWatcher::~ChangeWatcher() noexcept { Detach(); }

void ChangeWatcher::Watch(ViewModelBase& vmViewModel)
{
    vmViewModel.AddNotifyTarget(*this);
    m_vViewModels.push_back(&vmViewModel);
}

void ChangeWatcher::Watch(ViewModelCollectionBase& vmCollection)
{
    vmCollection.AddNotifyTarget(*this);
    m_vCollections.push_back(&vmCollection);
}

void ChangeWatcher::Detach() noexcept
{
    for (auto* pViewModel : m_vViewModels)
        pViewModel->RemoveNotifyTarget(*this);
    m_vViewModels.clear();

    for (auto* pCollection : m_vCollections)
    {
        try
        {
            pCollection->RemoveNotifyTarget(*this);
        }
        catch (...)
        {
        }
    }
    m_vCollections.clear();
}

LookupChoice::LookupChoice(wxWindow* pParent, std::function<void(int)> fOnSelected) :
    wxChoice(pParent, wxID_ANY),
    m_fOnSelected(std::move(fOnSelected))
{
    Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
        const int nIndex = GetSelection();
        if (nIndex >= 0 && gsl::narrow_cast<size_t>(nIndex) < m_vItems.size())
            m_fOnSelected(m_vItems.at(gsl::narrow_cast<size_t>(nIndex)).first);
    });
}

void LookupChoice::Update(std::vector<std::pair<int, std::wstring>>&& vItems, int nSelectedId)
{
    if (vItems != m_vItems)
    {
        m_vItems = std::move(vItems);

        // keep an explicitly set minimum size, otherwise the control resizes to its longest label
        const wxSize szMin = GetMinSize();
        wxArrayString vLabels;
        for (const auto& pItem : m_vItems)
            vLabels.Add(ToWx(pItem.second));
        Set(vLabels);
        if (szMin.GetWidth() > 0)
            SetMinSize(szMin);
        else
            InvalidateBestSize();
    }

    const auto pIter = std::find_if(m_vItems.begin(), m_vItems.end(),
                                    [nSelectedId](const auto& pItem) { return pItem.first == nSelectedId; });
    const int nIndex = (pIter == m_vItems.end()) ? wxNOT_FOUND : gsl::narrow_cast<int>(pIter - m_vItems.begin());
    if (GetSelection() != nIndex)
        SetSelection(nIndex);
}

void SetTextIfChanged(wxTextEntry& pControl, const std::wstring& sValue)
{
    const wxString sNewValue = ToWx(sValue);
    if (pControl.GetValue() != sNewValue)
        pControl.ChangeValue(sNewValue); // ChangeValue doesn't raise wxEVT_TEXT
}

void SetLabelIfChanged(wxWindow& pControl, const std::wstring& sValue)
{
    const wxString sNewValue = ToWx(sValue);
    if (pControl.GetLabel() != sNewValue)
        pControl.SetLabel(sNewValue);
}

} // namespace wx
} // namespace ui
} // namespace ra
