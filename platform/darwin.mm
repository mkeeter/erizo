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

    NSMenuItem* shaded;
    NSMenuItem* wireframe;

    NSMenuItem* perspective;
    NSMenuItem* orthographic;
}
-(void) onOpen;
-(void) onClose;
-(void) onAboutMenu;
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
    if (window) {
        InstanceHandle* handle = objc_getAssociatedObject(
            window, "WINDOW_INSTANCE");
        if (handle) {
            glfwSetWindowShouldClose(handle->instance->window, 1);
        } else {
            [window close];
        }
    }
}

-(void) onShaded {
    [self->shaded setState:NSOnState];
    [self->wireframe setState:NSOffState];
    app_view_shaded(self->app);
}

-(void) onWireframe {
    [self->shaded setState:NSOffState];
    [self->wireframe setState:NSOnState];
    app_view_wireframe(self->app);
}

-(void) onPerspective {
    [self->perspective setState:NSOnState];
    [self->orthographic setState:NSOffState];
    app_view_perspective(self->app);
}

-(void) onOrthographic {
    [self->perspective setState:NSOffState];
    [self->orthographic setState:NSOnState];
    app_view_orthographic(self->app);
}

-(void)onAboutMenu {
    extern const char* GIT_REV;
    NSString *version = [NSString stringWithFormat:@"Version: %s", GIT_REV];
    NSDictionary* d = @{
        NSAboutPanelOptionApplicationVersion: version,
    };
    [[NSApplication sharedApplication] orderFrontStandardAboutPanelWithOptions:d];
}

@end

static Glue* GLUE = NULL;

void fopenFiles(id self, SEL _cmd, NSApplication* application,
                NSArray<NSString *>* openFiles) {
    for (NSString* t in openFiles) {
        app_open(GLUE->app, [t UTF8String]);
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
        //
        //  Skip "files" beginning with -psn, which is a Mac-specific
        //  pseudo-file that's appended to the command-line arguments if
        //  you launch the application by dragging a file onto the icon.
        const char psn_prefix[] = "-psn";
        if (strncmp(psn_prefix, argv[1], sizeof(psn_prefix) - 1)) {
            app_open(app, argv[1]);
        }
    }

    GLUE = [[Glue alloc] init];
    GLUE->app = app;

    //  Monkey-patch the application delegate so that it knows
    //  how to open files.
    Class delegate_class = NSClassFromString(@"GLFWApplicationDelegate");
    class_addMethod(delegate_class, @selector(application:openFiles:),
                    (IMP)fopenFiles, "v@:@@");

    if (app->instance_count == 0) {
        //  Construct a dummy window, which triggers GLFW initialization
        //  and may cause the application to open a file (if it was
        //  double-clicked or dragged onto the icon).
        window_new("", 1.0f, 1.0f);

        //  If no file was opened, then load the default
        if (app->instance_count == 0) {
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

    // Build the view menu
    NSMenu *viewMenu = [[[NSMenu alloc] initWithTitle:@"View"] autorelease];
    NSMenuItem *viewMenuItem = [[[NSMenuItem alloc]
        initWithTitle:@"View" action:NULL keyEquivalent:@""] autorelease];
    [viewMenuItem setSubmenu:viewMenu];

    {
        NSMenuItem *shaded = [[[NSMenuItem alloc]
            initWithTitle:@"Shaded"
            action:@selector(onShaded)
            keyEquivalent:@""
            ] autorelease];
        shaded.target = GLUE;
        [shaded setState:NSOnState];
        [viewMenu addItem:shaded];
        GLUE->shaded = shaded;
    }


    {
        NSMenuItem *wireframe = [[[NSMenuItem alloc]
            initWithTitle:@"Wireframe"
            action:@selector(onWireframe)
            keyEquivalent:@""
            ] autorelease];
        wireframe.target = GLUE;
        [viewMenu addItem:wireframe];
        GLUE->wireframe = wireframe;
    }

    [viewMenu addItem:[NSMenuItem separatorItem]];

    {
        NSMenuItem *orthographic = [[[NSMenuItem alloc]
            initWithTitle:@"Orthographic"
            action:@selector(onOrthographic)
            keyEquivalent:@""
            ] autorelease];
        orthographic.target = GLUE;
        [orthographic setState:NSOnState];
        [viewMenu addItem:orthographic];
        GLUE->orthographic = orthographic;
    }

    {
        NSMenuItem *perspective = [[[NSMenuItem alloc]
            initWithTitle:@"Perspective"
            action:@selector(onPerspective)
            keyEquivalent:@""
            ] autorelease];
        perspective.target = GLUE;
        [viewMenu addItem:perspective];
        GLUE->perspective = perspective;
    }

    NSApplication * nsApp = [NSApplication sharedApplication];
    [nsApp.mainMenu insertItem:fileMenuItem atIndex:1];
    [nsApp.mainMenu insertItem:viewMenuItem atIndex:2];

    // Patch the "About" menu item to call our custom function
    NSMenu* appMenu = [nsApp.mainMenu itemWithTitle:@""].submenu;
    NSMenuItem* aboutItem = [appMenu itemAtIndex:0];
    aboutItem.action = @selector(onAboutMenu);
    aboutItem.target = GLUE;
}

extern "C" void platform_warning(const char* title, const char* text) {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:[NSString stringWithUTF8String:title]];
    [alert setInformativeText:[NSString stringWithUTF8String:text]];
    [alert addButtonWithTitle:@"Okay"];
    [alert runModal];
}
