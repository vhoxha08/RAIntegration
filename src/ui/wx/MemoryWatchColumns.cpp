#include "MemoryWatchColumns.hh"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#include "ui/viewmodels/MemoryWatchViewModel.hh"
#include "ui/viewmodels/MessageBoxViewModel.hh"

#include "util/Strings.hh"

namespace ra {
namespace ui {
namespace wx {

using ra::ui::viewmodels::MemoryWatchViewModel;

GridColumn MemoryWatchValueColumn(const std::wstring& sHeader, const StringModelProperty& pProperty, bool bEditable)
{
    GridColumn pColumn = GridColumn::Text(sHeader, pProperty, 72);
    pColumn.nAlignment = wxALIGN_CENTER;
    if (bEditable)
    {
        pColumn.fSetText = [](ViewModelCollectionBase& vItems, gsl::index nIndex, const std::wstring& sValue) {
            auto* vmItem = dynamic_cast<MemoryWatchViewModel*>(vItems.GetViewModelAt(nIndex));
            if (vmItem != nullptr)
            {
                std::wstring sError;
                if (!vmItem->SetCurrentValue(sValue, sError))
                {
                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input", sError);
                    return false;
                }
            }
            return true;
        };
        pColumn.fIsEditable = [](const ViewModelCollectionBase& vItems, gsl::index nIndex) {
            return !vItems.GetItemValue(nIndex, MemoryWatchViewModel::ReadOnlyProperty);
        };
    }
    return pColumn;
}

static bool HasFormat(const ViewModelCollectionBase& vItems, gsl::index nIndex)
{
    switch (ra::itoe<ra::data::Memory::Size>(vItems.GetItemValue(nIndex, MemoryWatchViewModel::SizeProperty)))
    {
        case ra::data::Memory::Size::Float:
        case ra::data::Memory::Size::FloatBigEndian:
        case ra::data::Memory::Size::Double32:
        case ra::data::Memory::Size::Double32BigEndian:
        case ra::data::Memory::Size::MBF32:
        case ra::data::Memory::Size::MBF32LE:
        case ra::data::Memory::Size::Text:
            return false;

        default:
            return true;
    }
}

GridColumn MemoryWatchFormatColumn(const std::wstring& sHeader, const IntModelProperty& pProperty,
                                   const ra::ui::viewmodels::LookupItemViewModelCollection& vFormats)
{
    GridColumn pColumn = GridColumn::Lookup(sHeader, pProperty, vFormats, 60, true);
    pColumn.fIsEditable = HasFormat;
    return pColumn;
}

GridColumn MemoryWatchAddressColumn(const std::wstring& sHeader, const IntModelProperty& pProperty)
{
    GridColumn pColumn = GridColumn::Custom(sHeader, 80, [&pProperty](const ViewModelCollectionBase& vItems,
                                                                     gsl::index nIndex) {
        const auto nAddress = gsl::narrow_cast<ra::data::ByteAddress>(vItems.GetItemValue(nIndex, pProperty));
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        const auto* vmItem = dynamic_cast<const MemoryWatchViewModel*>(vItems.GetViewModelAt(nIndex));
        if (vmItem != nullptr && vmItem->IsIndirectAddress())
            return ra::util::String::Printf(L"(%s)", pMemoryContext.FormatAddress(nAddress).substr(2));
        return ra::util::String::Widen(pMemoryContext.FormatAddress(nAddress));
    });
    pColumn.nAlignment = wxALIGN_RIGHT;
    return pColumn;
}

} // namespace wx
} // namespace ui
} // namespace ra
