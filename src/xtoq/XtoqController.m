
/*Copyright (C) 2012 Aaron Skomra, Ben Huddle, Braden Wooley
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

/** 
 *  AppController.m
 *  xcwm
 *
 * 
 *  This is the controller for the Popup to retreive the display number
 *  from the user.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#import "XtoqController.h"

#define WINDOWBAR 22

@implementation XtoqController

- (id) init {
    self = [super init];
    if (self) {
        referenceToSelf = self;
    }
    return self;
}

- (int) xserverToOSX:(int)yValue windowHeight:(int)windowH {
    
    int height = [[NSScreen mainScreen] frame].size.height;    
    return height - windowH + yValue;
    
}

- (int) osxToXserver:(int)yValue windowHeight:(int)windowH {
    
    int height = [[NSScreen mainScreen] frame].size.height;    
    return height - yValue;
    
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  
    // setup X connection and get the initial image from the server
    rootContext = xcwm_init(screen);
    
    [[NSGraphicsContext currentContext]
    setImageInterpolation:NSImageInterpolationHigh];
    
    xcwmWindow = [[XtoqWindow alloc] 
                  initWithContentRect: NSMakeRect(rootContext->x, rootContext->y, 
                                                  rootContext->width, rootContext->height)
                            styleMask: (NSTitledWindowMask |
                                        NSClosableWindowMask |
                                        NSMiniaturizableWindowMask |
                                        NSResizableWindowMask)
                              backing: NSBackingStoreBuffered
                                defer: YES];

	[xcwmWindow setContext: rootContext];
	rootContext->local_data = xcwmWindow;
    // Make the menu
    [self makeMenu];

    // FIXME: Since this is the root window, should be getting width
    // and height from the size of the root window, not hard-coded
    // values.
    imageRec = NSMakeRect(0, 0, 1028,768);//[image getWidth], [image getHeight]);
    // create a view, init'ing it with our rect
    ourView = [[XtoqView alloc] initWithFrame:imageRec];
	[ourView setContext: rootContext];

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

    xcwmDispatchQueue = dispatch_queue_create("xcwm.dispatch.queue", NULL);

    // Start the event loop and set the handler function
	xcwm_start_event_loop(rootContext, (void *) eventHandler);    
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    xcwm_close();
    
    const char *spawn[4];
    pid_t child;

    // FIXME: This clobbers all instantances of the xserver. Not what
    // we want to do.
    spawn[0] = "/usr/bin/killall";
    spawn[1] = "-9";
    spawn[2] = "Xorg";
    spawn[3] = NULL;
     
    posix_spawn(&child, spawn[0], NULL, NULL, (char * const*)spawn, environ);
}

- (void) applicationWillFinishLaunching: (NSNotification *) aNotification
{
}

- (void) mouseMovedInApp: (NSNotification *) aNotification {

    //CGFloat heightFloat;
    NSDictionary *mouseMoveInfo = [aNotification userInfo];
    NSNumber *xNum =  [mouseMoveInfo objectForKey: @"1"];
    NSNumber *yNum =  [mouseMoveInfo objectForKey: @"2"];
    
    int height = [[NSScreen mainScreen] frame].size.height;
        
    dispatch_async(xcwmDispatchQueue, 
                   ^{ xcwm_input_mouse_motion
                       (rootContext,
                        [xNum intValue], 
                        //Converting OSX coordinates to X11
                        height - WINDOWBAR - [yNum intValue],
                        0);});
}

- (void) keyDownInView: (NSNotification *) aNotification
{   
    NSDictionary *keyInfo = [aNotification userInfo];
    // note this keyInfo is the key in <key, value> not the key pressed
    NSEvent * event = [keyInfo objectForKey: @"1"];
    unsigned short aChar = [event keyCode];
    NSString* charNSString = [event characters]; 
    const char* charcharstar = [charNSString UTF8String];

    NSLog(@"%s pressed", charcharstar);
    // FIXME: Uses a 'magic number' for offset into keymap - should a
    // #define or gotten programmatically.
    dispatch_async(xcwmDispatchQueue, 
                   ^{ xcwm_input_key_press(rootContext, 
                                           aChar + 8) ;});
}

- (void) keyUpInView: (NSNotification *) aNotification
{   
    NSDictionary *keyInfo = [aNotification userInfo];
    // note this keyInfo is the key in <key, value> not the key pressed
    NSEvent * event = [keyInfo objectForKey: @"1"];
    unsigned short aChar = [event keyCode];

    // FIXME: Uses a 'magic number' for offset.
    dispatch_async(xcwmDispatchQueue, 
                   ^{ xcwm_input_key_release(rootContext, 
                                     aChar + 8) ;});
}
 

// on this side all I have is a xcwm_context , on the library side I need
// to turn that into a real context 
- (void) mouseButtonDownInView: (NSNotification *) aNotification
{
    CGFloat heightFloat;
    NSDictionary *mouseDownInfo = [aNotification userInfo];
    // NSLog(@"Controller Got a XTOQmouseButtonDownEvent");
    NSEvent * event = [mouseDownInfo objectForKey: @"1"];

    NSNumber * heightAsNumber =  [NSNumber alloc];
    heightAsNumber = [mouseDownInfo objectForKey: @"2"];
    heightFloat = [heightAsNumber floatValue];
    
    NSNumber * mouseButton = [NSNumber alloc];
    mouseButton = [mouseDownInfo objectForKey:@"3"];
    int buttonInt = [mouseButton intValue];
    
    dispatch_async(xcwmDispatchQueue, 
                   ^{ xcwm_input_mouse_button_event (rootContext,
                                                     0,
                                                     0, 
                                                     buttonInt,
                                                     1);;});
}

// on this side all I have is a xcwm_context , on the library side I need
// to turn that into a real context 
- (void) mouseButtonReleaseInView: (NSNotification *) aNotification
{
    CGFloat heightFloat;
    NSDictionary *mouseReleaseInfo = [aNotification userInfo];
    NSEvent * event = [mouseReleaseInfo objectForKey: @"1"];
    NSNumber * heightAsNumber =  [NSNumber alloc];
    heightAsNumber = [mouseReleaseInfo objectForKey: @"2"];
    heightFloat = [heightAsNumber floatValue];
    
    NSNumber * mouseButton = [NSNumber alloc];
    mouseButton = [mouseReleaseInfo objectForKey:@"3"];
    int buttonInt = [mouseButton intValue];
    
    dispatch_async(xcwmDispatchQueue, 
                   ^{ xcwm_input_mouse_button_event (rootContext,
                                                     0,
                                                     0,
                                                     buttonInt,
                                                     0);;});
}


- (void) setScreen:(char *)scrn {
    free(screen);
    screen = strdup(scrn);
    setenv("DISPLAY", screen, 1);
}

- (void) makeMenu {
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

- (void) launch_client:(NSString *)filename {
    int status;
    pid_t child;
    const char *file_name = [filename UTF8String];
    const char *newargv[6];
	char *exec_path;
    
    // Xman Special case
    if ([filename isEqualToString:@"xman"]) {
        const char *manpath = "/opt/local/share/man:/usr/share/man:/usr/local/share/man:/opt/X11/share/man:/usr/X11/man:/usr/local/git/share/man";        
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
						  (char * const *) newargv, environ);
    if(status) {
        NSLog(@"Error spawning file for launch.");
    }
}

- (void) runXeyes:(id) sender {
    [self launch_client:@"xeyes"];
}
- (void) runXclock:(id) sender {
    [self launch_client:@"xclock"];
}
- (void) runXlogo:(id) sender {
    [self launch_client:@"xlogo"];    
}
- (void) runXterm:(id) sender {
    [self launch_client:@"xterm"];    
}
-(void) runXman:(id) sender {
    [self launch_client:@"xman"];
}

// create a new window 
- (void) createNewWindow: (xcwm_context_t *) windowContext {
    
    XtoqWindow   *newWindow;
    XtoqView     *newView;
    xcwm_image_t *xcbImage;
    XtoqImageRep *imageRep;  

    int y = [self xserverToOSX:windowContext->y windowHeight:windowContext->height];
    
    newWindow =  [[XtoqWindow alloc] 
                  initWithContentRect: NSMakeRect(windowContext->x, y, 
                                                  windowContext->width, windowContext->height)
                  styleMask: (NSTitledWindowMask |
                              NSClosableWindowMask |
                              NSMiniaturizableWindowMask |
                              NSResizableWindowMask)
                  backing: NSBackingStoreBuffered
                  defer: YES];
    
    // save context in window
    [newWindow setContext:windowContext];
    
    // save the newWindow pointer into the context
    windowContext->local_data = (id)newWindow;
    
    // get image to darw
    xcbImage = xcwm_get_image(windowContext);
    imageRep = [[XtoqImageRep alloc] initWithData:xcbImage x: 0 y: 0];
    
    // draw the image into a rect
    NSRect imgRec = NSMakeRect(0, 0, [imageRep getWidth], [imageRep getHeight]);
    
    // create a view, init'ing it with our rect
    newView = [[XtoqView alloc] initWithFrame:imgRec];
	[newView setContext:windowContext];
    
    // add view to its window
    [newWindow setContentView: newView ];
    
    // set title
    NSString *winTitle;
    winTitle = [NSString stringWithCString:windowContext->name encoding:NSUTF8StringEncoding];
    [newWindow setTitle:winTitle];
    
    //shows the window
    [newWindow makeKeyAndOrderFront:self];
    
}

- (void) destroyWindow:(xcwm_context_t *) windowContext {
    // set the window to be closed
    XtoqWindow *destWindow = windowContext->local_data;
    //close window
    [destWindow close];
}

- (void) destroy:(NSNotification *) aNotification {    
    
    NSDictionary *contextInfo = [aNotification userInfo];    
    XtoqWindow *aWindow = [contextInfo objectForKey: @"1"];
    xcwm_context_t *theContext = [aWindow getContext];
    
    //use dispatch_async() to handle the actual close 
      dispatch_async(xcwmDispatchQueue, ^{
          NSLog(@"Call xcwm_request_close(theContext)");
          xcwm_request_close(theContext);
      });
}

- (void) updateImage:(xcwm_context_t *) windowContext
{
    float  y_transformed;
	NSRect newDamageRect;

    y_transformed =( windowContext->height - windowContext->damaged_y - windowContext->damaged_height)/1.0; 
	newDamageRect = NSMakeRect(windowContext->damaged_x,
							   y_transformed,
							   windowContext->damaged_width,
							   windowContext->damaged_height);
	XtoqView *localView = (XtoqView *)[(XtoqWindow *)windowContext->local_data contentView];
    [ localView setPartialImage:newDamageRect];
}

- (void) windowDidMove:(NSNotification*)notification {
    [self reshape];
}

- (void) windowDidResize:(NSNotification*)notification {
    [self reshape];
}

- (void) reshape {
    
  XtoqWindow *moveWindow = (XtoqWindow *)[NSApp mainWindow];
    
    if (moveWindow != nil) {        
        xcwm_context_t *moveContext = [moveWindow getContext];        
        NSRect moveFrame = [moveWindow frame];
        int x = (int)moveFrame.origin.x;
        int y = [self osxToXserver:(int)moveFrame.origin.y
					  windowHeight:moveContext->height] - WINDOWBAR;
        int width = (int)moveFrame.size.width;
        int height = (int)moveFrame.size.height - WINDOWBAR;

        xcwm_configure_window(moveContext, x, y - height, height, width);
		[[moveWindow contentView] setNeedsDisplay: YES];
    }    
}

@end

void eventHandler (xcwm_event_t *event)
{
    xcwm_context_t *context = xcwm_event_get_context(event);
    if (xcwm_event_get_type(event) == XTOQ_DAMAGE) {
	  [referenceToSelf updateImage: context];
    } else if (xcwm_event_get_type(event) == XTOQ_CREATE) {
        NSLog(@"Window was created");
        [referenceToSelf createNewWindow: context];
    } else if (xcwm_event_get_type(event) == XTOQ_DESTROY) {
        NSLog(@"Window was destroyed");
        [referenceToSelf destroyWindow: context];
    } else { 
        NSLog(@"Hey I'm Not damage!"); 
    }
    free(event);
}
