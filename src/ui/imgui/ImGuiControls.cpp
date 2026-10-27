#include "ImGuiControls.hh"

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <algorithm>
#include <unordered_map>

namespace ra {
namespace ui {
namespace imgui {

bool BoundInputText(const char* sId, const std::wstring& sValue, std::wstring& sNewValue, ImGuiInputTextFlags nFlags,
                    const ImVec2& vSize)
{
    // ImGui keeps its own copy of the text while a text box is being edited; the buffer only has to outlive the
    // call. Refresh it from the view model whenever the user isn't editing it.
    static std::unordered_map<ImGuiID, std::string> s_mBuffers;

    const ImGuiID nId = ImGui::GetID(sId);
    auto& sBuffer = s_mBuffers[nId];
    if (ImGui::GetActiveID() != nId)
        sBuffer = ToUtf8(sValue);

    const bool bChanged = (vSize.y > 0.0f) ? ImGui::InputTextMultiline(sId, &sBuffer, vSize, nFlags)
                                           : ImGui::InputText(sId, &sBuffer, nFlags);
    if (!bChanged)
        return false;

    sNewValue = FromUtf8(sBuffer);
    return true;
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

ImU32 ToImGuiColor(uint32_t nARGB)
{
    if ((nARGB >> 24) == 0)
        return 0;

    return IM_COL32((nARGB >> 16) & 0xFF, (nARGB >> 8) & 0xFF, nARGB & 0xFF, nARGB >> 24);
}

} // namespace imgui
} // namespace ui
} // namespace ra
