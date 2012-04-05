/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net> All rights
 * reserved
 *
 * event_loop.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <xcwm/xcwm.h>
#include "xcwm_internal.h"

typedef struct _connection_data {
    xcb_connection_t *conn;
    xcwm_event_cb_t callback;
} _connection_data;

/* The thread that is running the event loop */
pthread_t _event_thread = 0;

pthread_mutex_t _event_thread_lock;

/* Functions only called within event_loop.c */
void
*
run_event_loop(void *thread_arg_struct);

/* Functions included in xcwm.h */
int
xcwm_get_event_thread_lock(void)
{
    return pthread_mutex_lock(&_event_thread_lock);
}

int
xcwm_release_event_thread_lock(void)
{
    return pthread_mutex_unlock(&_event_thread_lock);
}

/* Functions included in xcwm_internal.h */

int
_xcwm_start_event_loop(xcb_connection_t *conn,
                       xcwm_event_cb_t event_callback)
{
    _connection_data *conn_data;
    int ret_val;
    int oldstate;

    conn_data = malloc(sizeof(_connection_data));
    assert(conn_data);

    conn_data->conn = conn;
    conn_data->callback = event_callback;

    pthread_mutex_init(&_event_thread_lock, NULL);

    ret_val = pthread_create(&_event_thread,
                             NULL,
                             run_event_loop,
                             (void *)conn_data);

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);

    return ret_val;
}

int
_xcwm_stop_event_loop(void)
{
    if (_event_thread) {
        return pthread_cancel(_event_thread);
    }
    return 1;
}

void *
run_event_loop(void *thread_arg_struct)
{
    _connection_data *conn_data;
    xcb_connection_t *event_conn;
    xcb_generic_event_t *evt;
    xcwm_event_t *return_evt;
    xcwm_event_cb_t callback_ptr;

    conn_data = thread_arg_struct;
    event_conn = conn_data->conn;
    callback_ptr = conn_data->callback;

    free(thread_arg_struct);

    /* Start the event loop, and flush if first */
    xcb_flush(event_conn);

    while ((evt = xcb_wait_for_event(event_conn))) {
        if ((evt->response_type & ~0x80) == _damage_event) {
            xcb_damage_notify_event_t *dmgevnt =
                (xcb_damage_notify_event_t *)evt;
            int old_x;
            int old_y;
            int old_height;
            int old_width;

            return_evt = malloc(sizeof(xcwm_event_t));
            return_evt->event_type = XTOQ_DAMAGE;
            return_evt->window =
                _xcwm_get_window_node_by_window_id(dmgevnt->drawable);
            if (!return_evt->window) {
                free(return_evt);
                continue;
            }

            /* Increase the damaged area if new damage is outside the
             * area already marked - this should be set back to 0 by 0
             * when area is actually redrawn. This is likely to be
             * done in another thread that handles window redraws */
            xcwm_get_event_thread_lock();

            old_x = return_evt->window->damaged_x;
            old_y = return_evt->window->damaged_y;
            old_width = return_evt->window->damaged_width;
            old_height = return_evt->window->damaged_height;

            if (return_evt->window->damaged_width == 0) {
                /* We know something is damaged */
                return_evt->window->damaged_x = dmgevnt->area.x;
                return_evt->window->damaged_y = dmgevnt->area.y;
                return_evt->window->damaged_width = dmgevnt->area.width;
                return_evt->window->damaged_height = dmgevnt->area.height;
            }
            else {
                /* Is the new damage bigger than the old */
                if (old_x > dmgevnt->area.x) {
                    return_evt->window->damaged_x = dmgevnt->area.x;
                }
                if (old_y > dmgevnt->area.y) {
                    return_evt->window->damaged_y = dmgevnt->area.y;
                }
                if (old_width < dmgevnt->area.width) {
                    return_evt->window->damaged_width = dmgevnt->area.width;
                }
                if (old_height < dmgevnt->area.height) {
                    return_evt->window->damaged_height = dmgevnt->area.height;
                }
            }
            xcwm_release_event_thread_lock();

            if (((old_x > dmgevnt->area.x) || (old_y > dmgevnt->area.y))
                || ((old_width < dmgevnt->area.width)
                    || (old_height < dmgevnt->area.height))) {
                /* We should only reach here if this damage event
                 * actually increases that area of the window marked
                 * for damage. */
                callback_ptr(return_evt);
            }
        }
        else {
            switch (evt->response_type & ~0x80) {
            case 0:
            {
                /* Error case. Something very bad has happened. Spit
                 * out some hopefully useful information and then
                 * die.
                 * FIXME: Decide under what circumstances we should
                 * acutally kill the application. */
                xcb_generic_error_t *err = (xcb_generic_error_t *)evt;
                fprintf(stderr, "Error received in event loop.\n"
                        "Error code: %i\n",
                        err->error_code);
                if ((err->error_code >= XCB_VALUE)
                    && (err->error_code <= XCB_FONT)) {
                    xcb_value_error_t *val_err = (xcb_value_error_t *)evt;
                    fprintf(stderr, "Bad value: %i\n"
                            "Major opcode: %i\n"
                            "Minor opcode: %i\n",
                            val_err->bad_value,
                            val_err->major_opcode,
                            val_err->minor_opcode);
                }
                break;
            }

            case XCB_EXPOSE:
            {
                xcb_expose_event_t *exevnt = (xcb_expose_event_t *)evt;

                printf(
                    "Window %u exposed. Region to be redrawn at location (%d, %d), ",
                    exevnt->window, exevnt->x, exevnt->y);
                printf("with dimentions (%d, %d).\n", exevnt->width,
                       exevnt->height);

                return_evt = malloc(sizeof(xcwm_event_t));
                return_evt->event_type = XTOQ_EXPOSE;
                callback_ptr(return_evt);
                break;
            }

            case XCB_CREATE_NOTIFY:
            {
                /* We don't actually allow our client to create its
                 * window here, wait until the XCB_MAP_REQUEST */
                printf("Got create notify\n");
                break;
            }

            case XCB_DESTROY_NOTIFY:
            {
                // Window destroyed in root window
                xcb_destroy_notify_event_t *notify =
                    (xcb_destroy_notify_event_t *)evt;
                xcwm_window_t *window =
                    _xcwm_destroy_window(event_conn, notify);

                if (!window) {
                    /* Not a window in the list, don't try and destroy */
                    break;
                }

                return_evt = malloc(sizeof(xcwm_event_t));
                return_evt->event_type = XTOQ_DESTROY;
                return_evt->window = window;

                callback_ptr(return_evt);
                free(window);
                break;
            }

            case XCB_MAP_REQUEST:
            {
                xcb_map_request_event_t *request =
                    (xcb_map_request_event_t *)evt;
                return_evt = malloc(sizeof(xcwm_event_t));
                return_evt->window = _xcwm_window_created(event_conn, request);
                if (!return_evt->window) {
                    free(return_evt);
                    break;
                }
                _xcwm_map_window(event_conn, return_evt->window);
                return_evt->event_type = XTOQ_CREATE;
                callback_ptr(return_evt);
                break;
            }

            case XCB_CONFIGURE_NOTIFY:
            {
                break;
            }

            case XCB_CONFIGURE_REQUEST:
            {
                xcb_configure_request_event_t *request =
                    (xcb_configure_request_event_t *)evt;
                printf("Got configure request: ");
                printf("x = %i, y = %i, w = %i, h = %i\n", request->x,
                       request->y,
                       request->width,
                       request->height);

                /* Change the size of the window, but not its position */
                _xcwm_resize_window(event_conn, request->window,
                                    request->width, request->height);
                break;
            }

            case XCB_KEY_PRESS:
            {
                printf("X Key press from xserver-");
                xcb_button_press_event_t *kp =
                    (xcb_button_press_event_t *)evt;
                printf("Key pressed in window %u detail %c\n",
                       kp->event, kp->detail);
                break;
            }

            case XCB_BUTTON_PRESS:
            {
                printf("X Button press from xserver ");
                xcb_button_press_event_t *bp =
                    (xcb_button_press_event_t *)evt;
                printf("in window %u, at coordinates (%d,%d)\n",
                       bp->event, bp->event_x, bp->event_y);
                break;
            }

            case XCB_BUTTON_RELEASE:
            {
                printf("X Button release from xserver ");
                xcb_button_press_event_t *bp =
                    (xcb_button_press_event_t *)evt;
                printf("in window %u, at coordinates (%d,%d)\n",
                       bp->event, bp->event_x, bp->event_y);
                break;
            }

            case XCB_MOTION_NOTIFY:
            {
                //printf("X mouse motion from from xserver-");
                xcb_button_press_event_t *bp =
                    (xcb_button_press_event_t *)evt;
                // printf ("mouse motion in window %ld, at coordinates (%d,%d)\n",
                //        bp->event, bp->event_x, bp->event_y );
                break;
            }

            default:
            {
                printf("UNKNOWN EVENT: %i\n", (evt->response_type & ~0x80));
                break;
            }
            }
        }
        /* Free the event */
        free(evt);
    }
    return NULL;
}

int
xcwm_event_get_type(xcwm_event_t const *event)
{
    return event->event_type;
}

/* FIXME: Do we still need a context in the event? Probably not... */
xcwm_context_t *
xcwm_event_get_context(xcwm_event_t const *event)
{
    return event->context;
}

xcwm_window_t *
xcwm_event_get_window(xcwm_event_t const *event)
{
    return event->window;
}
