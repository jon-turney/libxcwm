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
#include <xcb/xcb_icccm.h>

enum xcwm_context_flags_t {
    XCWM_DISABLE_SHM = 1,         // Disable SHM use on this context
    XCWM_VERBOSE_LOG_XEVENTS = 2, // Log all Xevent
};
typedef enum xcwm_context_flags_t xcwm_context_flags_t;


/* forward reference to xcwm_window_t type */
struct xcwm_window_t;
typedef struct xcwm_window_t xcwm_window_t;

struct xcwm_context_t;
typedef struct xcwm_context_t xcwm_context_t;

/**
 * Sets up the connection and creates the context
 * @param display the display to connect to
 * @flags flags to be applied to the context
 * @return The context
 */
xcwm_context_t *
xcwm_context_open(char *display, xcwm_context_flags_t flags);

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

/**
 * Get the connection for this context.
 * @param context The context to get connection for
 * @return The connection for this context.
 */
xcb_connection_t *
xcwm_context_get_connection(xcwm_context_t const *context);

#endif  /* _XCWM_CONTEXT_H_ */
