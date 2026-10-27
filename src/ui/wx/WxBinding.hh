#ifndef RA_UI_WX_BINDING_HH
#define RA_UI_WX_BINDING_HH
#pragma once

#include "ui/ViewModelCollection.hh"
#include "ui/viewmodels/LookupItemViewModel.hh"

#include <wx/choice.h>
#include <wx/textentry.h>

#include <functional>
#include <vector>

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// Calls a function whenever a view model or a collection of view models changes.
/// </summary>
class ChangeWatcher : protected ViewModelBase::NotifyTarget, protected ViewModelCollectionBase::NotifyTarget
{
public:
    explicit ChangeWatcher(std::function<void()> fOnChanged) noexcept : m_fOnChanged(std::move(fOnChanged)) {}
    ~ChangeWatcher() noexcept override;
    ChangeWatcher(const ChangeWatcher&) noexcept = delete;
    ChangeWatcher& operator=(const ChangeWatcher&) noexcept = delete;
    ChangeWatcher(ChangeWatcher&&) noexcept = delete;
    ChangeWatcher& operator=(ChangeWatcher&&) noexcept = delete;

    void Watch(ViewModelBase& vmViewModel);
    void Watch(ViewModelCollectionBase& vmCollection);

    /// <summary>
    /// Stops watching everything (call before the watched objects are destroyed).
    /// </summary>
    void Detach() noexcept;

protected:
    void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs&) override { m_fOnChanged(); }
    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs&) override { m_fOnChanged(); }
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs&) override { m_fOnChanged(); }

    void OnViewModelBoolValueChanged(gsl::index, const BoolModelProperty::ChangeArgs&) override { m_fOnChanged(); }
    void OnViewModelStringValueChanged(gsl::index, const StringModelProperty::ChangeArgs&) override { m_fOnChanged(); }
    void OnViewModelIntValueChanged(gsl::index, const IntModelProperty::ChangeArgs&) override { m_fOnChanged(); }
    void OnViewModelAdded(gsl::index) override { m_fOnChanged(); }
    void OnViewModelRemoved(gsl::index) override { m_fOnChanged(); }
    void OnViewModelChanged(gsl::index) override { m_fOnChanged(); }
    void OnEndViewModelCollectionUpdate() override { m_fOnChanged(); }

private:
    std::function<void()> m_fOnChanged;
    std::vector<ViewModelBase*> m_vViewModels;
    std::vector<ViewModelCollectionBase*> m_vCollections;
};

/// <summary>
/// Native drop down list of lookup items (the counterpart of the Win32 ComboBoxBinding).
/// </summary>
class LookupChoice : public wxChoice
{
public:
    LookupChoice(wxWindow* pParent, std::function<void(int)> fOnSelected);

    /// <summary>
    /// Rebuilds the items if the collection changed, and selects the item with <paramref name="nSelectedId" />.
    /// </summary>
    template<class TCollection>
    void Update(const TCollection& vItems, int nSelectedId)
    {
        std::vector<std::pair<int, std::wstring>> vNewItems;
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vItems.Count()); ++i)
        {
            if (const auto* pItem = vItems.GetItemAt(i))
                vNewItems.emplace_back(pItem->GetId(), pItem->GetLabel());
        }
        Update(std::move(vNewItems), nSelectedId);
    }

    void Update(std::vector<std::pair<int, std::wstring>>&& vItems, int nSelectedId);

private:
    std::function<void(int)> m_fOnSelected;
    std::vector<std::pair<int, std::wstring>> m_vItems;
};

/// <summary>
/// Sets a text control's value only if it differs (keeps the caret and avoids change events while typing).
/// </summary>
void SetTextIfChanged(wxTextEntry& pControl, const std::wstring& sValue);

/// <summary>
/// Sets a label's text only if it differs.
/// </summary>
void SetLabelIfChanged(wxWindow& pControl, const std::wstring& sValue);

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_BINDING_HH
