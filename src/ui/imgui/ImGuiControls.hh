#ifndef RA_UI_IMGUI_CONTROLS_HH
#define RA_UI_IMGUI_CONTROLS_HH
#pragma once

#include "ui/imgui/ImGuiWindowView.hh"
#include "ui/viewmodels/LookupItemViewModel.hh"

#include <imgui.h>

namespace ra {
namespace ui {
namespace imgui {

/// <summary>
/// Drop down list of a collection of lookup items (the counterpart of the Win32 ComboBoxBinding).
/// </summary>
/// <returns><c>true</c> if the user picked a different item; <paramref name="nSelectedId" /> is updated.</returns>
template<class TCollection>
bool LookupCombo(const char* sId, const TCollection& vItems, int& nSelectedId)
{
    std::string sPreview;
    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vItems.Count()); ++i)
    {
        const auto* pItem = vItems.GetItemAt(i);
        if (pItem != nullptr && pItem->GetId() == nSelectedId)
        {
            sPreview = ToUtf8(pItem->GetLabel());
            break;
        }
    }

    bool bChanged = false;
    if (ImGui::BeginCombo(sId, sPreview.c_str()))
    {
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(vItems.Count()); ++i)
        {
            const auto* pItem = vItems.GetItemAt(i);
            if (pItem == nullptr)
                continue;

            const bool bSelected = (pItem->GetId() == nSelectedId);
            ImGui::PushID(gsl::narrow_cast<int>(i));
            if (ImGui::Selectable(ToUtf8(pItem->GetLabel()).c_str(), bSelected) && !bSelected)
            {
                nSelectedId = pItem->GetId();
                bChanged = true;
            }
            if (bSelected)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    return bChanged;
}

/// <summary>
/// Text box bound to a view model string (the counterpart of the Win32 TextBoxBinding). The text is taken from
/// <paramref name="sValue" /> unless the user is editing it. A height in <paramref name="vSize" /> makes it
/// multi-line.
/// </summary>
/// <returns><c>true</c> if the user changed the text; <paramref name="sNewValue" /> holds the new text.</returns>
bool BoundInputText(const char* sId, const std::wstring& sValue, std::wstring& sNewValue,
                    ImGuiInputTextFlags nFlags = 0, const ImVec2& vSize = ImVec2(0.0f, 0.0f));

/// <summary>
/// Whether Escape was pressed while the current window (or one of its children) has focus.
/// </summary>
bool IsEscapePressed();

/// <summary>
/// Moves the cursor so <paramref name="nButtons" /> buttons of <paramref name="fButtonWidth" /> end at the right edge
/// (but not left of the current position).
/// </summary>
void AlignButtonsRight(int nButtons, float fButtonWidth);

/// <summary>
/// Converts a toolkit ARGB color to an ImGui color. Returns 0 for Color::Transparent (no color).
/// </summary>
ImU32 ToImGuiColor(uint32_t nARGB);

} // namespace imgui
} // namespace ui
} // namespace ra

#endif // !RA_UI_IMGUI_CONTROLS_HH
