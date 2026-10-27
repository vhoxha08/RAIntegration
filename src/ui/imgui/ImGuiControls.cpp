#include "ImGuiControls.hh"

#include "ui/imgui/ImGuiWindowView.hh"

#include <imgui.h>

#include <algorithm>

namespace ra {
namespace ui {
namespace imgui {

bool LookupCombo(const char* sId, const ra::ui::viewmodels::LookupItemViewModelCollection& vItems, int& nSelectedId)
{
    std::string sPreview;
    for (const auto& pItem : vItems)
    {
        if (pItem.GetId() == nSelectedId)
        {
            sPreview = ToUtf8(pItem.GetLabel());
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

bool IsEscapePressed()
{
    return ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsKeyPressed(ImGuiKey_Escape, false);
}

void AlignButtonsRight(int nButtons, float fButtonWidth)
{
    const float fWidth = fButtonWidth * static_cast<float>(nButtons) +
                         ImGui::GetStyle().ItemSpacing.x * static_cast<float>(nButtons - 1);
    // never move left: in auto-sized windows the content region is only as wide as the previous frame's content
    ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - fWidth));
}

} // namespace imgui
} // namespace ui
} // namespace ra
