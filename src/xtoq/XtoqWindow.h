/*Copyright (C) 2012 Ben Huddle, Braden Wooley
 
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

// Placehoder

#import <Cocoa/Cocoa.h>
#import <xcwm/xcwm.h>
#import "XtoqView.h"

@interface XtoqWindow : NSWindow {
    xcwm_context_t *xcwmContext;  // The context of the window.
    xcwm_window_t *xcwmWindow;    // The xcwm window assoicated with this one
    NSNotificationCenter * notificationCenter;
}

/**
 * Used for initialization of a window.
 */
-(id) initWithContentRect: (NSRect)contentRect styleMask: (NSUInteger)aStyle 
                  backing: (NSBackingStoreType)bufferingType defer: (BOOL)flag;

/*
 * Orders a specific window to the front and makes it key window.
 */
-(void) makeKeyAndOrderFront:(id)sender;

/**
 * Set xcwm_window_t associated with this window and the
 * xcwm_context_t context.
 * @param aWindow The xcwm_window_t for this window.
 * @param aContext The xcwm_context_t for this window.
 */
-(void) setXcwmWindow: (xcwm_window_t *) aWindow
       andXcwmContext: (xcwm_context_t *) aContext;;

/**
 * Function for getting the xcwm_window_t associated with this window.
 * @return The xcwm_window_t for this window.
 */
-(xcwm_window_t *) getXcwmWindow;

/**
 * Catches the close event from clicking the red button or from preformClose.
 */
- (BOOL) windowShouldClose: (id)sender;

/**
 * Catches the event that a window gains focus
 */
-(void) windowDidBecomeKey: (NSNotification *)note;

/**
 * Handler for mouse movement events.
 * @param event The mouse movement event.
 */
-(void) mouseMoved: (NSEvent *)event;

@end
