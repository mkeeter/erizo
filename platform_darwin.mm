extern "C" {
#include "app.h"
}

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

// Cocoa constructs the default application menu by hand for each program; that's what MainMenu.[nx]ib does
extern "C" void platform_build_menus()
{
    NSMenu* fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
    NSMenuItem *fileMenuItem = [[[NSMenuItem alloc] initWithTitle:@"File" action:NULL keyEquivalent:@""] autorelease];
    [fileMenuItem setSubmenu:fileMenu];

    [[NSApplication sharedApplication].mainMenu insertItem:fileMenuItem atIndex:1];
}
