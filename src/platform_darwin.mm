extern "C" {
#include "app.h"
#include "instance.h"
}
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSObjCRuntime.h>
#import <objc/runtime.h>

@interface Glue : NSObject {
@public
    app_t* app;
}
-(void) onOpen;
-(void) onClose;
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

-(void) onClose {
    void* window = [[NSApplication sharedApplication] keyWindow];
    app_close_native_window(self->app, window);
}
@end

void fopenFiles(id self, SEL _cmd, NSApplication* application,
                NSArray<NSString *>* openFiles) {
    for (NSString* t in openFiles) {
        Glue* glue = objc_getAssociatedObject(self, "GLUE");
        instance_t* instance = app_open(glue->app, [t UTF8String]);
        NSWindow* window = glfwGetCocoaWindow(instance->window);
        [window makeKeyWindow];
    }
    NSLog(@"%@ %@ %@", self, application, openFiles);
}

extern "C" void platform_init(app_t* app)
{
    Glue* glue = [[Glue alloc] init];
    glue->app = app;

    id delegate = [NSApp delegate];
    Class delegate_class = object_getClass(delegate);
    objc_setAssociatedObject(delegate, "GLUE", glue, OBJC_ASSOCIATION_RETAIN);
    class_addMethod(delegate_class, @selector(application:openFiles:),
                    (IMP)fopenFiles, "v@:@@");

    NSMenu *fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
    NSMenuItem *fileMenuItem = [[[NSMenuItem alloc]
        initWithTitle:@"File" action:NULL keyEquivalent:@""] autorelease];
    [fileMenuItem setSubmenu:fileMenu];

    NSMenuItem *open = [[[NSMenuItem alloc]
        initWithTitle:@"Open"
        action:@selector(onOpen) keyEquivalent:@"o"] autorelease];
    open.target = glue;
    [fileMenu addItem:open];

    NSMenuItem *close = [[[NSMenuItem alloc]
        initWithTitle:@"Close"
        action:@selector(onClose) keyEquivalent:@"w"] autorelease];
    close.target = glue;
    [fileMenu addItem:close];

    [[NSApplication sharedApplication].mainMenu insertItem:fileMenuItem atIndex:1];
}

extern "C" void* platform_native_window(instance_t* instance) {
    return (instance == NULL) ? NULL : glfwGetCocoaWindow(instance->window);
}
