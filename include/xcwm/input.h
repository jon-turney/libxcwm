/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>
 *
 * xcwm/event.h
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _XCWM_INPUT_H_
#define _XCWM_INPUT_H_

#ifndef __XCWM_INDIRECT__
#error "Please #include <xcwm/xcwm.h> instead of this file directly."
#endif

/**
 * Send key event to the X server.
 * @param context The context the event occured in.
 * @param code The keycode of event.
 * @param state 1 if key has been pressed, 0 if key released.
 */
void
xcwm_input_key_event(xcwm_context_t *context, uint8_t code, int state);

/**
 * Uses the XTEST protocol to send input events to the X Server (The X Server
 * is usually in the position of sending input events to a client). The client
 * will often choose to send coordinates through mouse motion and set the params
 * x & y to 0 here.
 * @param window The window the event occured in.
 * @param x - x coordinate
 * @param y - y coordinate
 * @param button The mouse button pressed.
 * @param state 1 if the mouse button is pressed down, 0 if released.
 */
void
xcwm_input_mouse_button_event(xcwm_window_t *window,
                              long x, long y, int button,
                              int state);

/**
 * Handler for mouse motion event.
 * @param context xcwm_context_t
 * @param x - x coordinate
 * @param y - y coordinate
 */
void
xcwm_input_mouse_motion(xcwm_context_t *context, long x, long y, int button);


#endif  /* _XCWM_INPUT_H_ */
