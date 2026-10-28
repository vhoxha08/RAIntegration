#include "WxTestHooks.hh"

#import <AppKit/AppKit.h>


#include <cstdio>
#include <cstdlib>
#include <functional>
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
    else if (pStep.sAction == "cancelmenus")
    {
        // closes open drop down menus (opened by clicking a pop-up button)
        std::function<void(NSView*)> Cancel = [&Cancel](NSView* pView) {
            if ([pView isKindOfClass:[NSPopUpButton class]])
                [[(NSPopUpButton*)pView menu] cancelTracking];
            for (NSView* pChild in [pView subviews])
                Cancel(pChild);
        };
        for (NSWindow* pWindow in [NSApp windows])
            Cancel([pWindow contentView]);
        fprintf(stderr, "RA_WX_TEST cancelled menus\n");
    }
    else if (pStep.sAction == "movewin")
    {
        // "movewin x y title prefix": moves a window (top left corner, screen coordinates from the top)
        int nX = 0, nY = 0, nConsumed = 0;
        if (sscanf(pStep.sArgs.c_str(), "%d %d %n", &nX, &nY, &nConsumed) >= 2)
        {
            NSString* sPrefix = [NSString stringWithUTF8String:pStep.sArgs.substr(static_cast<size_t>(nConsumed)).c_str()];
            for (NSWindow* pWindow in [NSApp windows])
            {
                if (![pWindow isVisible] || ![[pWindow title] hasPrefix:sPrefix])
                    continue;
                const CGFloat fScreenTop = NSMaxY([[[NSScreen screens] firstObject] frame]);
                [pWindow setFrameTopLeftPoint:NSMakePoint(nX, fScreenTop - nY)];
                fprintf(stderr, "RA_WX_TEST moved '%s' to %d,%d\n", [[pWindow title] UTF8String], nX, nY);
                break;
            }
        }
    }
    else if (pStep.sAction == "list")
    {
        for (NSWindow* pWindow in [NSApp windows])
        {
            if (![pWindow isVisible])
                continue;
            const CGFloat fScreenTop = NSMaxY([[[NSScreen screens] firstObject] frame]);
            const NSRect rFrame = [pWindow frame];
            fprintf(stderr, "RA_WX_TEST window '%s'%s at %d,%d size %dx%d\n", [[pWindow title] UTF8String],
                    pWindow == [NSApp modalWindow] ? " (modal)" : "", static_cast<int>(rFrame.origin.x),
                    static_cast<int>(fScreenTop - NSMaxY(rFrame)), static_cast<int>(rFrame.size.width),
                    static_cast<int>(rFrame.size.height));
            ListButtons([pWindow contentView], 0);
        }
    }
}

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

    // runs for the rest of the process, in every run loop mode (also inside menus, resizing and modal loops)
    static std::vector<Step> s_vSteps;
    static int s_nTick = 0;
    s_vSteps = std::move(vSteps);
    CFRunLoopTimerRef pTimer = CFRunLoopTimerCreateWithHandler(
        kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + 0.1, 0.1, 0, 0, ^(CFRunLoopTimerRef) {
            ++s_nTick;
            for (const auto& pStep : s_vSteps)
            {
                if (pStep.nTick == s_nTick)
                    RunStep(pStep);
            }
        });
    CFRunLoopAddTimer(CFRunLoopGetMain(), pTimer, kCFRunLoopCommonModes);
    CFRelease(pTimer);
}

} // namespace wx
} // namespace ui
} // namespace ra
