#include "MemoryViewerWidget.hh"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#include "ui/EditorTheme.hh"
#include "ui/imgui/ImGuiControls.hh"
#include "ui/imgui/ImGuiSurface.hh"

#include <imgui.h>
#include <imgui_internal.h>

namespace ra {
namespace ui {
namespace imgui {

constexpr int MEMVIEW_MARGIN = 4;

MemoryViewerWidget::~MemoryViewerWidget() noexcept
{
    if (m_bFocused)
    {
        try
        {
            m_vmViewer.OnLostFocus();
        }
        catch (...)
        {
        }
    }
}

void MemoryViewerWidget::SetFocused(bool bFocused)
{
    if (m_bFocused == bFocused)
        return;

    m_bFocused = bFocused;
    if (bFocused)
        m_vmViewer.OnGotFocus();
    else
        m_vmViewer.OnLostFocus();
}

void MemoryViewerWidget::Render(float fHeight)
{
    const int nWidth = static_cast<int>(ImGui::GetContentRegionAvail().x);
    const int nHeight = static_cast<int>(fHeight);
    if (nWidth <= MEMVIEW_MARGIN * 2 || nHeight <= MEMVIEW_MARGIN * 2)
        return;

    if (nWidth != m_nWidth || nHeight != m_nHeight)
    {
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        m_vmViewer.OnResized(nWidth - MEMVIEW_MARGIN * 2, nHeight - MEMVIEW_MARGIN * 2);
    }

    m_vmViewer.UpdateRenderImage();

    const ImVec2 vPos = ImGui::GetCursorScreenPos();
    const ImVec2 vSize(static_cast<float>(nWidth), static_cast<float>(nHeight));
    ImGui::InvisibleButton("##memoryviewer", vSize);
    const ImGuiID nId = ImGui::GetItemID();
    const bool bHovered = ImGui::IsItemHovered();

    // the viewer has keyboard focus from a click on it until a click elsewhere or another window gets focus
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !bHovered)
        SetFocused(false);
    else if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        SetFocused(false);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        SetFocused(true);
        if (!m_vmViewer.IsAddressFixed())
        {
            const ImVec2 vMouse = ImGui::GetMousePos();
            m_vmViewer.OnClick(static_cast<int>(vMouse.x - vPos.x) - MEMVIEW_MARGIN,
                               static_cast<int>(vMouse.y - vPos.y) - MEMVIEW_MARGIN);
        }
    }

    if (bHovered && !m_vmViewer.IsAddressFixed())
    {
        const float fWheel = ImGui::GetIO().MouseWheel;
        if (fWheel > 0.0f)
            m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() - 32);
        else if (fWheel < 0.0f)
            m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() + 32);
    }

    if (m_bFocused)
    {
        // keep the keys from moving ImGui's keyboard navigation or reaching the emulator
        for (const ImGuiKey nKey : {ImGuiKey_LeftArrow, ImGuiKey_RightArrow, ImGuiKey_UpArrow, ImGuiKey_DownArrow,
                                    ImGuiKey_PageUp, ImGuiKey_PageDown, ImGuiKey_Home, ImGuiKey_End})
        {
            ImGui::SetKeyOwner(nKey, nId);
        }
        ImGui::SetNextFrameWantCaptureKeyboard(true);

        bool bHandled = false;
        if (!m_vmViewer.IsAddressFixed())
            bHandled = HandleNavigation();
        if (!bHandled)
            HandleShortcut();

        for (const ImWchar c : ImGui::GetIO().InputQueueCharacters)
        {
            if (c < 0x80)
                m_vmViewer.OnChar(static_cast<char>(c));
        }

        m_vmViewer.UpdateRenderImage();
    }

    auto* pDrawList = ImGui::GetWindowDrawList();
    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    pDrawList->AddRectFilled(vPos, ImVec2(vPos.x + vSize.x, vPos.y + vSize.y),
                             ToImGuiColor(pEditorTheme.ColorBackground().ARGB | 0xFF000000));

    const auto* pSurface = dynamic_cast<const ImGuiSurface*>(&m_vmViewer.GetRenderImage());
    if (pSurface != nullptr)
    {
        pDrawList->PushClipRect(vPos, ImVec2(vPos.x + vSize.x, vPos.y + vSize.y), true);
        pSurface->Replay(*pDrawList, vPos.x + MEMVIEW_MARGIN, vPos.y + MEMVIEW_MARGIN);
        pDrawList->PopClipRect();
    }

    if (m_bFocused)
    {
        pDrawList->AddRect(vPos, ImVec2(vPos.x + vSize.x, vPos.y + vSize.y), ImGui::GetColorU32(ImGuiCol_NavCursor));
    }
}

bool MemoryViewerWidget::HandleNavigation()
{
    const bool bShiftHeld = ImGui::IsKeyDown(ImGuiMod_Shift);
    const bool bControlHeld = ImGui::IsKeyDown(ImGuiMod_Ctrl);
    const ImGuiID nOwner = ImGui::GetItemID();
    auto IsPressed = [nOwner](ImGuiKey nKey) { return ImGui::IsKeyPressed(nKey, ImGuiInputFlags_Repeat, nOwner); };

    if (IsPressed(ImGuiKey_RightArrow))
    {
        if (bShiftHeld || bControlHeld)
            m_vmViewer.AdvanceCursorWord();
        else
            m_vmViewer.AdvanceCursor();
        return true;
    }

    if (IsPressed(ImGuiKey_LeftArrow))
    {
        if (bShiftHeld || bControlHeld)
            m_vmViewer.RetreatCursorWord();
        else
            m_vmViewer.RetreatCursor();
        return true;
    }

    if (IsPressed(ImGuiKey_DownArrow))
    {
        if (bControlHeld)
            m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() + 0x10);
        else
            m_vmViewer.AdvanceCursorLine();
        return true;
    }

    if (IsPressed(ImGuiKey_UpArrow))
    {
        if (bControlHeld)
            m_vmViewer.SetFirstAddress(m_vmViewer.GetFirstAddress() - 0x10);
        else
            m_vmViewer.RetreatCursorLine();
        return true;
    }

    if (IsPressed(ImGuiKey_PageUp))
    {
        m_vmViewer.RetreatCursorPage();
        return true;
    }

    if (IsPressed(ImGuiKey_PageDown))
    {
        m_vmViewer.AdvanceCursorPage();
        return true;
    }

    if (IsPressed(ImGuiKey_Home))
    {
        if (bControlHeld)
        {
            m_vmViewer.SetFirstAddress(0);
            m_vmViewer.SetAddress(0);
        }
        else
        {
            m_vmViewer.SetAddress(m_vmViewer.GetAddress() & ~0x0F);
        }
        return true;
    }

    if (IsPressed(ImGuiKey_End))
    {
        if (bControlHeld)
        {
            const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
            const auto nTotalBytes = gsl::narrow<ra::data::ByteAddress>(pMemoryContext.TotalMemorySize());

            m_vmViewer.SetFirstAddress(nTotalBytes & ~0x0F);
            m_vmViewer.SetAddress(nTotalBytes - 1);
        }
        else
        {
            switch (m_vmViewer.GetSize())
            {
                case ra::data::Memory::Size::ThirtyTwoBit:
                    m_vmViewer.SetAddress((m_vmViewer.GetAddress() & ~0x0F) | 0x0C);
                    break;

                case ra::data::Memory::Size::SixteenBit:
                    m_vmViewer.SetAddress((m_vmViewer.GetAddress() & ~0x0F) | 0x0E);
                    break;

                default:
                    m_vmViewer.SetAddress(m_vmViewer.GetAddress() | 0x0F);
                    break;
            }
        }
        return true;
    }

    return false;
}

bool MemoryViewerWidget::HandleShortcut()
{
    // Increase/decrease by 1 or 2 on the high and lower nibble depending on the modifiers
    const bool bIncrease = ImGui::IsKeyPressed(ImGuiKey_KeypadAdd);
    const bool bDecrease = ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract);
    if (!bIncrease && !bDecrease)
        return false;

    uint32_t nModifier = 1;
    if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
        nModifier *= 2;
    if (ImGui::IsKeyDown(ImGuiMod_Shift))
        nModifier *= 16;

    if (bIncrease)
        m_vmViewer.IncreaseCurrentValue(nModifier);
    else
        m_vmViewer.DecreaseCurrentValue(nModifier);

    return true;
}

} // namespace imgui
} // namespace ui
} // namespace ra
