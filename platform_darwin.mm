extern "C" {
#include "app.h"
#include "instance.h"
}
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

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
            instance_t* instance = app_open(self->app, [urlString UTF8String]);

            NSWindow* window = glfwGetCocoaWindow(instance->window);
            [window makeKeyWindow];
        }
    }];
}
@end

extern "C" void platform_init(app_t* app)
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
