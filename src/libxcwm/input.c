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
#include <xcb/xtest.h>
#include "xcwm_internal.h"

void
xcwm_input_key_event(xcwm_context_t *context, uint8_t code, int state)
{
    xcb_window_t none = { XCB_NONE };
    int key_state;

    if (state) {
        key_state = XCB_KEY_PRESS;
    }
    else {
        key_state = XCB_KEY_RELEASE;
    }

    xcb_test_fake_input(context->conn, key_state, code,
                        XCB_CURRENT_TIME, none, 0, 0, 1);

    xcb_flush(context->conn);
    printf("Injected key event - key code %i\n", code);
}

void
xcwm_input_mouse_button_event(xcwm_window_t *window,
                              int button, int state)
{
    int button_state;

    if (state) {
        button_state = XCB_BUTTON_PRESS;
    }
    else {
        button_state = XCB_BUTTON_RELEASE;
    }
    /* FIXME: Why are we passing a specifing window ID here, when
     * other handlers use either the root window or none. */
    xcb_test_fake_input(window->context->conn, button_state, button,
                        XCB_CURRENT_TIME,
                        window->window_id, 0, 0, 0);
    xcb_flush(window->context->conn);
    printf("Injected mouse event - button %d, state: %d\n",
           button, state);
}

void
xcwm_input_mouse_motion(xcwm_context_t *context, long x, long y)
{
    xcb_test_fake_input(context->conn, XCB_MOTION_NOTIFY, 0, XCB_CURRENT_TIME,
                        context->root_window->window_id, x, y, 0);
    xcb_flush(context->conn);
}
