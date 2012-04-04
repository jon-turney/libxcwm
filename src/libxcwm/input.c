/*Copyright (C) 2012 Aaron Skomra
 
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

#include <stdio.h>
#include <xcwm/xcwm.h>

#include "xcwm_internal.h"

void
xcwm_input_key_event (xcwm_context_t *context, uint8_t code, int state)
{
    xcb_window_t none = { XCB_NONE };
    int key_state;

    if (state) {
        key_state = XCB_KEY_PRESS;
    } else {
        key_state = XCB_KEY_RELEASE;
    }

    xcb_test_fake_input( context->conn, key_state, code, 
						 XCB_CURRENT_TIME, none, 0, 0, 1 );  
    
    xcb_flush(context->conn);
    printf("xcwm.c received key event - uint8_t '%i', to context.window %u\n", code, context->window);
}

void
xcwm_input_mouse_button_event (xcwm_context_t *context,
                               long x, long y,
                               int button, int state)
{
    int button_state;

    if (state) {
        button_state = XCB_BUTTON_PRESS;
    } else {
        button_state = XCB_BUTTON_RELEASE;
    }
    xcb_test_fake_input (context->conn, button_state, button, XCB_CURRENT_TIME,
                         context->window, 0, 0, 0);
	xcb_flush(context->conn);
    printf("Mouse event received by xtoq.c - (%ld,%ld), state: %d\n",
           x, y, state);
}

void
xcwm_input_mouse_motion (xcwm_context_t *context, long x, long y,int button)
{
    xcb_test_fake_input (context->conn, XCB_MOTION_NOTIFY, 0, XCB_CURRENT_TIME,
                         root_context->window, x, y, 0);
	xcb_flush(context->conn);
}
