#include "EditorTheme.hh"
#include "OverlayTheme.hh"

#include "util/Json.hh"
#include "util/Log.hh"

#include "services/IFileSystem.hh"
#include "services/ServiceLocator.hh"

namespace ra {
namespace ui {

static void ReadColor(Color& nColor, const ra::util::Json::Reader::Node& pColors, const std::string& sFieldName)
{
    std::string sValue;
    if (pColors.TryGetString(sFieldName, sValue))
    {
        if (sValue.length() == 7 && sValue.at(0) == '#')
            sValue.erase(sValue.begin());

        if (sValue.length() == 6)
        {
            char* pEnd;
            const auto nValue = strtoul(sValue.c_str(), &pEnd, 16);
            if (pEnd && *pEnd == '\0')
                nColor = Color(nValue | 0xFF000000);
        }
    }
}

void OverlayTheme::LoadFromFile()
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    std::wstring sFullPath = pFileSystem.BaseDirectory() + L"Overlay\\theme.json";
    if (pFileSystem.GetFileSize(sFullPath) == -1)
        return;

    auto pFile = pFileSystem.OpenTextFile(sFullPath);
    if (!pFile)
        return;

    ra::util::Json::Reader pJson;
    if (!pJson.Parse(*pFile))
    {
        RA_LOG_ERR("Unable to read %s: %s (%zu)", L"Overlay\\theme.json", pJson.GetParseError(), pJson.GetParseErrorOffset());
        return;
    }

    ra::util::Json::Reader::Node pPopup;
    if (pJson.TryGetObject("Popup", pPopup))
    {
        pPopup.TryGetString("Font", m_sFontPopup);

        ra::util::Json::Reader::Node pFontSizes;
        if (pPopup.TryGetObject("FontSizes", pFontSizes))
        {
            pFontSizes.TryGetInteger("Title", m_nFontSizePopupTitle);
            pFontSizes.TryGetInteger("Subtitle", m_nFontSizePopupSubtitle);
            pFontSizes.TryGetInteger("Detail", m_nFontSizePopupDetail);
            pFontSizes.TryGetInteger("LeaderboardTitle", m_nFontSizePopupLeaderboardTitle);
            pFontSizes.TryGetInteger("LeaderboardEntry", m_nFontSizePopupLeaderboardEntry);
            pFontSizes.TryGetInteger("LeaderboardTracker", m_nFontSizePopupLeaderboardTracker);
        }

        ra::util::Json::Reader::Node pColors;
        if (pPopup.TryGetObject("Colors", pColors))
        {
            ReadColor(m_colorBackground, pColors, "Background");
            ReadColor(m_colorMasteryBackground, pColors, "MasteryBackground");
            ReadColor(m_colorNonHardcoreBackground, pColors, "NonHardcoreBackground");
            ReadColor(m_colorBorder, pColors, "Border");
            ReadColor(m_colorTextShadow, pColors, "TextShadow");
            ReadColor(m_colorTitle, pColors, "Title");
            ReadColor(m_colorDescription, pColors, "Description");
            ReadColor(m_colorDetail, pColors, "Detail");
            ReadColor(m_colorError, pColors, "Error");
            ReadColor(m_colorLeaderboardEntry, pColors, "LeaderboardEntry");
            ReadColor(m_colorLeaderboardPlayer, pColors, "LeaderboardPlayer");
        }
    }

    ra::util::Json::Reader::Node pOverlay;
    if (pJson.TryGetObject("Overlay", pOverlay))
    {
        pOverlay.TryGetString("Font", m_sFontOverlay);

        ra::util::Json::Reader::Node pFontSizes;
        if (pOverlay.TryGetObject("FontSizes", pFontSizes))
        {
            pFontSizes.TryGetInteger("Title", m_nFontSizeOverlayTitle);
            pFontSizes.TryGetInteger("Header", m_nFontSizeOverlayHeader);
            pFontSizes.TryGetInteger("Summary", m_nFontSizeOverlaySummary);
            pFontSizes.TryGetInteger("Detail", m_nFontSizeOverlayDetail);
        }

        ra::util::Json::Reader::Node pColors;
        if (pOverlay.TryGetObject("Colors", pColors))
        {
            ReadColor(m_colorOverlayPanel, pColors, "Panel");
            ReadColor(m_colorOverlayText, pColors, "Text");
            ReadColor(m_colorOverlayDisabledText, pColors, "DisabledText");
            ReadColor(m_colorOverlaySubText, pColors, "SubText");
            ReadColor(m_colorOverlayDisabledSubText, pColors, "DisabledSubText");
            ReadColor(m_colorOverlaySelectionBackground, pColors, "SelectionBackground");
            ReadColor(m_colorOverlaySelectionText, pColors, "SelectionText");
            ReadColor(m_colorOverlaySelectionDisabledText, pColors, "SelectionDisabledText");
            ReadColor(m_colorOverlayScrollBar, pColors, "ScrollBar");
            ReadColor(m_colorOverlayScrollBarGripper, pColors, "ScrollBarGripper");
        }
    }

    pJson.TryGetBoolean("Transparent", m_bTransparent);
}

void EditorTheme::ApplyDarkColors() noexcept
{
    m_colorBackground = Color(255, 30, 30, 30);
    m_colorSeparator = Color(255, 70, 70, 70);
    m_colorCursor = Color(255, 75, 75, 95);
    m_colorNormal = Color(255, 220, 220, 220);
    m_colorSelected = Color(255, 255, 110, 110);
    m_colorHasNote = Color(255, 110, 160, 255);
    m_colorHasSurrogateNote = Color(255, 130, 150, 210);
    m_colorHasBookmark = Color(255, 100, 210, 100);
    m_colorFrozen = Color(255, 255, 200, 0);
    m_colorHeader = Color(255, 140, 140, 140);
    m_colorHeaderSelected = Color(255, 255, 120, 120);

    m_colorNoteNormal = Color(255, 225, 225, 225);
    m_colorNoteModified = Color(255, 255, 130, 130);

    m_colorTriggerIsTrue = Color(255, 40, 90, 40);
    m_colorTriggerWasTrue = Color(255, 45, 45, 105);
    m_colorTriggerBecomingTrue = Color(255, 30, 90, 90);
    m_colorTriggerResetTrue = Color(255, 90, 90, 30);
    m_colorTriggerPauseTrue = Color(255, 105, 40, 40);

    m_colorExplainConflicting = Color(255, 105, 40, 40);
    m_colorExplainTriggerWhen = Color(255, 40, 90, 40);
    m_colorExplainWhile = Color(255, 45, 45, 105);
    m_colorExplainUnless = Color(255, 110, 70, 20);
    m_colorExplainStartingWhen = Color(255, 100, 90, 30);
    m_colorExplainFailingWhen = Color(255, 105, 40, 40);
    m_colorExplainImpotent = Color(255, 70, 70, 70);
}

void EditorTheme::LoadFromFile()
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    std::wstring sFullPath = pFileSystem.BaseDirectory() + L"Overlay\\editor_theme.json";
    if (pFileSystem.GetFileSize(sFullPath) == -1)
        return;

    auto pFile = pFileSystem.OpenTextFile(sFullPath);
    if (!pFile)
        return;

    ra::util::Json::Reader pJson;
    if (!pJson.Parse(*pFile))
    {
        RA_LOG_ERR("Unable to read %s: %s (%zu)", L"Overlay\\editor_theme.json", pJson.GetParseError(), pJson.GetParseErrorOffset());
        return;
    }

    ra::util::Json::Reader::Node pMemoryViewer;
    if (pJson.TryGetObject("MemoryViewer", pMemoryViewer))
    {
        pMemoryViewer.TryGetString("Font", m_sFontMemoryViewer);
        pMemoryViewer.TryGetInteger("FontSize", m_nFontSizeMemoryViewer);

        ra::util::Json::Reader::Node pColors;
        if (pMemoryViewer.TryGetObject("Colors", pColors))
        {
            ReadColor(m_colorBackground, pColors, "Background");
            ReadColor(m_colorSeparator, pColors, "Separator");
            ReadColor(m_colorCursor, pColors, "Cursor");
            ReadColor(m_colorNormal, pColors, "Normal");
            ReadColor(m_colorSelected, pColors, "Selected");
            ReadColor(m_colorHasNote, pColors, "HasNote");
            ReadColor(m_colorHasSurrogateNote, pColors, "HasSurrogateNote");
            ReadColor(m_colorHasBookmark, pColors, "HasBookmark");
            ReadColor(m_colorFrozen, pColors, "Frozen");
            ReadColor(m_colorHeader, pColors, "Header");
            ReadColor(m_colorHeaderSelected, pColors, "HeaderSelected");
        }
    }

    ra::util::Json::Reader::Node pCodeNotes;
    if (pJson.TryGetObject("CodeNotes", pCodeNotes))
    {
        ra::util::Json::Reader::Node pColors;
        if (pCodeNotes.TryGetObject("Colors", pColors))
        {
            ReadColor(m_colorNoteNormal, pColors, "Normal");
            ReadColor(m_colorNoteModified, pColors, "Modified");
        }
    }

    ra::util::Json::Reader::Node pTriggerColors;
    if (pJson.TryGetObject("TriggerColors", pTriggerColors))
    {
        ReadColor(m_colorTriggerIsTrue, pTriggerColors, "IsTrue");
        ReadColor(m_colorTriggerWasTrue, pTriggerColors, "WasTrue");
        ReadColor(m_colorTriggerBecomingTrue, pTriggerColors, "BecomingTrue");
        ReadColor(m_colorTriggerResetTrue, pTriggerColors, "ResetTrue");
        ReadColor(m_colorTriggerPauseTrue, pTriggerColors, "PauseTrue");
    }
}

} // namespace ui
} // namespace ra
