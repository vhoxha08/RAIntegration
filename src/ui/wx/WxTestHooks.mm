#include "WxTestHooks.hh"

#import <AppKit/AppKit.h>

#include <wx/timer.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace ra {
namespace ui {
namespace wx {

namespace {

struct Step
{
    int nTick;
    std::string sAction;
    std::string sArgs;
};

NSWindow* FrontWindow()
{
    if (NSWindow* pModal = [NSApp modalWindow])
        return pModal;
    if (NSWindow* pKey = [NSApp keyWindow])
        return pKey;
    return [[NSApp orderedWindows] firstObject];
}

NSTableView* FindTable(NSView* pView)
{
    if ([pView isKindOfClass:[NSTableView class]])
        return (NSTableView*)pView;

    for (NSView* pChild in [pView subviews])
    {
        if (NSTableView* pTable = FindTable(pChild))
            return pTable;
    }

    return nil;
}

NSButton* FindButton(NSView* pView, NSString* sTitle)
{
    if ([pView isKindOfClass:[NSButton class]] && [[(NSButton*)pView title] isEqualToString:sTitle])
        return (NSButton*)pView;

    for (NSView* pChild in [pView subviews])
    {
        if (NSButton* pButton = FindButton(pChild, sTitle))
            return pButton;
    }

    return nil;
}

void ListButtons(NSView* pView, int nDepth)
{
    if ([pView isKindOfClass:[NSButton class]])
        fprintf(stderr, "RA_WX_TEST   button '%s'\n", [[(NSButton*)pView title] UTF8String]);
    else if ([pView isKindOfClass:[NSTextField class]])
        fprintf(stderr, "RA_WX_TEST   text '%s'\n", [[(NSTextField*)pView stringValue] UTF8String]);

    for (NSView* pChild in [pView subviews])
        ListButtons(pChild, nDepth + 1);
}

void Capture(const std::string& sPrefix)
{
    int nIndex = 0;
    for (NSWindow* pWindow in [NSApp windows])
    {
        if (![pWindow isVisible])
            continue;

        // the frame view includes the title bar
        NSView* pView = [[pWindow contentView] superview];
        if (pView == nil)
            pView = [pWindow contentView];

        NSBitmapImageRep* pRep = [pView bitmapImageRepForCachingDisplayInRect:[pView bounds]];
        [pView cacheDisplayInRect:[pView bounds] toBitmapImageRep:pRep];
        NSData* pPng = [pRep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];

        const std::string sPath = sPrefix + "_" + std::to_string(nIndex++) + ".png";
        [pPng writeToFile:[NSString stringWithUTF8String:sPath.c_str()] atomically:YES];
        fprintf(stderr, "RA_WX_TEST captured '%s' -> %s\n", [[pWindow title] UTF8String], sPath.c_str());
    }
}

void RunStep(const Step& pStep)
{
    if (pStep.sAction == "capture")
    {
        Capture(pStep.sArgs);
    }
    else if (pStep.sAction == "click")
    {
        // the modal window if there is one, otherwise any visible window
        NSString* sTitle = [NSString stringWithUTF8String:pStep.sArgs.c_str()];
        NSWindow* pWindow = FrontWindow();
        NSButton* pButton = FindButton([[pWindow contentView] superview] ?: [pWindow contentView], sTitle);
        if (pButton == nil && [NSApp modalWindow] == nil)
        {
            for (NSWindow* pOther in [NSApp windows])
            {
                if (![pOther isVisible])
                    continue;
                pButton = FindButton([pOther contentView], sTitle);
                if (pButton != nil)
                {
                    pWindow = pOther;
                    break;
                }
            }
        }
        fprintf(stderr, "RA_WX_TEST click '%s' in '%s': %s\n", pStep.sArgs.c_str(), [[pWindow title] UTF8String],
                pButton ? "found" : "not found");
        [pButton performClick:nil];
    }
    else if (pStep.sAction == "type")
    {
        NSResponder* pResponder = [FrontWindow() firstResponder];
        if ([pResponder isKindOfClass:[NSTextView class]]) // field editor of the focused text field
            [(NSTextView*)pResponder insertText:[NSString stringWithUTF8String:pStep.sArgs.c_str()]
                               replacementRange:NSMakeRange(NSNotFound, 0)];
        else
            fprintf(stderr, "RA_WX_TEST type: no focused text field\n");
    }
    else if (pStep.sAction == "selectrow")
    {
        // "selectrow n [window title prefix]": selects a row of the first table in a (matching) visible window,
        // like clicking it
        const auto nSpace = pStep.sArgs.find(' ');
        NSString* sPrefix = (nSpace == std::string::npos) ? @""
                                : [NSString stringWithUTF8String:pStep.sArgs.substr(nSpace + 1).c_str()];
        for (NSWindow* pWindow in [NSApp windows])
        {
            if ([sPrefix length] > 0 && ![[pWindow title] hasPrefix:sPrefix])
                continue;

            NSTableView* pTable = [pWindow isVisible] ? FindTable([pWindow contentView]) : nil;
            if (pTable == nil)
                continue;

            const auto nRow = static_cast<NSUInteger>(atoi(pStep.sArgs.c_str()));
            [pTable selectRowIndexes:[NSIndexSet indexSetWithIndex:nRow] byExtendingSelection:NO];
            fprintf(stderr, "RA_WX_TEST selected row %lu in '%s'\n", static_cast<unsigned long>(nRow),
                    [[pWindow title] UTF8String]);
            break;
        }
    }
    else if (pStep.sAction == "list")
    {
        for (NSWindow* pWindow in [NSApp windows])
        {
            if (![pWindow isVisible])
                continue;
            fprintf(stderr, "RA_WX_TEST window '%s'%s\n", [[pWindow title] UTF8String],
                    pWindow == [NSApp modalWindow] ? " (modal)" : "");
            ListButtons([pWindow contentView], 0);
        }
    }
}

class TestTimer : public wxTimer
{
public:
    explicit TestTimer(std::vector<Step>&& vSteps) : m_vSteps(std::move(vSteps)) {}

    void Notify() override
    {
        ++m_nTick;
        for (const auto& pStep : m_vSteps)
        {
            if (pStep.nTick == m_nTick)
                RunStep(pStep);
        }
    }

private:
    std::vector<Step> m_vSteps;
    int m_nTick = 0;
};

} // namespace

void StartTestHooks()
{
    const char* sScript = getenv("RA_WX_TEST_SCRIPT");
    if (sScript == nullptr)
        return;

    std::vector<Step> vSteps;
    std::string sAll(sScript);
    size_t nStart = 0;
    while (nStart < sAll.length())
    {
        size_t nEnd = sAll.find(';', nStart);
        if (nEnd == std::string::npos)
            nEnd = sAll.length();
        const std::string sEntry = sAll.substr(nStart, nEnd - nStart);
        nStart = nEnd + 1;

        Step pStep{};
        char sAction[32] = {0};
        int nConsumed = 0;
        if (sscanf(sEntry.c_str(), " %d %31s %n", &pStep.nTick, sAction, &nConsumed) >= 2)
        {
            pStep.sAction = sAction;
            pStep.sArgs = sEntry.substr(static_cast<size_t>(nConsumed));
            vSteps.push_back(std::move(pStep));
        }
    }

    // runs for the rest of the process, also inside native modal loops
    auto* pTimer = new TestTimer(std::move(vSteps));
    pTimer->Start(100);
}

} // namespace wx
} // namespace ui
} // namespace ra
