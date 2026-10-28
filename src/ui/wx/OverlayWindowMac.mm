// macOS specifics of the overlay window (see OverlayWindow.cpp)

#import <AppKit/AppKit.h>

#include <wx/defs.h>

namespace ra {
namespace ui {
namespace wx {

void PrepareOverlayNSWindow(WXWindow pWindow)
{
    NSWindow* pNSWindow = (NSWindow*)pWindow;

    // clicks go through to the game; the overlay is only drawn
    [pNSWindow setIgnoresMouseEvents:YES];
    [pNSWindow setHasShadow:NO];
    [pNSWindow setOpaque:NO];
    [pNSWindow setBackgroundColor:[NSColor clearColor]];

    // shown with the game in a full screen space
    [pNSWindow setCollectionBehavior:[pNSWindow collectionBehavior] | NSWindowCollectionBehaviorFullScreenAuxiliary];
}

} // namespace wx
} // namespace ui
} // namespace ra
