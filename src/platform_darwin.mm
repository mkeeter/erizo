extern "C" {
#include "app.h"
#include "instance.h"
#include "window.h"
}
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSObjCRuntime.h>
#import <objc/runtime.h>

@interface InstanceHandle : NSObject {
@public
    instance_t* instance;
}
@end
@implementation InstanceHandle
@end

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
    NSWindow* window = [[NSApplication sharedApplication] keyWindow];
    InstanceHandle* handle = objc_getAssociatedObject(window, "WINDOW_INSTANCE");
    glfwSetWindowShouldClose(handle->instance->window, 1);
}
@end

static Glue* GLUE = NULL;

void fopenFiles(id self, SEL _cmd, NSApplication* application,
                NSArray<NSString *>* openFiles) {
    for (NSString* t in openFiles) {
        instance_t* instance = app_open(GLUE->app, [t UTF8String]);
        NSWindow* window = glfwGetCocoaWindow(instance->window);
        [window makeKeyWindow];
    }
}

extern "C" void platform_window_bind(GLFWwindow* w) {
    InstanceHandle* handle = [[InstanceHandle alloc] init];
    handle->instance = (instance_t*)glfwGetWindowUserPointer(w);

    objc_setAssociatedObject(glfwGetCocoaWindow(w), "WINDOW_INSTANCE",
                             handle, OBJC_ASSOCIATION_RETAIN);
}

extern "C" void platform_init(app_t* app, int argc, char** argv)
{
    if (argc == 2) {
        //  Disable file opening through application:openFiles:, which
        //  is triggered for command-line arguments during the first call
        //  to glfwInit.
        app_open(app, argv[1]);
    }

    GLUE = [[Glue alloc] init];
    GLUE->app = app;

    //  Monkey-patch the application delegate so that it knows
    //  how to open files.
    Class delegate_class = NSClassFromString(@"GLFWApplicationDelegate");
    class_addMethod(delegate_class, @selector(application:openFiles:),
                    (IMP)fopenFiles, "v@:@@");

    if (app->num_instances == 0) {
        //  Construct a dummy window, which triggers GLFW initialization
        //  and may cause the application to open a file (if it was
        //  double-clicked or dragged onto the icon).
        window_new(1.0f, 1.0f);

        //  If no file was opened, then load the default
        if (app->num_instances == 0) {
            app_open(app, ":/sphere");
        }
    }

    // Build a file menu
    NSMenu *fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
    NSMenuItem *fileMenuItem = [[[NSMenuItem alloc]
        initWithTitle:@"File" action:NULL keyEquivalent:@""] autorelease];
    [fileMenuItem setSubmenu:fileMenu];

    NSMenuItem *open = [[[NSMenuItem alloc]
        initWithTitle:@"Open"
        action:@selector(onOpen) keyEquivalent:@"o"] autorelease];
    open.target = GLUE;
    [fileMenu addItem:open];

    NSMenuItem *close = [[[NSMenuItem alloc]
        initWithTitle:@"Close"
        action:@selector(onClose) keyEquivalent:@"w"] autorelease];
    close.target = GLUE;
    [fileMenu addItem:close];

    [[NSApplication sharedApplication].mainMenu insertItem:fileMenuItem atIndex:1];
}

extern "C" void* platform_native_window(instance_t* instance) {
    return (instance == NULL) ? NULL : glfwGetCocoaWindow(instance->window);
}
