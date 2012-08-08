/* Copyright (c) 2012 Apple Inc
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

#ifndef _XCWM_EVENT_H_
#define _XCWM_EVENT_H_

#ifndef __XCWM_INDIRECT__
#error "Please #include <xcwm/xcwm.h> instead of this file directly."
#endif

/* Abstract types for xcwm data types */
struct xcwm_event_t;
typedef struct xcwm_event_t xcwm_event_t;

typedef void (*xcwm_event_cb_t)(xcwm_event_t const *event);

/* Event types */
typedef enum xcwm_event_type_t {
    XCWM_EVENT_WINDOW_DAMAGE = 0,
    XCWM_EVENT_WINDOW_EXPOSE,
    XCWM_EVENT_WINDOW_CREATE,
    XCWM_EVENT_WINDOW_DESTROY,
    XCWM_EVENT_WINDOW_NAME
} xcwm_event_type_t;

/**
 *  Return the event type for the given event.
 */
xcwm_event_type_t
xcwm_event_get_type(xcwm_event_t const *event);

/**
 * Return the window for the given event.
 * @param event The event
 * @return The window connected to the event.
 */
xcwm_window_t *
xcwm_event_get_window(xcwm_event_t const *event);

/**
 * Starts the event loop and listens on the connection specified in
 * the given xcwm_context_t. Uses callback as the function to call
 * when an event of interest is received. Callback must be able to
 * take an xcwm_event_t as its one and only parameter.
 * @param context The context containing the connection to listen
 * for events on.
 * @param callback The function to call when an event of interest is
 * received.
 * @return 0 on success, otherwise non-zero
 */
int
xcwm_event_start_loop(xcwm_context_t *context, xcwm_event_cb_t callback);

/**
 * Request a lock on the mutex for the event loop thread. Blocks
 * until lock is aquired, or error occurs.
 * @return 0 if successful, otherwise non-zero
 */
int
xcwm_event_get_thread_lock(void);

/**
 * Release the lock on the mutex for the event loop thread.
 * @return 0 if successsful, otherwise non-zero
 */
int
xcwm_event_release_thread_lock(void);

#endif /* _XCWM_EVENT_H_ */
