extern "C" {
#include "app.h"
}

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

@interface Glue : NSObject {
@public
    app_t* app;
}
-(void) onOpen;
@end

@implementation Glue
-(void) onOpen {
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    NSArray *fileTypes = [NSArray arrayWithObjects:@"stl", @"STL", nil];
    [panel setAllowedFileTypes:fileTypes];

    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSModalResponseOK) {
            NSURL *doc = [[panel URLs] objectAtIndex:0];
            NSString *urlString = [doc path];
            app_open(self->app, [urlString UTF8String]);
            NSLog(@"open item called %@", doc);
        }
    }];
}
@end

// Cocoa constructs the default application menu by hand for each program; that's what MainMenu.[nx]ib does
extern "C" void platform_build_menus(app_t* app)
{
    Glue *glue = [[Glue alloc] init];
    glue->app = app;

    NSMenu *fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
    NSMenuItem *fileMenuItem = [[[NSMenuItem alloc] initWithTitle:@"File" action:NULL keyEquivalent:@""] autorelease];
    [fileMenuItem setSubmenu:fileMenu];

    NSMenuItem *open = [[[NSMenuItem alloc] initWithTitle:@"Open" action:@selector(onOpen) keyEquivalent:@"o"] autorelease];
    [fileMenu setAutoenablesItems:NO];
    open.target = glue;
    [open setEnabled:true];
    [fileMenu addItem:open];

    [[NSApplication sharedApplication].mainMenu insertItem:fileMenuItem atIndex:1];
}
