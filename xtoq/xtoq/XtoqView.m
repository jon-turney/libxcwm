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

#import "XtoqView.h"

@implementation XtoqView

/**
 *  This is the initializer.
 */
- (id)
initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    
    if (self) {
        screen = ":1";
        NSLog(@"screen = %s", screen);
        xcbContext = xtoq_init(screen);
        imageT = xtoq_get_image(xcbContext);
        image = [[XtoqImageRep alloc] initWithData:imageT];
        
        // Leaving these in for testing
        // file = @"Xtoq.app/Contents/Resources/Mac-Logo.jpg";
        // image2 = [[NSImage alloc] initWithContentsOfFile:(file)];
        
    }
    return self;
}


/**
 *  This is the initializer.
 */
- (id)
initWithImage:(XtoqImageRep *)newImage {
    
    frame = NSMakeRect(0, 0, newImage.size.width, newImage.size.height);
    self = [super initWithFrame:frame];
    
    if (self) {
        
//        screen = ":1";
//        NSLog(@"screen = %s", screen);
//        xcbContext = xtoq_init(screen);
//        imageT = xtoq_get_image(xcbContext);
        image = newImage;//[[XtoqImageRep alloc] initWithData:imageT];
        [image draw];
        [[self window] flushWindow];

        
    }
    return self;
}

/**
 *  This function draws the initial image to the window.
 */
- (void)
drawRect:(NSRect)dirtyRect {
    [[NSGraphicsContext currentContext]
     setImageInterpolation:NSImageInterpolationHigh];
    
    // NEED TO CHANGE to remove hard coded size
   // NSSize imageSize = { 1024,768 };
  //  NSRect destRect;
  //  destRect.size = imageSize;
    [image draw];
    [[self window] flushWindow];
    // Leaving in for testing
    //[image2 drawInRect:destRect fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
}

/**
 *  This is necessary for accepting input.
 */
- (BOOL)
acceptsFirstResponder {
    return YES;
}

/**
 *  This is the function that captures the event which is 
 *  the down arrow key, not the numpad down arrow key.
 *  It updates the image on the screen when the down arrow is pressed.
 */
- (void)
keyDown:(NSEvent *)theEvent {      
    NSString *characters = [theEvent characters];
    int key = [characters characterAtIndex:0];
    
    [[NSNotificationCenter defaultCenter]
     postNotificationName:@"viewKeyDownEvent" object:self 
     userInfo: [[NSDictionary alloc]
                initWithObjectsAndKeys:[[NSArray alloc]
                                        initWithObjects: theEvent, nil], @"1", nil]];
    
//    // NEED TO CHANGE to remove hard coded size
//    NSSize imageSize = {1024, 768};
//    
//    NSRect destRect;
//    destRect.size = imageSize;
//    
//    if (key == NSDownArrowFunctionKey) {        
//        // Get update image
//  //      imageT = xtoq_get_image(xcbContext);
//        
//        image = [[XtoqImageRep alloc] initWithData:imageT];
//        [image drawInRect:destRect];
//        [image draw];        
//        [[self window] flushWindow];
//        
//    } else {
        [super keyDown:theEvent];
//    }
}

- (void)setImage:(XtoqImageRep *)newImage {
    //[newImage retain];
    //[image release];
    image = newImage;
    [[self window] flushWindow];
    [self setNeedsDisplay:YES];
}

@end

