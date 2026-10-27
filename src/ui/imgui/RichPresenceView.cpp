#include "RichPresenceView.hh"

#include <imgui.h>

namespace ra {
namespace ui {
namespace imgui {

ImVec2 RichPresenceView::GetInitialSize() const
{
    return ImVec2(ImGui::GetFontSize() * 20.0f, ImGui::GetFontSize() * 6.0f);
}

void RichPresenceView::RenderContents()
{
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(ToUtf8(m_vmRichPresence.GetDisplayString()).c_str());
    ImGui::PopTextWrapPos();
}

} // namespace imgui
} // namespace ui
} // namespace ra
