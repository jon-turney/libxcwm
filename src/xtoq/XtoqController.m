/* Copyright (C) 2012 Aaron Skomra, Ben Huddle, Braden Wooley

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#import "XtoqController.h"

#define WINDOWBAR 22

@implementation XtoqController

- (id) init
{
    self = [super init];
    if (self) {
        referenceToSelf = self;
    }
    return self;
}

- (int) xserverToOSX:(int)yValue windowHeight:(int)windowH
{

    int height = [[NSScreen mainScreen] frame].size.height;
    return height - windowH - yValue;

}

- (int) osxToXserver:(int)yValue windowHeight:(int)windowH
{

    int height = [[NSScreen mainScreen] frame].size.height;
    return height - yValue;

}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{

    // setup X connection and get the initial image from the server
    rootContext = xcwm_context_open(screen);

    [[NSGraphicsContext currentContext]
     setImageInterpolation:NSImageInterpolationHigh];

    xcwm_window_t *rootWin = xcwm_context_get_root_window(rootContext);
    xcwm_rect_t *winRect = xcwm_window_get_full_rect(rootWin);
    xcwmWindow = [[XtoqWindow alloc]
                  initWithContentRect: NSMakeRect(winRect->x,
                                                  winRect->y,
                                                  winRect->width,
                                                  winRect->height)
                  styleMask: (NSTitledWindowMask |
                              NSClosableWindowMask |
                              NSMiniaturizableWindowMask |
                              NSResizableWindowMask)
                  backing: NSBackingStoreBuffered
                  defer: YES];

    [xcwmWindow setXcwmWindow: rootWin];
    xcwm_window_set_local_data(rootWin, xcwmWindow);
    // Make the menu
    [self makeMenu];

    // FIXME: Since this is the root window, should be getting width
    // and height from the size of the root window, not hard-coded
    // values.
    imageRec = NSMakeRect(0, 0, 1028, 768);
    // create a view, init'ing it with our rect
    ourView = [[XtoqView alloc] initWithFrame:imageRec];
    [ourView setXcwmWindow: rootWin];
    
    // add view to its window
    [xcwmWindow setContentView: ourView];

    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];

    // Register for the key down notifications from the view
    [nc addObserver: self
           selector: @selector(keyDownInView:)
               name: @"XTOQviewKeyDownEvent"
             object: nil];

    [nc addObserver: self
           selector: @selector(keyUpInView:)
               name: @"XTOQviewKeyUpEvent"
             object: nil];

    [nc addObserver: self
           selector: @selector(mouseButtonDownInView:)
               name: @"XTOQmouseButtonDownEvent"
             object: nil];

    [nc addObserver: self
           selector: @selector(mouseButtonReleaseInView:)
               name: @"XTOQmouseButtonReleaseEvent"
             object: nil];

    [nc addObserver: self
           selector: @selector(mouseMovedInApp:)
               name: @"MouseMovedEvent"
             object: nil];
    // register for destroy event
    [nc addObserver: self
           selector: @selector(destroy:)
               name: @"XTOQdestroyTheWindow"
             object: nil];

    [nc addObserver:self
           selector:@selector(windowDidMove:)
               name:NSWindowDidMoveNotification
             object:nil];

    // regester for window resize notification
    [nc addObserver:self
           selector:@selector(windowDidResize:)
               name:NSWindowDidResizeNotification
             object:nil];

    [nc addObserver:self
           selector:@selector(windowDidMiniaturize:)
               name:NSWindowDidMiniaturizeNotification
             object:nil];

    [nc addObserver:self
           selector:@selector(windowDidDeminiaturize:)
               name:NSWindowDidDeminiaturizeNotification
             object:nil];

    xcwmDispatchQueue = dispatch_queue_create("xcwm.dispatch.queue", NULL);

    // Start the event loop and set the handler function
    xcwm_event_start_loop(rootContext, (void *)eventHandler);
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    xcwm_context_close(rootContext);
 
    // TODO: Implement decent server shutdown on app termination.
    // FIXME: This way of closing the server creates more problems than its
    // currently worth.
    // const char *spawn[4];
    // pid_t child;

    // spawn[0] = "/usr/bin/killall";
    // spawn[1] = "-9";
    // spawn[2] = "Xorg";
    // spawn[3] = NULL;

    // posix_spawn(&child, spawn[0], NULL, NULL, (char *const *)spawn, environ);
}

- (void) applicationWillFinishLaunching: (NSNotification *) aNotification
{}

- (void) mouseMovedInApp: (NSNotification *) aNotification
{

    //CGFloat heightFloat;
    NSDictionary *mouseMoveInfo = [aNotification userInfo];
    NSNumber *xNum = [mouseMoveInfo objectForKey: @"1"];
    NSNumber *yNum = [mouseMoveInfo objectForKey: @"2"];

    int height = [[NSScreen mainScreen] frame].size.height;

    dispatch_async(xcwmDispatchQueue,
                   ^{ xcwm_input_mouse_motion
                      (rootContext,
                       [xNum intValue],
                       //Converting OSX coordinates to X11
                       height - WINDOWBAR - [yNum intValue]);
                   });
}

- (void) keyDownInView: (NSNotification *) aNotification
{
    NSDictionary *keyInfo = [aNotification userInfo];
    // note this keyInfo is the key in <key, value> not the key pressed
    NSEvent * event = [keyInfo objectForKey: @"1"];
    unsigned short aChar = [event keyCode];

    // FIXME: Uses a 'magic number' for offset into keymap - should a
    // #define or gotten programmatically.
    dispatch_async(xcwmDispatchQueue,
                   ^{ xcwm_input_key_event (rootContext,
                                            aChar + 8,
                                            1);
                   });
}

- (void) keyUpInView: (NSNotification *) aNotification
{
    NSDictionary *keyInfo = [aNotification userInfo];
    // note this keyInfo is the key in <key, value> not the key pressed
    NSEvent * event = [keyInfo objectForKey: @"1"];
    unsigned short aChar = [event keyCode];

    // FIXME: Uses a 'magic number' for offset.
    dispatch_async(xcwmDispatchQueue,
                   ^{ xcwm_input_key_event (rootContext,
                                            aChar + 8,
                                            0);
                   });
}

// on this side all I have is a xcwm_context , on the library side I need
// to turn that into a real context
- (void) mouseButtonDownInView: (NSNotification *) aNotification
{
    CGFloat heightFloat;
    NSDictionary *mouseDownInfo = [aNotification userInfo];
    // NSLog(@"Controller Got a XTOQmouseButtonDownEvent");
    NSEvent * event = [mouseDownInfo objectForKey: @"1"];
    xcwm_window_t *window = [(XtoqWindow *)[event window] getXcwmWindow];

    NSNumber * heightAsNumber = [NSNumber alloc];
    heightAsNumber = [mouseDownInfo objectForKey: @"2"];
    heightFloat = [heightAsNumber floatValue];

    NSNumber * mouseButton = [NSNumber alloc];
    mouseButton = [mouseDownInfo objectForKey:@"3"];
    int buttonInt = [mouseButton intValue];

    dispatch_async(xcwmDispatchQueue,
                   ^{ xcwm_input_mouse_button_event (window,
                                                     buttonInt,
                                                     1);
                      ;
                   });
}

// on this side all I have is a xcwm_context , on the library side I need
// to turn that into a real context
- (void) mouseButtonReleaseInView: (NSNotification *) aNotification
{
    CGFloat heightFloat;
    NSDictionary *mouseReleaseInfo = [aNotification userInfo];
    NSEvent * event = [mouseReleaseInfo objectForKey: @"1"];
    xcwm_window_t *window = [(XtoqWindow *)[event window] getXcwmWindow];

    NSNumber * heightAsNumber = [NSNumber alloc];
    heightAsNumber = [mouseReleaseInfo objectForKey: @"2"];
    heightFloat = [heightAsNumber floatValue];

    NSNumber * mouseButton = [NSNumber alloc];
    mouseButton = [mouseReleaseInfo objectForKey:@"3"];
    int buttonInt = [mouseButton intValue];

    dispatch_async(xcwmDispatchQueue,
                   ^{ xcwm_input_mouse_button_event (window,
                                                     buttonInt,
                                                     0);
                   });
}

- (void) setScreen:(char *)scrn
{
    free(screen);
    screen = strdup(scrn);
    setenv("DISPLAY", screen, 1);
}

- (void) makeMenu
{
    // Create menu
    NSMenu *menubar;
    NSMenuItem *appMenuItem;
    NSMenu *appMenu;
    NSString *appName;
    NSString *aboutTitle;
    NSMenuItem *aboutMenuItem;
    NSString *quitTitle;
    NSMenuItem *quitMenuItem;

    menubar = [[NSMenu new] autorelease];
    appMenuItem = [[NSMenuItem new] autorelease];

    appMenu = [[NSMenu new] autorelease];
    appName = [[NSProcessInfo processInfo] processName];

    // Xtoq -> About
    aboutTitle = [@"About " stringByAppendingString:appName];
    aboutMenuItem = [[NSMenuItem alloc] initWithTitle:aboutTitle
                                               action:NULL
                                        keyEquivalent:@"a"];
    [appMenu addItem:aboutMenuItem];
    [appMenuItem setSubmenu:appMenu];

    // Xtoq -> Quit
    quitTitle = [@"Quit " stringByAppendingString:appName];
    quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle
                                              action:@selector(terminate:)
                                       keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    // Menu under Applications
    NSMenu *startXMenu = [[NSMenu new] autorelease];
    NSMenuItem *startXApps = [[NSMenuItem new] autorelease];
    [startXMenu setTitle:@"Applications"];

    // Run Xeyes
    NSString *xTitle;
    NSMenuItem *xeyesMenuItem;
    xTitle = @"Run Xeyes";
    xeyesMenuItem = [[NSMenuItem alloc] initWithTitle:xTitle
                                               action:@selector(runXeyes:)
                                        keyEquivalent:@""];
    [startXMenu addItem:xeyesMenuItem];
    [startXApps setSubmenu:startXMenu];

    // Run Xclock
    NSMenuItem *xclockMenuItem;
    xTitle = @"Run Xclock";
    xclockMenuItem = [[NSMenuItem alloc] initWithTitle:xTitle
                                                action:@selector(runXclock:)
                                         keyEquivalent:@""];
    [startXMenu addItem:xclockMenuItem];
    [startXApps setSubmenu:startXMenu];

    // Run Xlogo
    NSMenuItem *xlogoMenuItem;
    xTitle = @"Run Xlogo";
    xlogoMenuItem = [[NSMenuItem alloc] initWithTitle:xTitle
                                               action:@selector(runXlogo:)
                                        keyEquivalent:@""];
    [startXMenu addItem:xlogoMenuItem];
    [startXApps setSubmenu:startXMenu];

    // Run Xterm
    NSMenuItem *xtermMenuItem;
    xTitle = @"Run Xterm";
    xtermMenuItem = [[NSMenuItem alloc] initWithTitle:xTitle
                                               action:@selector(runXterm:)
                                        keyEquivalent:@""];
    [startXMenu addItem:xtermMenuItem];
    [startXApps setSubmenu:startXMenu];

    // Run Xman
    NSMenuItem *xmanMenuItem;
    xTitle = @"Run Xman";
    xmanMenuItem = [[NSMenuItem alloc] initWithTitle:xTitle
                                              action:@selector(runXman:)
                                       keyEquivalent:@""];
    [startXMenu addItem:xmanMenuItem];
    [startXApps setSubmenu:startXMenu];

    // Adding all the menu items to the main menu for XtoQ.
    [menubar addItem:appMenuItem];
    [menubar addItem:startXApps];
    [NSApp setMainMenu:menubar];
}

- (void) launch_client:(NSString *)filename
{
    int status;
    pid_t child;
    const char *file_name = [filename UTF8String];
    const char *newargv[6];
    char *exec_path;

    // Xman Special case
    if ([filename isEqualToString:@"xman"]) {
        const char *manpath =
            "/opt/local/share/man:/usr/share/man:/usr/local/share/man:/opt/X11/share/man:/usr/X11/man:/usr/local/git/share/man";
        setenv("MANPATH", manpath, 1);
    }

    // FIXME: Doesn't seem like we should be relying on an absolute path,
    // but just sending the executable name doesn't always work.
    asprintf(&exec_path, "/usr/X11/bin/%s", file_name);
    newargv[0] = exec_path;
    newargv[1] = "-display";
    newargv[2] = screen;

    if ([filename isEqualToString:@"xclock"]) {
        newargv[3] = "-update";
        newargv[4] = "1";
        newargv[5] = NULL;
    }
    else {
        newargv[3] = NULL;
    }

    status = posix_spawnp(&child, newargv[0], NULL, NULL,
                          (char *const *)newargv, environ);
    if (status) {
        NSLog(@"Error spawning file for launch.");
    }
}

- (void) runXeyes:(id) sender
{
    [self launch_client:@"xeyes"];
}
- (void) runXclock:(id) sender
{
    [self launch_client:@"xclock"];
}
- (void) runXlogo:(id) sender
{
    [self launch_client:@"xlogo"];
}
- (void) runXterm:(id) sender
{
    [self launch_client:@"xterm"];
}
-(void) runXman:(id) sender
{
    [self launch_client:@"xman"];
}

// create a new window
- (void) createNewWindow: (xcwm_window_t *) window
{

    XtoqWindow   *newWindow;
    XtoqView     *newView;
    xcwm_image_t *xcbImage;
    XtoqImageRep *imageRep;
    NSUInteger winStyle;

    xcwm_rect_t  *windowSize = xcwm_window_get_full_rect(window);
    int y = [self xserverToOSX: windowSize->y windowHeight:windowSize->height];

    if (xcwm_window_is_override_redirect(window)) {
      winStyle = NSBorderlessWindowMask;
      // Make up the width of the window bar
      y = y - WINDOWBAR;
    } else {
        winStyle = (NSTitledWindowMask |
                    NSClosableWindowMask |
                    NSMiniaturizableWindowMask |
                    NSResizableWindowMask);
        // Move the window down by the height of the OSX menu bar
        xcwm_window_configure(window, windowSize->x,
                              windowSize->y - WINDOWBAR,
                              windowSize->width,
                              windowSize->height);
    }
    
    newWindow = [[XtoqWindow alloc]
                 initWithContentRect: NSMakeRect(windowSize->x, y,
                                                 windowSize->width,
                                                 windowSize->height)
                           styleMask: winStyle
                             backing: NSBackingStoreBuffered
                               defer: YES];

    // save xcwm_window_t in the XtoqWindow
    [newWindow setXcwmWindow: window];

    // save the newWindow pointer into the context
    xcwm_window_set_local_data(window, (id)newWindow);

    // get image to draw
    xcbImage = xcwm_image_copy_full(window);
    imageRep = [[XtoqImageRep alloc] initWithData: xcbImage x: 0 y: 0];

    // draw the image into a rect
    NSRect imgRec = NSMakeRect(0, 0, [imageRep getWidth],
                               [imageRep getHeight]);

    // create a view, init'ing it with our rect
    newView = [[XtoqView alloc] initWithFrame:imgRec];
    [newView setXcwmWindow: window];

    // add view to its window
    [newWindow setContentView: newView];

    // set title
    [self updateWindowName: window];

    // Set the sizing for the window, if we have values.
    xcwm_window_sizing_t const *sizing = xcwm_window_get_sizing(window);
    if (sizing->min_width > 0 || sizing->min_height > 0) {
        [newWindow setContentMinSize: NSMakeSize(sizing->min_width,
                                                 sizing->min_height)];
    }
    if (sizing->max_width > 0 || sizing->max_height > 0) {
        [newWindow setContentMaxSize: NSMakeSize(sizing->max_width,
                                                 sizing->max_height)];
    }
    if (sizing->width_inc > 0 || sizing->height_inc > 0) {
        [newWindow setContentResizeIncrements: NSMakeSize(sizing->width_inc,
                                                          sizing->height_inc)];
    }

    //shows the window
    [newWindow makeKeyAndOrderFront: self];
}

- (void) destroyWindow:(xcwm_window_t *) window
{
    // set the window to be closed
    XtoqWindow *destWindow = xcwm_window_get_local_data(window);
    //close window
    [destWindow close];
}

- (void) destroy:(NSNotification *) aNotification
{

    NSDictionary *contextInfo = [aNotification userInfo];
    XtoqWindow *aWindow = [contextInfo objectForKey: @"1"];

    //use dispatch_async() to handle the actual close
    dispatch_async(xcwmDispatchQueue, ^{
                       NSLog (@"Call xcwm_request_close(theContext)");
                       xcwm_window_request_close ([aWindow getXcwmWindow]);
                   });
}

- (void) updateImage:(xcwm_window_t *) window
{
    float y_transformed;
    NSRect newDamageRect;

    xcwm_rect_t *winRect = xcwm_window_get_full_rect(window);
    xcwm_rect_t *dmgRect = xcwm_window_get_damaged_rect(window);

    y_transformed = (winRect->height - dmgRect->y
                     - dmgRect->height) / 1.0;
    newDamageRect = NSMakeRect(dmgRect->x,
                               y_transformed,
                               dmgRect->width,
                               dmgRect->height);
    XtoqView *localView =
      (XtoqView *)[(XtoqWindow *)xcwm_window_get_local_data(window)
                                 contentView];
    [localView setPartialImage:newDamageRect];
}

- (void) windowDidMove:(NSNotification *)notification
{
    [self reshape];
}

- (void) windowDidResize:(NSNotification *)notification
{
    [self reshape];
}

- (void) reshape
{

    XtoqWindow *moveWindow = (XtoqWindow *)[NSApp mainWindow];

    if (moveWindow != nil) {
        xcwm_window_t *window = [moveWindow getXcwmWindow];
        NSRect moveFrame = [moveWindow frame];
        int x = (int)moveFrame.origin.x;
        int y = [self osxToXserver: (int)moveFrame.origin.y
                      windowHeight: xcwm_window_get_full_rect(window)->height]
          - WINDOWBAR;
        int width = (int)moveFrame.size.width;
        int height = (int)moveFrame.size.height - WINDOWBAR;

        xcwm_window_configure(window,
                              x, y - height, width, height);
        [[moveWindow contentView] setNeedsDisplay: YES];
    }
}

- (void)updateWindowName:(xcwm_window_t *)window
{
    XtoqWindow *nameWindow = xcwm_window_get_local_data(window);
    char *name = xcwm_window_copy_name(window);
    NSString *winTitle;
    winTitle = [NSString stringWithCString: name
                                  encoding: NSUTF8StringEncoding];
    [nameWindow setTitle: winTitle];
    free(name);
}

- (void) windowDidMiniaturize:(NSNotification *)notification
{
    XtoqWindow *xtoqWin = (XtoqWindow *)[notification object];
    xcwm_window_t *window = [xtoqWin getXcwmWindow];
    
    xcwm_window_iconify(window);
}

- (void) windowDidDeminiaturize:(NSNotification *)notification
{
    XtoqWindow *xtoqWin = (XtoqWindow *)[notification object];
    xcwm_window_t *window = [xtoqWin getXcwmWindow];
    
    xcwm_window_deiconify(window);
}

@end

void
eventHandler(xcwm_event_t *event)
{
    xcwm_window_t *window = xcwm_event_get_window(event);
    if (xcwm_event_get_type(event) == XCWM_EVENT_WINDOW_DAMAGE) {
        [referenceToSelf updateImage: window];
    }
    else if (xcwm_event_get_type(event) == XCWM_EVENT_WINDOW_CREATE) {
        NSLog(@"Window was created");
        [referenceToSelf createNewWindow: window];
    }
    else if (xcwm_event_get_type(event) == XCWM_EVENT_WINDOW_DESTROY) {
        NSLog(@"Window was destroyed");
        [referenceToSelf destroyWindow: window];
    }
    else if (xcwm_event_get_type(event) == XCWM_EVENT_WINDOW_NAME) {
        NSLog(@"Window name changed");
        [referenceToSelf updateWindowName: window];
    }
    else {
        NSLog(@"Unknown event type received.");
    }
    free(event);
}
