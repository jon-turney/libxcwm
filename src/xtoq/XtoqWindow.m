/*Copyright (C) 2012 Ben Huddle, Braden Wooley, David Snyder
 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#import "XtoqWindow.h"

@implementation XtoqWindow



-(id) initWithContentRect: (NSRect)contentRect 
                styleMask: (NSUInteger)aStyle 
                  backing: (NSBackingStoreType)bufferingType 
                    defer: (BOOL)flag {
    
    notificationCenter = [NSNotificationCenter defaultCenter];
    
    // Hack job, probably -- David
    [[NSNotificationCenter defaultCenter] addObserver: self 
                                             selector: @selector(windowDidBecomeKey:) 
                                                 name: @"NSWindowDidBecomeKeyNotification" 
                                               object: self];
    // End Hack job -- David
    
    XtoqWindow *result = [super initWithContentRect: contentRect
                                          styleMask: aStyle
                                            backing: bufferingType
                                              defer: flag];
	[self setAcceptsMouseMovedEvents: YES];
    return result;
}

-(void) makeKeyAndOrderFront: (id)sender {
    [super makeKeyAndOrderFront: sender];
}

-(void) setXcwmWindow: (xcwm_window_t *) aWindow
       andXcwmContext: (xcwm_context_t *) aContext {
    xcwmWindow = aWindow;
    xcwmContext = aContext;
}

-(xcwm_window_t *) getXcwmWindow {
    return xcwmWindow;
}

- (BOOL) windowShouldClose: (id)sender {    
    // send notification to controller to close the window
    XtoqWindow * aWindow = self;
    NSDictionary * dictionary = [NSDictionary dictionaryWithObject: aWindow 
                                                            forKey: @"1"];    
    [notificationCenter postNotificationName: @"XTOQdestroyTheWindow" 
                                      object: self
                                    userInfo: dictionary];  
    
    // keep window from closing till server tells it to
    return NO;
}

-(void)windowDidBecomeKey: (NSNotification *)note {
    
    xcwm_set_input_focus(xcwmContext, xcwmWindow);
    xcwm_set_window_to_top(xcwmContext, xcwmWindow);
}


-(void) mouseMoved: (NSEvent *) event {
    NSMutableDictionary *InfoDict;
    NSNumber *xNum, *yNum;

	NSPoint location = [event locationInWindow];
	NSRect frame = [self frame];

	location.x += frame.origin.x;
	location.y += frame.origin.y;
            
	xNum = [[NSNumber alloc] initWithFloat: location.x];
	yNum = [[NSNumber alloc] initWithFloat: location.y];
            
	InfoDict = [[NSMutableDictionary alloc] initWithCapacity: 2];
	[InfoDict setObject: xNum forKey: @"1"];
	[InfoDict setObject: yNum forKey: @"2"];
            
	[[NSNotificationCenter defaultCenter]
	  postNotificationName: @"MouseMovedEvent" 
	                object: self 
                  userInfo: InfoDict];
}

@end
