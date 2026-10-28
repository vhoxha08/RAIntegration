#include "WxBinding.hh"

#include "ui/wx/WxApp.hh"

#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
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

ViewModelAccess& Binder::Access(ViewModelBase& vmViewModel)
{
    for (auto& pAccess : m_vAccess)
    {
        if (pAccess.first == &vmViewModel)
            return *pAccess.second;
    }

    m_vAccess.emplace_back(&vmViewModel, std::make_unique<ViewModelAccess>(vmViewModel));
    return *m_vAccess.back().second;
}

void Binder::BindText(wxTextEntry* pControl, ViewModelBase& vmViewModel, const StringModelProperty& pProperty)
{
    auto& pAccess = Access(vmViewModel);
    auto* pWindow = dynamic_cast<wxWindow*>(pControl);
    Expects(pWindow != nullptr);
    pWindow->Bind(wxEVT_TEXT, [pControl, &pAccess, &pProperty](wxCommandEvent&) {
        pAccess.SetValue(pProperty, FromWx(pControl->GetValue()));
    });
    m_vUpdaters.push_back([pControl, &pAccess, &pProperty]() { SetTextIfChanged(*pControl, pAccess.GetValue(pProperty)); });
}

void Binder::BindLabel(wxWindow* pControl, ViewModelBase& vmViewModel, const StringModelProperty& pProperty)
{
    auto& pAccess = Access(vmViewModel);
    m_vUpdaters.push_back([pControl, &pAccess, &pProperty]() { SetLabelIfChanged(*pControl, pAccess.GetValue(pProperty)); });
}

void Binder::BindLabel(wxWindow* pControl, ViewModelBase& vmViewModel, const IntModelProperty& pProperty)
{
    auto& pAccess = Access(vmViewModel);
    m_vUpdaters.push_back([pControl, &pAccess, &pProperty]() {
        SetLabelIfChanged(*pControl, std::to_wstring(pAccess.GetValue(pProperty)));
    });
}

void Binder::BindEnabled(wxWindow* pControl, ViewModelBase& vmViewModel, const BoolModelProperty& pProperty)
{
    auto& pAccess = Access(vmViewModel);
    m_vUpdaters.push_back([pControl, &pAccess, &pProperty]() { pControl->Enable(pAccess.GetValue(pProperty)); });
}

void Binder::BindVisible(wxWindow* pControl, ViewModelBase& vmViewModel, const BoolModelProperty& pProperty)
{
    auto& pAccess = Access(vmViewModel);
    m_vUpdaters.push_back([pControl, &pAccess, &pProperty]() {
        const bool bVisible = pAccess.GetValue(pProperty);
        if (pControl->IsShown() != bVisible)
        {
            pControl->Show(bVisible);
            if (auto* pSizer = pControl->GetContainingSizer())
                pSizer->Layout();
        }
    });
}

void Binder::BindCheck(wxCheckBox* pControl, ViewModelBase& vmViewModel, const BoolModelProperty& pProperty)
{
    auto& pAccess = Access(vmViewModel);
    pControl->Bind(wxEVT_CHECKBOX, [&pAccess, &pProperty](wxCommandEvent& event) {
        pAccess.SetValue(pProperty, event.IsChecked());
    });
    m_vUpdaters.push_back([pControl, &pAccess, &pProperty]() {
        if (pControl->GetValue() != pAccess.GetValue(pProperty))
            pControl->SetValue(pAccess.GetValue(pProperty));
    });
}

void Binder::BindChoice(LookupChoice* pControl, const ra::ui::viewmodels::LookupItemViewModelCollection& vItems,
                        ViewModelBase& vmViewModel, const IntModelProperty& pProperty)
{
    auto& pAccess = Access(vmViewModel);
    pControl->SetOnSelected([&pAccess, &pProperty](int nId) { pAccess.SetValue(pProperty, nId); });
    m_vUpdaters.push_back([pControl, &vItems, &pAccess, &pProperty]() {
        pControl->Update(vItems, pAccess.GetValue(pProperty));
    });
}

void Binder::BindRadio(wxRadioButton* pControl, ViewModelBase& vmViewModel, const IntModelProperty& pProperty,
                       int nValue)
{
    auto& pAccess = Access(vmViewModel);
    pControl->Bind(wxEVT_RADIOBUTTON, [&pAccess, &pProperty, nValue](wxCommandEvent&) {
        pAccess.SetValue(pProperty, nValue);
    });
    m_vUpdaters.push_back([pControl, &pAccess, &pProperty, nValue]() {
        const bool bSelected = (pAccess.GetValue(pProperty) == nValue);
        if (bSelected && !pControl->GetValue())
            pControl->SetValue(true);
    });
}

void Binder::Update()
{
    for (const auto& fUpdate : m_vUpdaters)
        fUpdate();
}

LookupChoice::LookupChoice(wxWindow* pParent, std::function<void(int)> fOnSelected) :
    wxChoice(pParent, wxID_ANY),
    m_fOnSelected(std::move(fOnSelected))
{
    Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
        const int nIndex = GetSelection();
        if (m_fOnSelected && nIndex >= 0 && gsl::narrow_cast<size_t>(nIndex) < m_vItems.size())
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
