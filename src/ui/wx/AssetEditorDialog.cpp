#include "AssetEditorDialog.hh"

#include "RA_Defs.h"

#include "data/models/AchievementModel.hh"

#include "services/IConfiguration.hh"
#include "services/ServiceLocator.hh"

#include "ui/IImageRepository.hh"
#include "ui/viewmodels/MessageBoxViewModel.hh"
#include "ui/viewmodels/WindowManager.hh"
#include "ui/wx/WxApp.hh"
#include "ui/wx/WxGrid.hh"

#include "util/Strings.hh"

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/image.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/spinbutt.h>
#include <wx/statbmp.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace ra {
namespace ui {
namespace wx {

using ra::data::models::AchievementModel;
using ra::ui::viewmodels::AssetEditorViewModel;
using ra::ui::viewmodels::TriggerConditionViewModel;
using ra::ui::viewmodels::TriggerViewModel;

// the trigger view model only materializes VisibleItemCount conditions (the Win32 list is virtual); the native
// list shows all of them
constexpr int ALL_CONDITIONS = 100000;

constexpr int BADGE_SIZE = 64;

AssetEditorDialog::AssetEditorDialog(AssetEditorViewModel& vmAssetEditor) :
    WxDialog(vmAssetEditor, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX),
    m_vmAssetEditor(vmAssetEditor)
{
    auto& vmTrigger = vmAssetEditor.Trigger();
    Watch(vmTrigger);
    Watch(vmAssetEditor.LeaderboardParts());
    m_pBinder.Access(vmTrigger).SetValue(TriggerViewModel::VisibleItemCountProperty, ALL_CONDITIONS);
    m_pBadgeTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { UpdateBadge(); });

    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    pSizer->Add(CreateHeader(), 0, wxEXPAND | wxALL, 8);
    pSizer->Add(CreateTrigger(), 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizer(pSizer);
    SetMinSize(FromDIP(wxSize(860, 480)));
    Fit();

    UpdateControls();
}

wxSizer* AssetEditorDialog::CreateHeader()
{
    auto& vm = m_vmAssetEditor;
    auto* pHeader = new wxBoxSizer(wxHORIZONTAL);
    auto* pFields = new wxFlexGridSizer(2, wxSize(6, 4));
    pFields->AddGrowableCol(1);

    // Title / ID / Active
    pFields->Add(new wxStaticText(this, wxID_ANY, "Title:"), 0, wxALIGN_CENTER_VERTICAL);
    auto* pTitleRow = new wxBoxSizer(wxHORIZONTAL);
    auto* pTitle = new wxTextCtrl(this, wxID_ANY);
    m_pBinder.BindText(pTitle, vm, AssetEditorViewModel::NameProperty);
    m_pBinder.BindEnabled(pTitle, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pTitleRow->Add(pTitle, 1, wxALIGN_CENTER_VERTICAL);
    pTitleRow->Add(new wxStaticText(this, wxID_ANY, "ID:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    auto* pId = new wxStaticText(this, wxID_ANY, "0", wxDefaultPosition, FromDIP(wxSize(64, -1)),
                                 wxALIGN_RIGHT | wxST_NO_AUTORESIZE);
    m_pBinder.BindLabel(pId, vm, AssetEditorViewModel::IDProperty);
    pTitleRow->Add(pId, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    m_pActive = new wxCheckBox(this, wxID_ANY, "Active");
    m_pActive->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { OnActiveClicked(); });
    m_pBinder.BindLabel(m_pActive, vm, AssetEditorViewModel::WaitingLabelProperty);
    m_pBinder.BindEnabled(m_pActive, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pTitleRow->Add(m_pActive, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    pFields->Add(pTitleRow, 1, wxEXPAND);

    // Description
    pFields->Add(new wxStaticText(this, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
    auto* pDescription = new wxTextCtrl(this, wxID_ANY);
    m_pBinder.BindText(pDescription, vm, AssetEditorViewModel::DescriptionProperty);
    m_pBinder.BindEnabled(pDescription, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pFields->Add(pDescription, 1, wxEXPAND);

    // achievement: Points / Type / Measured; leaderboard: Format / Lower is Better / Value
    pFields->AddSpacer(0);
    auto* pDetails = new wxBoxSizer(wxHORIZONTAL);
    auto AddLabel = [this, pDetails](const char* sLabel, const BoolModelProperty& pVisible, int nLeft) {
        auto* pLabel = new wxStaticText(this, wxID_ANY, sLabel);
        m_pBinder.BindVisible(pLabel, m_vmAssetEditor, pVisible);
        pDetails->Add(pLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, nLeft);
        return pLabel;
    };

    AddLabel("Points:", AssetEditorViewModel::IsAchievementProperty, 0);
    m_pPoints = new LookupChoice(this, [&vm](int nPoints) { vm.SetPoints(nPoints); });
    m_pBinder.BindVisible(m_pPoints, vm, AssetEditorViewModel::IsAchievementProperty);
    m_pBinder.BindEnabled(m_pPoints, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pDetails->Add(m_pPoints, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    AddLabel("Type:", AssetEditorViewModel::IsAchievementProperty, 12);
    auto* pType = new LookupChoice(this);
    m_pBinder.BindChoice(pType, vm.AchievementTypes(), vm, AssetEditorViewModel::AchievementTypeProperty);
    m_pBinder.BindVisible(pType, vm, AssetEditorViewModel::IsAchievementProperty);
    pDetails->Add(pType, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

    AddLabel("Format:", AssetEditorViewModel::IsLeaderboardProperty, 0);
    auto* pFormat = new LookupChoice(this);
    m_pBinder.BindChoice(pFormat, vm.Formats(), vm, AssetEditorViewModel::ValueFormatProperty);
    m_pBinder.BindVisible(pFormat, vm, AssetEditorViewModel::IsLeaderboardProperty);
    pDetails->Add(pFormat, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    auto* pLowerIsBetter = new wxCheckBox(this, wxID_ANY, "Lower is Better");
    m_pBinder.BindCheck(pLowerIsBetter, vm, AssetEditorViewModel::LowerIsBetterProperty);
    m_pBinder.BindVisible(pLowerIsBetter, vm, AssetEditorViewModel::IsLeaderboardProperty);
    pDetails->Add(pLowerIsBetter, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    AddLabel("Display:", AssetEditorViewModel::IsLeaderboardProperty, 12);
    auto* pDisplay = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_pBinder.BindLabel(pDisplay, vm, AssetEditorViewModel::FormattedValueProperty);
    m_pBinder.BindVisible(pDisplay, vm, AssetEditorViewModel::IsLeaderboardProperty);
    pDetails->Add(pDisplay, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

    pDetails->AddStretchSpacer();
    AddLabel("Measured:", AssetEditorViewModel::HasMeasuredProperty, 12);
    auto* pMeasured = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_pBinder.BindLabel(pMeasured, vm, AssetEditorViewModel::MeasuredValueProperty);
    m_pBinder.BindVisible(pMeasured, vm, AssetEditorViewModel::HasMeasuredProperty);
    pDetails->Add(pMeasured, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    auto* pAsPercent = new wxCheckBox(this, wxID_ANY, "Track as %");
    m_pBinder.BindCheck(pAsPercent, vm.Trigger(), TriggerViewModel::IsMeasuredTrackedAsPercentProperty);
    m_pBinder.OnUpdate([pAsPercent, &vm]() {
        const bool bVisible = vm.HasMeasured() && vm.IsAchievement();
        if (pAsPercent->IsShown() != bVisible)
        {
            pAsPercent->Show(bVisible);
            pAsPercent->GetContainingSizer()->Layout();
        }
    });
    pDetails->Add(pAsPercent, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    pFields->Add(pDetails, 1, wxEXPAND);
    pHeader->Add(pFields, 1, wxEXPAND | wxRIGHT, 8);

    // badge (achievements)
    auto* pBadgeBox = new wxStaticBoxSizer(wxHORIZONTAL, this, "Badge");
    auto* pBadgeParent = pBadgeBox->GetStaticBox();
    auto* pBadgeLeft = new wxBoxSizer(wxVERTICAL);
    auto* pBadgeRow = new wxBoxSizer(wxHORIZONTAL);
    m_pBadge = new wxTextCtrl(pBadgeParent, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(64, -1)),
                              wxTE_RIGHT | wxTE_PROCESS_ENTER);
    m_pBadge->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { ApplyBadgeText(); });
    m_pBadge->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& event) {
        ApplyBadgeText();
        event.Skip();
    });
    pBadgeRow->Add(m_pBadge, 0, wxALIGN_CENTER_VERTICAL);
    auto* pSpin = new wxSpinButton(pBadgeParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_VERTICAL | wxSP_WRAP);
    pSpin->SetRange(-1, 1);
    pSpin->SetValue(0);
    pSpin->Bind(wxEVT_SPIN, [this, pSpin, &vm](wxSpinEvent& event) {
        // step the badge number like the Win32 up/down control
        const std::wstring sBadge = vm.GetBadge();
        unsigned int nBadge = 0;
        std::wstring sError;
        if (!sBadge.empty() && ra::ParseUnsignedInt(sBadge, 0xFFFFFFFF, nBadge, sError))
        {
            nBadge = (event.GetPosition() > 0) ? nBadge + 1 : (nBadge > 0 ? nBadge - 1 : 0);
            vm.SetBadge(ra::util::String::Printf(L"%05u", nBadge));
        }
        pSpin->SetValue(0);
    });
    pBadgeRow->Add(pSpin, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
    pBadgeLeft->Add(pBadgeRow);
    auto* pUpload = new wxButton(pBadgeParent, wxID_ANY, "Upload");
    pUpload->Bind(wxEVT_BUTTON, [&vm](wxCommandEvent&) { vm.SelectBadgeFile(); });
    m_pBinder.BindEnabled(pUpload, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pBadgeLeft->Add(pUpload, 0, wxEXPAND | wxTOP, 4);
    pBadgeBox->Add(pBadgeLeft, 0, wxALL, 4);
    m_pBadgeImage = new wxStaticBitmap(pBadgeParent, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                       FromDIP(wxSize(BADGE_SIZE, BADGE_SIZE)));
    pBadgeBox->Add(m_pBadgeImage, 0, wxALL, 4);
    m_pBinder.BindVisible(pBadgeParent, vm, AssetEditorViewModel::IsAchievementProperty);
    pHeader->Add(pBadgeBox, 0, wxEXPAND);

    // leaderboard parts (start, cancel, submit, value)
    auto* pParts = new Grid(this, wxDV_SINGLE | wxDV_NO_HEADER);
    pParts->AddColumn(GridColumn::Text(L"Part", ra::ui::viewmodels::LookupItemViewModel::LabelProperty, 120));
    pParts->BindIsSelected(ra::ui::viewmodels::LookupItemViewModel::IsSelectedProperty);
    pParts->BindRowColor(AssetEditorViewModel::LeaderboardPartViewModel::ColorProperty);
    pParts->BindItems(vm.LeaderboardParts());
    pParts->SetMinSize(FromDIP(wxSize(140, 90)));
    m_pBinder.BindVisible(pParts, vm, AssetEditorViewModel::IsLeaderboardProperty);
    pHeader->Add(pParts, 0, wxEXPAND);

    return pHeader;
}

wxSizer* AssetEditorDialog::CreateTrigger()
{
    auto& vm = m_vmAssetEditor;
    auto& vmTrigger = vm.Trigger();
    auto* pSizer = new wxBoxSizer(wxHORIZONTAL);

    // groups
    auto* pGroupsColumn = new wxBoxSizer(wxVERTICAL);
    auto* pGroupsLabel = new wxStaticText(this, wxID_ANY, "Groups:");
    m_pBinder.BindLabel(pGroupsLabel, vm, AssetEditorViewModel::GroupsHeaderProperty);
    pGroupsColumn->Add(pGroupsLabel, 0, wxBOTTOM, 4);
    m_pGroups = new Grid(this, wxDV_SINGLE | wxDV_NO_HEADER);
    m_pGroups->AddColumn(GridColumn::Text(L"Group", TriggerViewModel::GroupViewModel::LabelProperty, 80));
    m_pGroups->BindIsSelected(TriggerViewModel::GroupViewModel::IsSelectedProperty);
    m_pGroups->BindRowColor(TriggerViewModel::GroupViewModel::ColorProperty);
    m_pGroups->BindItems(vmTrigger.Groups());
    m_pGroups->SetMinSize(FromDIP(wxSize(90, 160)));
    m_pBinder.BindEnabled(m_pGroups, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pGroupsColumn->Add(m_pGroups, 1, wxEXPAND);
    auto* pGroupButtons = new wxBoxSizer(wxHORIZONTAL);
    auto* pAddGroup = new wxButton(this, wxID_ANY, "+", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    pAddGroup->Bind(wxEVT_BUTTON, [&vmTrigger](wxCommandEvent&) { vmTrigger.AddGroup(); });
    m_pBinder.BindEnabled(pAddGroup, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pGroupButtons->Add(pAddGroup, 1, wxRIGHT, 2);
    auto* pRemoveGroup = new wxButton(this, wxID_ANY, "-", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    pRemoveGroup->Bind(wxEVT_BUTTON, [&vmTrigger](wxCommandEvent&) { vmTrigger.RemoveGroup(); });
    m_pBinder.BindEnabled(pRemoveGroup, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pGroupButtons->Add(pRemoveGroup, 1);
    pGroupsColumn->Add(pGroupButtons, 0, wxEXPAND | wxTOP, 4);
    auto* pCopyAll = new wxButton(this, wxID_ANY, "Copy Def");
    pCopyAll->Bind(wxEVT_BUTTON, [&vmTrigger](wxCommandEvent&) { vmTrigger.CopyToClipboard(); });
    m_pBinder.BindEnabled(pCopyAll, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pGroupsColumn->Add(pCopyAll, 0, wxEXPAND | wxTOP, 2);
    pSizer->Add(pGroupsColumn, 0, wxEXPAND | wxRIGHT, 6);

    // requirements
    auto* pRequirements = new wxBoxSizer(wxVERTICAL);
    auto* pToolbar = new wxBoxSizer(wxHORIZONTAL);
    pToolbar->Add(new wxStaticText(this, wxID_ANY, "Requirements:"), 0, wxALIGN_CENTER_VERTICAL);
    m_pErrorIndicator = new wxStaticText(this, wxID_ANY, L"\U000026A0");
    pToolbar->Add(m_pErrorIndicator, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
    pToolbar->AddStretchSpacer();
    auto* pExplain = new wxButton(this, wxID_ANY, "Explain");
    pExplain->Bind(wxEVT_BUTTON, [&vmTrigger](wxCommandEvent&) { vmTrigger.Summarize(); });
    m_pBinder.BindEnabled(pExplain, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pToolbar->Add(pExplain, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    auto AddCheck = [this, pToolbar, &vm](const char* sLabel, const BoolModelProperty& pProperty,
                                          const BoolModelProperty& pEnabled) {
        auto* pCheck = new wxCheckBox(this, wxID_ANY, sLabel);
        m_pBinder.BindCheck(pCheck, vm, pProperty);
        m_pBinder.BindEnabled(pCheck, vm, pEnabled);
        pToolbar->Add(pCheck, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    };
    AddCheck("Highlights", AssetEditorViewModel::DebugHighlightsEnabledProperty, AssetEditorViewModel::IsAssetLoadedProperty);
    AddCheck("Pause on Reset", AssetEditorViewModel::PauseOnResetProperty, AssetEditorViewModel::IsAssetLoadedProperty);
    AddCheck("Pause on Trigger", AssetEditorViewModel::PauseOnTriggerProperty, AssetEditorViewModel::IsTriggerProperty);
    pRequirements->Add(pToolbar, 0, wxEXPAND | wxBOTTOM, 4);

    m_pConditions = new Grid(this);
    using Condition = TriggerConditionViewModel;
    auto pIndex = GridColumn::Number(L"ID", Condition::IndexProperty, 34);
    m_pConditions->AddColumn(std::move(pIndex));

    // drop downs apply to all selected conditions (TriggerViewModel::SetSelectedItemValues)
    auto SetSelected = [&vmTrigger](const IntModelProperty& pProperty) {
        return [&vmTrigger, &pProperty](ViewModelCollectionBase& vItems, gsl::index nIndex, int nValue) {
            if (vItems.GetItemValue(nIndex, Condition::IsSelectedProperty))
                vmTrigger.SetSelectedItemValues(pProperty, nValue);
            else
                vItems.SetItemValue(nIndex, pProperty, nValue);
        };
    };
    auto Lookup = [&SetSelected](const wchar_t* sHeader, const IntModelProperty& pProperty,
                                 const ra::ui::viewmodels::LookupItemViewModelCollection& vItems, int nWidth,
                                 const BoolModelProperty* pVisible) {
        auto pColumn = GridColumn::Lookup(sHeader, pProperty, vItems, nWidth, true);
        pColumn.fSetChoice = SetSelected(pProperty);
        if (pVisible)
        {
            auto fGetText = pColumn.fGetText;
            pColumn.fGetText = [fGetText, pVisible](const ViewModelCollectionBase& vConditions, gsl::index nIndex) {
                return vConditions.GetItemValue(nIndex, *pVisible) ? fGetText(vConditions, nIndex) : std::wstring();
            };
            pColumn.fIsEditable = [pVisible](const ViewModelCollectionBase& vConditions, gsl::index nIndex) {
                return vConditions.GetItemValue(nIndex, *pVisible);
            };
        }
        return pColumn;
    };

    // memory/value cells: hex addresses, decimal or hex values, floats (ValueColumnBinding)
    auto Value = [](const wchar_t* sHeader, const StringModelProperty& pProperty, const IntModelProperty& pTypeProperty,
                    std::function<bool(const ViewModelCollectionBase&, gsl::index)> fIsVisible) {
        auto pColumn = GridColumn::Text(sHeader, pProperty, 96, true);
        pColumn.fGetText = [&pProperty, fIsVisible](const ViewModelCollectionBase& vConditions, gsl::index nIndex) {
            return fIsVisible(vConditions, nIndex) ? vConditions.GetItemValue(nIndex, pProperty) : std::wstring();
        };
        pColumn.fIsEditable = fIsVisible;
        pColumn.fSetText = [&pProperty, &pTypeProperty](ViewModelCollectionBase& vConditions, gsl::index nIndex,
                                                         const std::wstring& sValue) {
            using OperandType = ra::data::Requirement::OperandType;
            const auto nType = ra::itoe<OperandType>(vConditions.GetItemValue(nIndex, pTypeProperty));
            std::wstring sError;
            if (nType == OperandType::Float)
            {
                float fValue = 0.0f;
                if (!ra::ParseFloat(sValue, fValue, sError))
                {
                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                    return false;
                }
                vConditions.SetItemValue(nIndex, pProperty, ra::data::Requirement::FormatOperandValue(fValue, nType));
                return true;
            }

            if (nType == OperandType::Recall)
            {
                vConditions.SetItemValue(nIndex, pProperty, L"");
                return true;
            }

            unsigned int nValue = 0;
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            const bool bDecimal = (nType == OperandType::Value) &&
                                  pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal);
            if (bDecimal ? !ra::ParseUnsignedInt(sValue, 0xFFFFFFFF, nValue, sError)
                         : !ra::ParseHex(sValue, 0xFFFFFFFF, nValue, sError))
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                return false;
            }
            vConditions.SetItemValue(nIndex, pProperty, ra::data::Requirement::FormatOperandValue(nValue, nType));
            return true;
        };
        return pColumn;
    };

    m_pConditions->AddColumn(Lookup(L"Flag", Condition::TypeProperty, vmTrigger.ConditionTypes(), 110, nullptr));
    m_pConditions->AddColumn(Lookup(L"Type", Condition::SourceTypeProperty, vmTrigger.OperandTypes(), 70, nullptr));
    m_pConditions->AddColumn(Lookup(L"Size", Condition::SourceSizeProperty, vmTrigger.OperandSizes(), 96,
                                    &Condition::HasSourceSizeProperty));
    m_pConditions->AddColumn(Value(L"Memory", Condition::SourceValueProperty, Condition::SourceTypeProperty,
                                   [](const ViewModelCollectionBase& vConditions, gsl::index nIndex) {
                                       return vConditions.GetItemValue(nIndex, Condition::HasSourceValueProperty);
                                   }));
    m_pConditions->AddColumn(Lookup(L"Cmp", Condition::OperatorProperty, vmTrigger.OperatorTypes(), 50, nullptr));
    m_pConditions->AddColumn(Lookup(L"Type", Condition::TargetTypeProperty, vmTrigger.OperandTypes(), 70,
                                    &Condition::HasTargetProperty));
    m_pConditions->AddColumn(Lookup(L"Size", Condition::TargetSizeProperty, vmTrigger.OperandSizes(), 96,
                                    &Condition::HasTargetSizeProperty));
    m_pConditions->AddColumn(Value(L"Mem/Val", Condition::TargetValueProperty, Condition::TargetTypeProperty,
                                   [](const ViewModelCollectionBase& vConditions, gsl::index nIndex) {
                                       return vConditions.GetItemValue(nIndex, Condition::HasTargetProperty) &&
                                              vConditions.GetItemValue(nIndex, Condition::HasTargetValueProperty);
                                   }));

    // "required (current) [total]"; editing changes the required hits (HitsColumnBinding)
    auto pHits = GridColumn::Custom(
        L"Hits", 96,
        [](const ViewModelCollectionBase& vConditions, gsl::index nIndex) {
            if (!vConditions.GetItemValue(nIndex, Condition::HasHitsProperty))
                return std::wstring();
            const auto nRequired = vConditions.GetItemValue(nIndex, Condition::RequiredHitsProperty);
            const auto nCurrent = vConditions.GetItemValue(nIndex, Condition::CurrentHitsProperty);
            const auto nTotal = vConditions.GetItemValue(nIndex, Condition::TotalHitsProperty);
            if (nTotal != 0)
                return ra::util::String::Printf(L"%d (%d) [%d]", nRequired, nCurrent, nTotal);
            return ra::util::String::Printf(L"%d (%d)", nRequired, nCurrent);
        },
        [](ViewModelCollectionBase& vConditions, gsl::index nIndex, const std::wstring& sValue) {
            // accept "5" as well as an unchanged "5 (2)"
            std::wstring sRequired = sValue.substr(0, sValue.find(L' '));
            std::wstring sError;
            unsigned int nValue = 0;
            if (!ra::ParseUnsignedInt(sRequired, 0x7FFFFFFF, nValue, sError))
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                return false;
            }
            vConditions.SetItemValue(nIndex, Condition::RequiredHitsProperty, gsl::narrow_cast<int>(nValue));
            return true;
        });
    pHits.fIsEditable = [](const ViewModelCollectionBase& vConditions, gsl::index nIndex) {
        return vConditions.GetItemValue(nIndex, Condition::HasHitsProperty) &&
               vConditions.GetItemValue(nIndex, Condition::CanEditHitsProperty);
    };
    m_pConditions->AddColumn(std::move(pHits));

    m_pConditions->BindIsSelected(Condition::IsSelectedProperty);
    m_pConditions->BindRowColor(Condition::RowColorProperty);
    m_pConditions->BindItems(vmTrigger.Conditions());
    m_pConditions->SetSelectionHandler([&vmTrigger](const std::vector<gsl::index>& vSelected) {
        // the trigger tracks the selection itself (SelectRange), a contiguous range at a time
        const auto nCount = gsl::narrow_cast<gsl::index>(vmTrigger.Conditions().Count());
        if (nCount > 0)
            vmTrigger.SelectRange(0, nCount - 1, false);
        for (size_t i = 0; i < vSelected.size();)
        {
            size_t j = i;
            while (j + 1 < vSelected.size() && vSelected.at(j + 1) == vSelected.at(j) + 1)
                ++j;
            vmTrigger.SelectRange(vSelected.at(i), vSelected.at(j), true);
            i = j + 1;
        }
    });
    m_pConditions->SetContextMenuHandler([this](gsl::index nIndex) { ShowConditionAddressInInspector(nIndex); });
    m_pConditions->SetMinSize(FromDIP(wxSize(760, 220)));
    m_pBinder.BindEnabled(m_pConditions, vm, AssetEditorViewModel::IsAssetLoadedProperty);
    pRequirements->Add(m_pConditions, 1, wxEXPAND);

    auto* pButtons = new wxBoxSizer(wxHORIZONTAL);
    auto AddButton = [this, pButtons, &vm](const char* sLabel, std::function<void()> fAction, int nSpace) {
        auto* pButton = new wxButton(this, wxID_ANY, sLabel);
        pButton->Bind(wxEVT_BUTTON, [fAction](wxCommandEvent&) { fAction(); });
        m_pBinder.BindEnabled(pButton, vm, AssetEditorViewModel::IsAssetLoadedProperty);
        pButtons->Add(pButton, 0, wxRIGHT, nSpace);
    };
    AddButton("New", [&vmTrigger]() { vmTrigger.NewCondition(); }, 2);
    AddButton("Remove", [&vmTrigger]() { vmTrigger.RemoveSelectedConditions(); }, 12);
    AddButton("Copy", [&vmTrigger]() { vmTrigger.CopySelectedConditionsToClipboard(); }, 2);
    AddButton("Paste", [&vmTrigger]() { vmTrigger.PasteFromClipboard(); }, 12);
    AddButton("Move Up", [&vmTrigger]() { vmTrigger.MoveSelectedConditionsUp(); }, 2);
    AddButton("Move Down", [&vmTrigger]() { vmTrigger.MoveSelectedConditionsDown(); }, 2);
    pButtons->AddStretchSpacer();
    auto* pDecimal = new wxCheckBox(this, wxID_ANY, "Show Decimal");
    m_pBinder.BindCheck(pDecimal, vm, AssetEditorViewModel::DecimalPreferredProperty);
    pButtons->Add(pDecimal, 0, wxALIGN_CENTER_VERTICAL);
    pRequirements->Add(pButtons, 0, wxEXPAND | wxTOP, 4);
    pSizer->Add(pRequirements, 1, wxEXPAND);

    return pSizer;
}

void AssetEditorDialog::UpdateControls()
{
    m_pBinder.Update();

    const bool bActive = ra::data::models::AssetModelBase::IsActive(m_vmAssetEditor.GetState());
    if (m_pActive->GetValue() != bActive)
        m_pActive->SetValue(bActive);

    UpdatePoints();
    UpdateBadge();

    // error/warning indicator with the message as tooltip
    const bool bError = m_vmAssetEditor.HasAssetValidationError();
    const bool bWarning = !bError && m_vmAssetEditor.HasAssetValidationWarning();
    if (m_pErrorIndicator->IsShown() != (bError || bWarning))
    {
        m_pErrorIndicator->Show(bError || bWarning);
        m_pErrorIndicator->GetContainingSizer()->Layout();
    }
    m_pErrorIndicator->SetForegroundColour(bError ? *wxRED : wxColour(0xD0, 0xA0, 0x00));
    m_pErrorIndicator->SetToolTip(ToWx(bError ? m_vmAssetEditor.GetAssetValidationError()
                                              : m_vmAssetEditor.GetAssetValidationWarning()));

    // the trigger asks for a new or changed group to be scrolled into view
    const int nGroup = m_pBinder.Access(m_vmAssetEditor.Trigger()).GetValue(TriggerViewModel::EnsureVisibleGroupIndexProperty);
    if (nGroup != m_nEnsureVisibleGroup)
    {
        m_nEnsureVisibleGroup = nGroup;
        m_pGroups->EnsureIndexVisible(nGroup);
    }
}

void AssetEditorDialog::UpdatePoints()
{
    // the valid point values, plus the current value if it's not one of them (AchievementPointsBinding)
    const int nPoints = m_vmAssetEditor.GetPoints();
    std::vector<std::pair<int, std::wstring>> vItems;
    bool bFound = false;
    for (const int nValidPoints : AchievementModel::ValidPoints())
    {
        if (!bFound && nPoints < nValidPoints)
        {
            vItems.emplace_back(nPoints, std::to_wstring(nPoints));
            bFound = true;
        }
        if (nValidPoints == nPoints)
            bFound = true;
        vItems.emplace_back(nValidPoints, std::to_wstring(nValidPoints));
    }
    if (!bFound)
        vItems.emplace_back(nPoints, std::to_wstring(nPoints));

    m_pPoints->Update(std::move(vItems), nPoints);
}

void AssetEditorDialog::UpdateBadge()
{
    const std::wstring& sBadge = m_vmAssetEditor.GetBadge();
    const bool bLocal = ra::util::String::StartsWith(sBadge, L"local\\");
    if (!m_pBadge->HasFocus())
        SetTextIfChanged(*m_pBadge, bLocal ? std::wstring(L"[local]") : sBadge);

    if (sBadge != m_sDisplayedBadge)
    {
        m_sDisplayedBadge = sBadge;
        m_bBadgeImageLoaded = false;
        m_pBadgeImage->SetBitmap(wxNullBitmap);
    }

    if (m_bBadgeImageLoaded || sBadge.empty())
        return;

    // the badge is downloaded in the background; show it once it's available
    auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
    const auto nType = bLocal ? ra::ui::ImageType::Local : ra::ui::ImageType::Badge;
    const std::string sName = ra::util::String::Narrow(bLocal ? sBadge.substr(6) : sBadge);
    if (!pImageRepository.IsImageAvailable(nType, sName))
    {
        pImageRepository.FetchImage(nType, sName, "");
        if (!m_pBadgeTimer.IsRunning())
            m_pBadgeTimer.StartOnce(500); // check again later
        return;
    }

    wxImage pImage;
    wxLogNull pNoLog; // an incomplete or invalid download just shows no image
    if (pImage.LoadFile(ToWx(pImageRepository.GetFilename(nType, sName))))
    {
        const int nSize = FromDIP(BADGE_SIZE);
        pImage.Rescale(nSize, nSize, wxIMAGE_QUALITY_HIGH);
        m_pBadgeImage->SetBitmap(wxBitmap(pImage));
    }
    m_bBadgeImageLoaded = true;
}

void AssetEditorDialog::ApplyBadgeText()
{
    const std::wstring sValue = FromWx(m_pBadge->GetValue());
    if (sValue == L"[local]" || sValue == m_vmAssetEditor.GetBadge())
        return;

    // badges are numbers, padded to five digits (BadgeNameBinding)
    std::wstring sError;
    unsigned int nBadge = 0;
    if (!sValue.empty() && !ra::ParseUnsignedInt(sValue, 0xFFFFFFFF, nBadge, sError))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(sError);
        return;
    }

    std::wstring sBadge = sValue;
    if (!sBadge.empty() && sBadge.length() < 5)
        sBadge.insert(0, 5 - sBadge.length(), L'0');
    m_vmAssetEditor.SetBadge(sBadge);
}

void AssetEditorDialog::OnActiveClicked()
{
    if (m_pActive->GetValue())
    {
        if (m_vmAssetEditor.HasAssetValidationError())
        {
            m_pActive->SetValue(false);
            const auto sError = ra::util::String::Printf(L"The following errors must be corrected:\n* %s",
                                                         m_vmAssetEditor.GetAssetValidationError());
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Unable to activate", sError);
            return;
        }

        m_vmAssetEditor.SetState(ra::data::models::AssetState::Waiting);
    }
    else
    {
        m_vmAssetEditor.SetState(ra::data::models::AssetState::Inactive);
    }
}

void AssetEditorDialog::ShowConditionAddressInInspector(gsl::index nIndex)
{
    // like right clicking an address in the Win32 editor: show it in the memory inspector
    auto& vConditions = m_vmAssetEditor.Trigger().Conditions();
    const auto* vmCondition = vConditions.GetItemAt(nIndex);
    if (vmCondition == nullptr)
        return;

    using OperandType = ra::data::Requirement::OperandType;
    auto IsAddress = [](int nType) {
        const auto nOperandType = ra::itoe<OperandType>(nType);
        return nOperandType != OperandType::Value && nOperandType != OperandType::Float &&
               nOperandType != OperandType::Recall;
    };

    wxMenu pMenu;
    const bool bSource = vConditions.GetItemValue(nIndex, TriggerConditionViewModel::HasSourceValueProperty) &&
                         IsAddress(vConditions.GetItemValue(nIndex, TriggerConditionViewModel::SourceTypeProperty));
    const bool bTarget = vConditions.GetItemValue(nIndex, TriggerConditionViewModel::HasTargetProperty) &&
                         vConditions.GetItemValue(nIndex, TriggerConditionViewModel::HasTargetValueProperty) &&
                         IsAddress(vConditions.GetItemValue(nIndex, TriggerConditionViewModel::TargetTypeProperty));
    if (!bSource && !bTarget)
        return;

    constexpr int ID_SOURCE = wxID_HIGHEST + 1;
    constexpr int ID_TARGET = wxID_HIGHEST + 2;
    if (bSource)
        pMenu.Append(ID_SOURCE, "Show memory address in Memory Inspector");
    if (bTarget)
        pMenu.Append(ID_TARGET, "Show target address in Memory Inspector");

    const int nSelection = GetPopupMenuSelectionFromUser(pMenu);
    if (nSelection != ID_SOURCE && nSelection != ID_TARGET)
        return;

    const auto& sAddress = vConditions.GetItemValue(
        nIndex, nSelection == ID_SOURCE ? TriggerConditionViewModel::SourceValueProperty
                                        : TriggerConditionViewModel::TargetValueProperty);
    unsigned int nAddress = 0;
    std::wstring sError;
    if (!ra::ParseHex(sAddress, 0xFFFFFFFF, nAddress, sError))
        return;

    if (vConditions.GetItemValue(nIndex, TriggerConditionViewModel::IsIndirectProperty))
    {
        std::wstring sPointerChain;
        const ra::data::models::MemoryNoteModel* pNote = nullptr;
        nAddress = vmCondition->GetIndirectAddress(nAddress, sPointerChain, &pNote);
    }

    auto& pMemoryInspector = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector;
    pMemoryInspector.SetCurrentAddress(nAddress);
    if (!pMemoryInspector.IsVisible())
        pMemoryInspector.Show();
}

} // namespace wx
} // namespace ui
} // namespace ra
