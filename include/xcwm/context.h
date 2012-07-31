/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>
 *
 * xcwm/context.h
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

#ifndef _XCWM_CONTEXT_H_
#define _XCWM_CONTEXT_H_

#ifndef __XCWM_INDIRECT__
#error "Please #include <xcwm/xcwm.h> instead of this file directly."
#endif

#include <xcb/xcb.h>
#include <xcb/damage.h>

/* Abstract types for context data types 
 * FIXME: Move this to window.h once accessor API functions in place */
struct xcwm_window_t;
typedef struct xcwm_window_t xcwm_window_t;

/**
 * Struct to hold ICCCM/EWMH data for context.
 */
struct xcwm_wm_atoms_t;
typedef struct xcwm_wm_atoms_t xcwm_wm_atoms_t;

/* Structure to hold connection data */
struct xcwm_context_t {
    xcb_connection_t *conn;
    int conn_screen;
    xcwm_window_t *root_window;
    int damage_event_mask;
    xcwm_wm_atoms_t *atoms;
};
typedef struct xcwm_context_t xcwm_context_t;

/**
 * Sets up the connection and grabs the root window from the specified screen
 * @param display the display to connect to
 * @return The root context which contains the root window
 */
xcwm_context_t *
xcwm_context_open (char *display);

/**
 * Closes the windows open on the X Server, the connection, and the event
 * loop.
 * @param context The context to close.
 */
void
xcwm_context_close(xcwm_context_t *context);

/**
 * Get the root window for this context.
 * @param context The context to get root window from.
 * @return The root window for this context.
 */
xcwm_window_t *
xcwm_context_get_root_window(xcwm_context_t const *context);

#endif  /* _XCWM_CONTEXT_H_ */
