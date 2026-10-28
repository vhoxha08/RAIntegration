#ifndef RA_UI_WX_MEMORY_WATCH_COLUMNS_HH
#define RA_UI_WX_MEMORY_WATCH_COLUMNS_HH
#pragma once

#include "ui/wx/WxGrid.hh"

namespace ra {
namespace ui {
namespace wx {

/// <summary>
/// Value of a MemoryWatchViewModel. When editable, edits write memory (unless the watch is read-only) and invalid
/// input is reported (the counterpart of GridMemoryWatchValueColumnBinding).
/// </summary>
GridColumn MemoryWatchValueColumn(const std::wstring& sHeader, const StringModelProperty& pProperty, bool bEditable);

/// <summary>
/// Format of a MemoryWatchViewModel; not applicable to floats and text (GridMemoryWatchFormatColumnBinding).
/// </summary>
GridColumn MemoryWatchFormatColumn(const std::wstring& sHeader, const IntModelProperty& pProperty,
                                   const ra::ui::viewmodels::LookupItemViewModelCollection& vFormats);

/// <summary>
/// Address of a MemoryWatchViewModel, in parentheses if it is indirect (GridIndirectAddressColumnBinding).
/// </summary>
GridColumn MemoryWatchAddressColumn(const std::wstring& sHeader, const IntModelProperty& pProperty);

} // namespace wx
} // namespace ui
} // namespace ra

#endif // !RA_UI_WX_MEMORY_WATCH_COLUMNS_HH
