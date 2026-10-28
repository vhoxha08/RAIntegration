#include "AssetListDialog.hh"

#include "data/models/AchievementModel.hh"
#include "data/models/GameAssets.hh"

#include "ui/wx/WxApp.hh"
#include "ui/wx/WxGrid.hh"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace ra {
namespace ui {
namespace wx {

using ra::data::models::AssetModelBase;
using ra::data::models::AssetType;
using ra::ui::viewmodels::AssetListViewModel;
using AssetSummary = AssetListViewModel::AssetSummaryViewModel;

static AssetType GetType(const ViewModelCollectionBase& vItems, gsl::index nIndex)
{
    return ra::itoe<AssetType>(vItems.GetItemValue(nIndex, AssetModelBase::TypeProperty));
}

AssetListDialog::AssetListDialog(AssetListViewModel& vmAssetList) :
    WxDialog(vmAssetList, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX)
{
    auto& vm = vmAssetList;
    Watch(vm.AchievementSets());

    auto* pSizer = new wxBoxSizer(wxVERTICAL);

    // game and totals
    auto* pGameRow = new wxBoxSizer(wxHORIZONTAL);
    auto* pSubsets = new LookupChoice(this);
    pSubsets->SetMinSize(FromDIP(wxSize(260, -1)));
    m_pBinder.BindChoice(pSubsets, vm.AchievementSets(), vm, AssetListViewModel::SubsetFilterProperty);
    pGameRow->Add(pSubsets, 0, wxALIGN_CENTER_VERTICAL);
    pGameRow->AddStretchSpacer();
    auto AddTotal = [this, pGameRow, &vm](const char* sLabel, const IntModelProperty& pProperty) {
        pGameRow->Add(new wxStaticText(this, wxID_ANY, sLabel), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
        auto* pValue = new wxStaticText(this, wxID_ANY, "0");
        m_pBinder.BindLabel(pValue, vm, pProperty);
        pGameRow->Add(pValue, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    };
    AddTotal("Game ID:", AssetListViewModel::GameIdProperty);
    AddTotal("Achievements:", AssetListViewModel::AchievementCountProperty);
    AddTotal("Points:", AssetListViewModel::TotalPointsProperty);
    pSizer->Add(pGameRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    // filters and processing
    auto* pFilterRow = new wxBoxSizer(wxHORIZONTAL);
    auto AddFilter = [this, pFilterRow, &vm](const ra::ui::viewmodels::LookupItemViewModelCollection& vItems,
                                             const IntModelProperty& pProperty) {
        auto* pChoice = new LookupChoice(this);
        m_pBinder.BindChoice(pChoice, vItems, vm, pProperty);
        pFilterRow->Add(pChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    };
    AddFilter(vm.SpecialFilters(), AssetListViewModel::SpecialFilterProperty);
    AddFilter(vm.Categories(), AssetListViewModel::CategoryFilterProperty);
    AddFilter(vm.AssetTypeFilters(), AssetListViewModel::AssetTypeFilterProperty);
    pFilterRow->AddStretchSpacer();
    auto* pProcessing = new wxCheckBox(this, wxID_ANY, "Processing Active");
    m_pBinder.BindCheck(pProcessing, vm, AssetListViewModel::IsProcessingActiveProperty);
    pFilterRow->Add(pProcessing, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    auto* pKeepActive = new wxCheckBox(this, wxID_ANY, "Keep Active");
    m_pBinder.BindCheck(pKeepActive, vm, AssetListViewModel::KeepActiveProperty);
    pFilterRow->Add(pKeepActive, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    auto* pActivate = new wxButton(this, wxID_ANY, "Activate All");
    pActivate->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.ActivateSelected(); });
    m_pBinder.BindLabel(pActivate, vm, AssetListViewModel::ActivateButtonTextProperty);
    m_pBinder.BindEnabled(pActivate, vm, AssetListViewModel::CanActivateProperty);
    pFilterRow->Add(pActivate, 0, wxALIGN_CENTER_VERTICAL);
    pSizer->Add(pFilterRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    // assets
    m_pAssets = new Grid(this);
    auto pType = GridColumn::Custom(L"Type", 40, [](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        switch (GetType(vItems, nIndex))
        {
            case AssetType::Achievement: return std::wstring(L"\U0001F3C6");  // Trophy
            case AssetType::Leaderboard: return std::wstring(L"\U0001F4CA");  // Bar Chart
            case AssetType::RichPresence: return std::wstring(L"\U0001F4DC"); // Scroll
            default: return std::to_wstring(ra::etoi(GetType(vItems, nIndex)));
        }
    });
    pType.nAlignment = wxALIGN_CENTER;
    m_pAssets->AddColumn(std::move(pType));
    auto pId = GridColumn::Custom(L"ID", 60, [](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        const auto nId = vItems.GetItemValue(nIndex, AssetSummary::IdProperty);
        if (nId >= ra::data::models::GameAssets::FirstLocalId)
            return std::wstring(L"0");
        if (GetType(vItems, nIndex) == AssetType::RichPresence)
            return std::wstring();
        return std::to_wstring(nId);
    });
    pId.nAlignment = wxALIGN_RIGHT;
    m_pAssets->AddColumn(std::move(pId));
    m_pAssets->AddColumn(GridColumn::Text(L"Name", AssetSummary::LabelProperty, 280));
    auto pPoints = GridColumn::Custom(L"Points", 56, [](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
        if (GetType(vItems, nIndex) != AssetType::Achievement)
            return std::wstring();
        return std::to_wstring(vItems.GetItemValue(nIndex, ra::data::models::AchievementModel::PointsProperty));
    });
    pPoints.nAlignment = wxALIGN_RIGHT;
    m_pAssets->AddColumn(std::move(pPoints));
    m_pAssets->AddColumn(GridColumn::Lookup(L"State", AssetModelBase::StateProperty, vm.States(), 80));
    m_pAssets->AddColumn(GridColumn::Lookup(L"Category", AssetModelBase::CategoryProperty, vm.Categories(), 90));
    m_pAssets->AddColumn(GridColumn::Custom(L"Changes", 100, [&vm](const ViewModelCollectionBase& vItems,
                                                                   gsl::index nIndex) {
        std::wstring sLabel = vm.Changes().GetLabelForId(vItems.GetItemValue(nIndex, AssetModelBase::ChangesProperty));
        if (!vItems.GetItemValue(nIndex, AssetModelBase::ValidationErrorProperty).empty())
            sLabel.append(L"\U000026A0"); // Warning Sign
        return sLabel;
    }));
    m_pAssets->BindIsSelected(AssetSummary::IsSelectedProperty);
    m_pAssets->BindItems(vm.FilteredAssets());
    m_pAssets->SetDoubleClickHandler([&vm](gsl::index nIndex) {
        const auto* pSummary = vm.FilteredAssets().GetItemAt(nIndex);
        if (pSummary)
            vm.OpenEditor(pSummary);
    });
    m_pAssets->SetMinSize(FromDIP(wxSize(720, 300)));
    pSizer->Add(m_pAssets, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);

    // the view model asks for a new or changed asset to be scrolled into view
    auto& pAccess = m_pBinder.Access(vm);
    m_pBinder.OnUpdate([this, &pAccess]() {
        const int nIndex = pAccess.GetValue(AssetListViewModel::EnsureVisibleAssetIndexProperty);
        if (nIndex != m_nEnsureVisibleIndex)
        {
            m_nEnsureVisibleIndex = nIndex;
            m_pAssets->EnsureIndexVisible(nIndex);
        }
    });

    // commands
    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    auto AddButton = [this, pButtons, &vm](const char* sLabel, std::function<void()> fAction,
                                           const BoolModelProperty& pEnabled, const StringModelProperty* pLabel) {
        auto* pButton = new wxButton(this, wxID_ANY, sLabel);
        pButton->Bind(wxEVT_BUTTON, [fAction](wxCommandEvent&) { fAction(); });
        m_pBinder.BindEnabled(pButton, vm, pEnabled);
        if (pLabel)
            m_pBinder.BindLabel(pButton, vm, *pLabel);
        pButtons->Add(pButton, 0, wxRIGHT, 4);
    };
    AddButton("New", [&vm]() { vm.CreateNew(); }, AssetListViewModel::CanCreateProperty, nullptr);
    AddButton("Clone", [&vm]() { vm.CloneSelected(); }, AssetListViewModel::CanCloneProperty, nullptr);
    pButtons->AddStretchSpacer();
    AddButton("Save All", [&vm]() { vm.SaveSelected(); }, AssetListViewModel::CanSaveProperty,
              &AssetListViewModel::SaveButtonTextProperty);
    AddButton("Reset All", [&vm]() { vm.ReloadSelected(); }, AssetListViewModel::CanReloadProperty,
              &AssetListViewModel::ReloadButtonTextProperty);
    AddButton("Revert All", [&vm]() { vm.RevertSelected(); }, AssetListViewModel::CanRevertProperty,
              &AssetListViewModel::RevertButtonTextProperty);
    pSizer->Add(pButtons, 0, wxEXPAND | wxALL, 8);

    SetSizerAndFit(pSizer);
    RememberSizeAndPosition("Achievements");
    UpdateControls();
}

} // namespace wx
} // namespace ui
} // namespace ra
