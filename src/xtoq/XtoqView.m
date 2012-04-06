/*Copyright (C) 2012 Aaron Skomra and Ben Huddle

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

#import "XtoqView.h"

#define RECTLOG(rect) NSLog(@""  # rect @" x:%f y:%f w:%f h:%f", \
                            rect.origin.x, rect.origin.y, rect.size.width, \
                            rect.size.height)

@implementation XtoqView

/**
 *  This is the initializer.
 */
- (id)
   initWithFrame: (NSRect)frame
{
    self = [super initWithFrame:frame];

    if (self) {
        notificationCenter = [NSNotificationCenter defaultCenter];
        [[self window] flushWindow];
        [self setNeedsDisplay:YES];
    }
    return self;
}

-(void) setXcwmWindow: (xcwm_window_t *) xcwmWindow
{
    viewXcwmWindow = xcwmWindow;
}

// Overridden by subclasses to draw the receiver’s image within the
// passed-in rectangle.
-(void) drawRect: (NSRect)dirtyRect
{
    xcwm_image_t *imageT;
    float y_transformed;
    XtoqImageRep *imageNew;

    xcwm_event_get_thread_lock();
    imageT = xcwm_image_copy_damaged(viewXcwmWindow);
    if (imageT->image) {
        y_transformed = (viewXcwmWindow->height
                         - viewXcwmWindow->damaged_y
                         - viewXcwmWindow->damaged_height) / 1.0;
        imageNew = [[XtoqImageRep alloc]
                    initWithData:imageT
                               x:((viewXcwmWindow->damaged_x))
                               y:y_transformed];
        [imageNew draw];
        [imageNew destroy];

        // Remove the damage
        xcwm_window_remove_damage(viewXcwmWindow);
    }
    xcwm_event_release_thread_lock();
}

- (void)setPartialImage: (NSRect)newDamageRect
{
    [self setNeedsDisplayInRect: newDamageRect];
}

- (BOOL)isOpaque
{
    return YES;
}

@end
