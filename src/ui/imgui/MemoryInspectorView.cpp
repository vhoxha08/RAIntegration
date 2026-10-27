#include "MemoryInspectorView.hh"

#include "ui/imgui/ImGuiControls.hh"

#include <imgui.h>

namespace ra {
namespace ui {
namespace imgui {

using ra::ui::viewmodels::MemoryInspectorViewModel;
using ra::ui::viewmodels::MemorySearchViewModel;

// MemorySearchViewModel keeps SEARCH_ROWS_DISPLAYED (9) rows, one more than are shown
constexpr int VISIBLE_RESULT_ROWS = 8;

ImVec2 MemoryInspectorView::GetInitialSize() const
{
    return ImVec2(ImGui::GetFontSize() * 44.0f, ImGui::GetFontSize() * 46.0f);
}

void MemoryInspectorView::RenderContents()
{
    RenderSearch();
    RenderResults();
    RenderNotes();
    RenderViewerOptions();

    const float fHeight = ImGui::GetContentRegionAvail().y;
    m_pViewer.Render(std::max(fHeight, ImGui::GetFontSize() * 6.0f));
}

void MemoryInspectorView::RenderSearch()
{
    auto& vmSearch = m_vmMemoryInspector.Search();
    const float fFontSize = ImGui::GetFontSize();

    if (ImGui::BeginTable("##search", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("New Search", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        ImGui::TableSetupColumn("Filter", ImGuiTableColumnFlags_WidthStretch, 0.55f);
        ImGui::TableHeadersRow();
        ImGui::TableNextRow();

        // New Search
        ImGui::TableNextColumn();
        ImGui::BeginDisabled(!vmSearch.CanBeginNewSearch());
        int nRange = vmSearch.GetPredefinedFilterRange();
        ImGui::SetNextItemWidth(fFontSize * 6.0f);
        if (LookupCombo("##ranges", vmSearch.PredefinedFilterRanges(), nRange))
            vmSearch.SetPredefinedFilterRange(nRange);
        ImGui::SameLine();
        std::wstring sValue;
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (BoundInputText("##range", vmSearch.GetFilterRange(), sValue))
            vmSearch.SetFilterRange(sValue);

        int nSearchType = ra::etoi(vmSearch.GetSearchType());
        ImGui::SetNextItemWidth(fFontSize * 6.0f);
        if (LookupCombo("##searchtype", vmSearch.SearchTypes(), nSearchType))
            vmSearch.SetSearchType(ra::itoe<ra::services::SearchType>(nSearchType));
        ImGui::SameLine();
        ImGui::BeginDisabled(!vmSearch.CanAlign());
        bool bAligned = vmSearch.IsAligned();
        if (ImGui::Checkbox("Aligned", &bAligned))
            vmSearch.SetAligned(bAligned);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("New Search"))
            RunAfterFrame([&vmSearch]() { vmSearch.BeginNewSearch(); });
        ImGui::EndDisabled();

        // Filter
        ImGui::TableNextColumn();
        const bool bCanFilter = vmSearch.CanFilter();
        const float fButtonWidth = fFontSize * 10.5f;
        ImGui::BeginDisabled(!bCanFilter);
        int nComparison = ra::etoi(vmSearch.GetComparisonType());
        ImGui::SetNextItemWidth(fFontSize * 3.0f);
        if (LookupCombo("##comparison", vmSearch.ComparisonTypes(), nComparison))
            vmSearch.SetComparisonType(ra::itoe<ComparisonType>(nComparison));
        ImGui::SameLine();
        int nValueType = ra::etoi(vmSearch.GetValueType());
        ImGui::SetNextItemWidth(-fButtonWidth - ImGui::GetStyle().ItemSpacing.x);
        if (LookupCombo("##valuetype", vmSearch.ValueTypes(), nValueType))
            vmSearch.SetValueType(ra::itoe<ra::services::SearchFilterType>(nValueType));
        ImGui::SameLine();
        if (ImGui::Button("Filter Once", ImVec2(fButtonWidth, 0.0f)))
            RunAfterFrame([&vmSearch]() { vmSearch.ApplyFilter(); });
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!vmSearch.IsFilterValueEditable());
        ImGui::SetNextItemWidth(-fButtonWidth - ImGui::GetStyle().ItemSpacing.x);
        if (BoundInputText("##filtervalue", vmSearch.GetFilterValue(), sValue))
            vmSearch.SetFilterValue(sValue);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(!vmSearch.CanContinuousFilter());
        const std::string sContinuousLabel =
            ToUtf8(vmSearch.GetContinuousFilterLabel()) + "##continuous";
        if (ImGui::Button(sContinuousLabel.c_str(), ImVec2(fButtonWidth, 0.0f)))
            vmSearch.ToggleContinuousFilter();
        ImGui::EndDisabled();

        ImGui::EndTable();
    }
}

void MemoryInspectorView::RenderResults()
{
    auto& vmSearch = m_vmMemoryInspector.Search();
    const float fFontSize = ImGui::GetFontSize();
    const float fRowHeight = ImGui::GetTextLineHeightWithSpacing();
    const float fListHeight = fRowHeight * VISIBLE_RESULT_ROWS + ImGui::GetStyle().WindowPadding.y * 2.0f;

    ImGui::SeparatorText("Results");

    // left column: summary and commands
    const float fButtonsWidth = fFontSize * 10.5f;
    ImGui::BeginGroup();
    ImGui::Text("Count: %s", ToUtf8(vmSearch.GetResultCountText()).c_str());
    ImGui::Text("Filter: %s", ToUtf8(vmSearch.GetFilterSummary()).c_str());

    const float fSmallButton = ImGui::GetFrameHeight() * 1.2f;
    ImGui::BeginDisabled(!vmSearch.CanGoToPreviousPage());
    if (ImGui::Button("<<", ImVec2(fSmallButton, 0.0f)))
        RunAfterFrame([&vmSearch]() { vmSearch.PreviousPage(); });
    ImGui::EndDisabled();
    ImGui::SameLine();
    const std::string sPage = ToUtf8(vmSearch.GetSelectedPage());
    const float fPageWidth = fButtonsWidth - fSmallButton * 2.0f - ImGui::GetStyle().ItemSpacing.x * 2.0f;
    const float fTextWidth = ImGui::CalcTextSize(sPage.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (fPageWidth - fTextWidth) / 2.0f));
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(sPage.c_str());
    ImGui::SameLine();
    ImGui::SetCursorPosX(fButtonsWidth - fSmallButton + ImGui::GetStyle().WindowPadding.x);
    ImGui::BeginDisabled(!vmSearch.CanGoToNextPage());
    if (ImGui::Button(">>", ImVec2(fSmallButton, 0.0f)))
        RunAfterFrame([&vmSearch]() { vmSearch.NextPage(); });
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!vmSearch.HasSelection());
    if (ImGui::Button("Exclude Selected", ImVec2(fButtonsWidth, 0.0f)))
    {
        RunAfterFrame([this, &vmSearch]() {
            vmSearch.ExcludeSelected();
            m_nSelectionAnchor = -1;
        });
    }
    if (ImGui::Button("Bookmark Selected", ImVec2(fButtonsWidth, 0.0f)))
        RunAfterFrame([&vmSearch]() { vmSearch.BookmarkSelected(); });
    ImGui::EndDisabled();

    const float fHalfButton = (fButtonsWidth - ImGui::GetStyle().ItemSpacing.x) / 2.0f;
    ImGui::BeginDisabled(!vmSearch.CanFilter());
    if (ImGui::Button("Export", ImVec2(fHalfButton, 0.0f)))
        RunAfterFrame([&vmSearch]() { vmSearch.ExportResults(); });
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!vmSearch.CanBeginNewSearch());
    if (ImGui::Button("Import", ImVec2(fHalfButton, 0.0f)))
        RunAfterFrame([&vmSearch]() { vmSearch.ImportResults(); });
    ImGui::EndDisabled();
    ImGui::EndGroup();

    // right column: the results. Like the Win32 list, it is virtual: the view model only holds the visible rows,
    // starting at ScrollOffset, so the scroll position drives ScrollOffset.
    ImGui::SameLine();
    const auto nTotal = vmSearch.GetScrollMaximum();
    if (ImGui::BeginChild("##results", ImVec2(0.0f, fListHeight), ImGuiChildFlags_Borders))
    {
        ImGui::Dummy(ImVec2(0.0f, fRowHeight * static_cast<float>(nTotal)));

        const auto nMaxOffset = (nTotal > VISIBLE_RESULT_ROWS) ? nTotal - VISIBLE_RESULT_ROWS : 0U;
        const auto nFirst = std::min(static_cast<unsigned>(ImGui::GetScrollY() / fRowHeight), nMaxOffset);
        if (nFirst != vmSearch.GetScrollOffset())
            vmSearch.SetScrollOffset(nFirst);

        // the view model may move the offset (i.e. a new filter preserves the position); follow it
        const auto nOffset = vmSearch.GetScrollOffset();
        if (nOffset != nFirst)
            ImGui::SetScrollY(static_cast<float>(nOffset) * fRowHeight);

        const float fAddressWidth = fFontSize * 5.5f;
        const float fValueWidth = fFontSize * 4.5f;
        auto& vResults = vmSearch.Results();
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vResults.Count()) && i < VISIBLE_RESULT_ROWS; ++i)
        {
            auto* pRow = vResults.GetItemAt(i);
            if (pRow == nullptr)
                continue;

            const gsl::index nIndex = gsl::narrow_cast<gsl::index>(nOffset) + i;
            const float fY = static_cast<float>(nIndex) * fRowHeight;
            ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().WindowPadding.x, fY + ImGui::GetStyle().WindowPadding.y));

            const ImVec2 vRowPos = ImGui::GetCursorScreenPos();
            const ImU32 nRowColor = ToImGuiColor(pRow->GetRowColor().ARGB);
            if (nRowColor != 0)
            {
                ImGui::GetWindowDrawList()->AddRectFilled(
                    vRowPos, ImVec2(vRowPos.x + ImGui::GetContentRegionAvail().x, vRowPos.y + fRowHeight), nRowColor);
            }

            ImGui::PushID(gsl::narrow_cast<int>(nIndex));
            if (ImGui::Selectable("##row", pRow->IsSelected(), ImGuiSelectableFlags_None,
                                  ImVec2(0.0f, fRowHeight - ImGui::GetStyle().ItemSpacing.y)))
            {
                const auto& io = ImGui::GetIO();
                if (io.KeyShift && m_nSelectionAnchor >= 0)
                {
                    vmSearch.SelectRange(0, gsl::narrow_cast<gsl::index>(nTotal) - 1, false);
                    vmSearch.SelectRange(std::min(m_nSelectionAnchor, nIndex), std::max(m_nSelectionAnchor, nIndex),
                                         true);
                }
                else if (io.KeyCtrl || io.KeySuper)
                {
                    vmSearch.SelectRange(nIndex, nIndex, !pRow->IsSelected());
                    m_nSelectionAnchor = nIndex;
                }
                else
                {
                    vmSearch.SelectRange(0, gsl::narrow_cast<gsl::index>(nTotal) - 1, false);
                    vmSearch.SelectRange(nIndex, nIndex, true);
                    m_nSelectionAnchor = nIndex;

                    ra::services::SearchResult pResult{};
                    if (vmSearch.GetResult(nIndex, pResult))
                        m_vmMemoryInspector.SetCurrentAddress(pResult.nAddress);
                }
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
            {
                const auto sTooltip = vmSearch.GetTooltip(*pRow);
                if (!sTooltip.empty())
                    ImGui::SetTooltip("%s", ToUtf8(sTooltip).c_str());
            }

            ImGui::SameLine(ImGui::GetStyle().WindowPadding.x);
            ImGui::TextUnformatted(ToUtf8(pRow->GetAddress()).c_str());
            ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + fAddressWidth);
            ImGui::TextUnformatted(ToUtf8(pRow->GetCurrentValue()).c_str());
            ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + fAddressWidth + fValueWidth);
            const ImU32 nDescriptionColor = ToImGuiColor(pRow->GetDescriptionColor().ARGB);
            if (nDescriptionColor != 0)
                ImGui::PushStyleColor(ImGuiCol_Text, nDescriptionColor);
            ImGui::TextUnformatted(ToUtf8(pRow->GetDescription()).c_str());
            if (nDescriptionColor != 0)
                ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void MemoryInspectorView::RenderNotes()
{
    auto& vm = m_vmMemoryInspector;
    const float fFontSize = ImGui::GetFontSize();
    const float fLeftWidth = fFontSize * 10.5f;
    const float fRightWidth = fFontSize * 4.5f;
    const float fNoteHeight = (m_bNotesExpanded ? 12.0f : 4.0f) * ImGui::GetTextLineHeight() +
                              ImGui::GetStyle().FramePadding.y * 2.0f;

    ImGui::SeparatorText("Memory Notes");

    ImGui::BeginGroup();
    ImGui::TextUnformatted("Address:");
    std::wstring sValue;
    ImGui::SetNextItemWidth(fLeftWidth - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
    if (BoundInputText("##address", vm.GetCurrentAddressText(), sValue))
        vm.SetCurrentAddressText(sValue);
    if (ImGui::IsItemFocused())
    {
        // like the Win32 dialog, up/down in the address field move between notes
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            vm.PreviousNote();
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            vm.NextNote();
    }
    ImGui::SameLine();
    if (ImGui::Button("...", ImVec2(ImGui::GetFrameHeight(), 0.0f)))
        RunAfterFrame([&vm]() { vm.OpenNotesList(); });
    if (ImGui::Button("Add Bookmark", ImVec2(fLeftWidth, 0.0f)))
        vm.BookmarkCurrentAddress();
    ImGui::EndGroup();

    ImGui::SameLine();
    ImGui::BeginDisabled(!vm.CanEditCurrentAddressNote());
    ImGuiInputTextFlags nFlags = 0;
    if (vm.IsCurrentAddressNoteReadOnly())
        nFlags |= ImGuiInputTextFlags_ReadOnly;
    if (BoundInputText("##note", vm.GetCurrentAddressNote(), sValue, nFlags,
                       ImVec2(-fRightWidth - ImGui::GetStyle().ItemSpacing.x, fNoteHeight)))
    {
        vm.SetCurrentAddressNote(sValue);
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::BeginDisabled(!vm.CanPublishCurrentAddressNote());
    if (ImGui::Button("Publish", ImVec2(fRightWidth, 0.0f)))
        RunAfterFrame([&vm]() { vm.PublishCurrentAddressNote(); });
    ImGui::EndDisabled();
    ImGui::BeginDisabled(!vm.CanRevertCurrentAddressNote());
    if (ImGui::Button("Revert", ImVec2(fRightWidth, 0.0f)))
        RunAfterFrame([&vm]() { vm.RevertCurrentAddressNote(); });
    ImGui::EndDisabled();
    if (ImGui::ArrowButton("##expand", m_bNotesExpanded ? ImGuiDir_Up : ImGuiDir_Down))
        m_bNotesExpanded = !m_bNotesExpanded;
    ImGui::EndGroup();
}

void MemoryInspectorView::RenderViewerOptions()
{
    auto& vmViewer = m_vmMemoryInspector.Viewer();

    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Memory View:");

    const auto nSize = vmViewer.GetSize();
    auto SizeOption = [&vmViewer, nSize](const char* sLabel, ra::data::Memory::Size nOption) {
        ImGui::SameLine();
        if (ImGui::RadioButton(sLabel, nSize == nOption))
            vmViewer.SetSize(nOption);
    };
    SizeOption("8-bit", ra::data::Memory::Size::EightBit);
    SizeOption("16-bit", ra::data::Memory::Size::SixteenBit);
    SizeOption("32-bit", ra::data::Memory::Size::ThirtyTwoBit);
    SizeOption("32-bit BE", ra::data::Memory::Size::ThirtyTwoBitBigEndian);

    // " 0 0 0 0 0 0 0 0": bit 7 first. Like the Win32 dialog, double clicking a bit toggles it.
    const std::string sBits = ToUtf8(m_vmMemoryInspector.GetCurrentAddressBits());
    if (m_vmMemoryInspector.AreCurrentBitsVisible() && sBits.length() >= 16)
    {
        ImGui::SameLine();
        ImGui::TextUnformatted("Bits:");
        for (int nBit = 7; nBit >= 0; --nBit)
        {
            const size_t nIndex = gsl::narrow_cast<size_t>((7 - nBit) * 2 + 1);
            const char sBit[2] = {sBits.at(nIndex), '\0'};

            ImGui::SameLine(0.0f, 2.0f);
            ImGui::PushID(nBit);
            ImGui::Selectable(sBit, false, ImGuiSelectableFlags_AllowDoubleClick, ImGui::CalcTextSize("0"));
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Bit %d (double click to toggle)", nBit);
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    m_vmMemoryInspector.ToggleBit(nBit);
            }
            ImGui::PopID();
        }
    }
}

} // namespace imgui
} // namespace ui
} // namespace ra
