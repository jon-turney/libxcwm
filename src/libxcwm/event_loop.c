/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>
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
#include <xcb/composite.h>
#include "xcwm_internal.h"

/* Definition of abstract data type for event */
struct xcwm_event_t {
    xcwm_window_t *window;
    xcwm_event_type_t event_type;
};

/* Locally used data structure */
typedef struct _connection_data {
    xcwm_context_t *context;
    xcwm_event_cb_t callback;
} _connection_data;

/* The thread that is running the event loop */
pthread_t _event_thread = 0;

pthread_mutex_t _event_thread_lock;

/* Functions only called within event_loop.c */
void *
run_event_loop(void *thread_arg_struct);

/* Functions included in event.h */
int
xcwm_event_get_thread_lock(void)
{
    return pthread_mutex_lock(&_event_thread_lock);
}

int
xcwm_event_release_thread_lock(void)
{
    return pthread_mutex_unlock(&_event_thread_lock);
}

int
xcwm_event_start_loop(xcwm_context_t *context,
                       xcwm_event_cb_t event_callback)
{
    _connection_data *conn_data;
    int ret_val;
    int oldstate;

    conn_data = malloc(sizeof(_connection_data));
    assert(conn_data);

    conn_data->context = context;
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
_xcwm_event_stop_loop(void)
{
    if (_event_thread) {
        return pthread_cancel(_event_thread);
    }
    return 1;
}

static void
_xcwm_window_composite_pixmap_release(xcwm_window_t *window)
{
  if (window->composite_pixmap_id)
    {
      xcb_free_pixmap(window->context->conn, window->composite_pixmap_id);
      window->composite_pixmap_id = 0;
    }
}

static void
_xcwm_window_composite_pixmap_update(xcwm_window_t *window)
{
  _xcwm_window_composite_pixmap_release(window);
  window->composite_pixmap_id = xcb_generate_id(window->context->conn);
  xcb_composite_name_window_pixmap(window->context->conn, window->window_id, window->composite_pixmap_id);
}

/*
  Generate a XCWM_EVENT_WINDOW_CREATE event for all
  existing mapped top-level windows when we start
*/
static void
_xcwm_windows_adopt(xcwm_context_t *context, xcwm_event_cb_t callback_ptr)
{
    xcb_query_tree_cookie_t tree_cookie = xcb_query_tree(context->conn, context->root_window->window_id);
    xcb_query_tree_reply_t *reply = xcb_query_tree_reply(context->conn, tree_cookie, NULL);
    if (NULL == reply) {
        return;
    }

    int len = xcb_query_tree_children_length(reply);
    xcb_window_t *children = xcb_query_tree_children(reply);

    int i;
    for (i = 0; i < len; i ++) {
        xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(context->conn, children[i]);
        xcb_get_window_attributes_reply_t *attr = xcb_get_window_attributes_reply(context->conn, cookie, NULL);

        if (!attr) {
            fprintf(stderr, "Couldn't get attributes for window 0x%08x\n", children[i]);
            continue;
        }

        if (attr->map_state == XCB_MAP_STATE_VIEWABLE) {
            printf("window 0x%08x viewable\n", children[i]);

            xcwm_window_t *window = _xcwm_window_create(context, children[i], context->root_window->window_id);
            if (!window) {
                continue;
            }

            _xcwm_window_composite_pixmap_update(window);

            xcwm_event_t *return_evt = malloc(sizeof(xcwm_event_t));
            if (!return_evt) {
                continue;
            }

            return_evt->window = window;
            return_evt->event_type = XCWM_EVENT_WINDOW_CREATE;

            callback_ptr(return_evt);
        }
        else {
            printf("window 0x%08x non-viewable\n", children[i]);
        }

        free(attr);
    }

    free(reply);
}

void *
run_event_loop(void *thread_arg_struct)
{
    _connection_data *conn_data;
    xcwm_context_t *context;
    xcb_connection_t *event_conn;
    xcb_generic_event_t *evt;
    xcwm_event_t *return_evt;
    xcwm_event_cb_t callback_ptr;

    conn_data = thread_arg_struct;
    context = conn_data->context;
    event_conn = context->conn;
    callback_ptr = conn_data->callback;

    free(thread_arg_struct);

    _xcwm_windows_adopt(context, callback_ptr);

    /* Start the event loop, and flush if first */
    xcb_flush(event_conn);

    while ((evt = xcb_wait_for_event(event_conn))) {
        if ((evt->response_type & ~0x80) == context->damage_event_mask) {
            xcb_damage_notify_event_t *dmgevnt =
                (xcb_damage_notify_event_t *)evt;
            int old_x;
            int old_y;
            int old_height;
            int old_width;
            xcwm_rect_t dmg_area;

            return_evt = malloc(sizeof(xcwm_event_t));
            return_evt->event_type = XCWM_EVENT_WINDOW_DAMAGE;
            return_evt->window =
                _xcwm_get_window_node_by_window_id(dmgevnt->drawable);
            if (!return_evt->window) {
                free(return_evt);
                continue;
            }

            /* Increase the damaged area of window if new damage is
             * larger than current. */
            xcwm_event_get_thread_lock();

            /* Initial damage events for override-redirect windows are
             * reported relative to the root window, subsequent events
             * are relative to the window itself. We also catch cases
             * where the damage area is larger than the bounds of the
             * window. */
            if (return_evt->window->initial_damage == 1
                || (dmgevnt->area.width > return_evt->window->bounds->width)
                || (dmgevnt->area.height > return_evt->window->bounds->height) ) {
                xcb_xfixes_region_t region =
                    xcb_generate_id(return_evt->window->context->conn);
                xcb_rectangle_t rect;

                /* Remove the damage */
                xcb_xfixes_create_region(return_evt->window->context->conn,
                                         region,
                                         1,
                                         &dmgevnt->area);
                xcb_damage_subtract(return_evt->window->context->conn,
                                    return_evt->window->damage,
                                    region,
                                    XCB_NONE);
                
                /* Add new damage area for entire window */
                rect.x = 0;
                rect.y = 0;
                rect.width = return_evt->window->bounds->width;
                rect.height = return_evt->window->bounds->height;
                xcb_xfixes_set_region(return_evt->window->context->conn,
                                      region,
                                      1,
                                      &rect);
                xcb_damage_add(return_evt->window->context->conn,
                               return_evt->window->window_id,
                               region);

                return_evt->window->initial_damage = 0;
                xcb_xfixes_destroy_region(return_evt->window->context->conn,
                                          region);
                free(return_evt);
                xcwm_event_release_thread_lock();
                continue;
            }

            dmg_area.x = dmgevnt->area.x;
            dmg_area.y = dmgevnt->area.y;
            dmg_area.width = dmgevnt->area.width;
            dmg_area.height = dmgevnt->area.height;

            old_x = return_evt->window->dmg_bounds->x;
            old_y = return_evt->window->dmg_bounds->y;
            old_width = return_evt->window->dmg_bounds->width;
            old_height = return_evt->window->dmg_bounds->height;

            if (return_evt->window->dmg_bounds->width == 0) {
                /* We know something is damaged */
                return_evt->window->dmg_bounds->x = dmg_area.x;
                return_evt->window->dmg_bounds->y = dmg_area.y;
                return_evt->window->dmg_bounds->width = dmg_area.width;
                return_evt->window->dmg_bounds->height = dmg_area.height;
            }
            else {
                /* Is the new damage bigger than the old */
                if (old_x > dmg_area.x) {
                    return_evt->window->dmg_bounds->x = dmg_area.x;
                }
                if (old_y > dmg_area.y) {
                    return_evt->window->dmg_bounds->y = dmg_area.y;
                }
                if (old_width < dmg_area.width) {
                    return_evt->window->dmg_bounds->width = dmg_area.width;
                }
                if (old_height < dmg_area.height) {
                    return_evt->window->dmg_bounds->height = dmg_area.height;
                }
            }
            xcwm_event_release_thread_lock();

            if (((old_x > dmg_area.x) || (old_y > dmg_area.y))
                || ((old_width < dmg_area.width)
                    || (old_height < dmg_area.height))) {
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
                printf("with dimensions (%d, %d).\n", exevnt->width,
                       exevnt->height);

                return_evt = malloc(sizeof(xcwm_event_t));
                return_evt->event_type = XCWM_EVENT_WINDOW_EXPOSE;
                callback_ptr(return_evt);
                break;
            }

            case XCB_CREATE_NOTIFY:
            {
                /* We don't actually allow our client to create its
                 * window here, wait until the XCB_MAP_REQUEST */
                break;
            }

            case XCB_DESTROY_NOTIFY:
            {
                // Window destroyed in root window
                xcb_destroy_notify_event_t *notify =
                    (xcb_destroy_notify_event_t *)evt;
                xcwm_window_t *window =
                    _xcwm_window_remove(event_conn, notify->window);

                if (!window) {
                    /* Not a window in the list, don't try and destroy */
                    break;
                }

                return_evt = malloc(sizeof(xcwm_event_t));
                return_evt->event_type = XCWM_EVENT_WINDOW_DESTROY;
                return_evt->window = window;

                callback_ptr(return_evt);

                _xcwm_window_composite_pixmap_release(window);

                // Release memory for the window
                _xcwm_window_release(window);
                break;
            }

            case XCB_MAP_NOTIFY:
            {
                xcb_map_notify_event_t *notify =
                    (xcb_map_notify_event_t *)evt;
                return_evt = malloc(sizeof(xcwm_event_t));
                /* notify->event holds parent of the window */

                xcwm_window_t *window =
                    _xcwm_get_window_node_by_window_id(notify->window);
                if (!window)
                {
                    /*
                      No MAP_REQUEST for override-redirect windows, so
                      need to create the xcwm_window_t for it now
                    */
                    printf("MAP_NOTIFY without MAP_REQUEST\n");
                    window =
                        _xcwm_window_create(context, notify->window,
                                            notify->event);

                    _xcwm_window_composite_pixmap_update(window);

                    return_evt->window = window;
                    return_evt->event_type = XCWM_EVENT_WINDOW_CREATE;
                    callback_ptr(return_evt);
                }

                _xcwm_window_composite_pixmap_update(window);
                break;
            }

            case XCB_MAP_REQUEST:
            {
                xcb_map_request_event_t *request =
                    (xcb_map_request_event_t *)evt;
                return_evt = malloc(sizeof(xcwm_event_t));
                return_evt->window =
                    _xcwm_window_create(context, request->window,
                                        request->parent);
                if (!return_evt->window) {
                    free(return_evt);
                    break;
                }
                _xcwm_map_window(event_conn, return_evt->window);
                return_evt->event_type = XCWM_EVENT_WINDOW_CREATE;
                callback_ptr(return_evt);
                break;
            }

            case XCB_UNMAP_NOTIFY:
            {
                xcb_unmap_notify_event_t *notify =
                    (xcb_unmap_notify_event_t *)evt;
                
                xcwm_window_t *window =
                    _xcwm_window_remove(event_conn, notify->window);

                if (!window) {
                    /* Not a window in the list, don't try and destroy */
                    break;
                }

                return_evt = malloc(sizeof(xcwm_event_t));
                return_evt->event_type = XCWM_EVENT_WINDOW_DESTROY;
                return_evt->window = window;

                callback_ptr(return_evt);

                _xcwm_window_composite_pixmap_release(window);

                // Release memory for the window
                _xcwm_window_release(window);
                break;
            }

            case XCB_CONFIGURE_NOTIFY:
            {
                xcb_configure_notify_event_t *request =
                    (xcb_configure_notify_event_t *)evt;
                xcwm_window_t *window =
                    _xcwm_get_window_node_by_window_id(request->window);
                if (window)
                    _xcwm_window_composite_pixmap_update(window);
                break;
            }

            case XCB_CONFIGURE_REQUEST:
            {
                xcb_configure_request_event_t *request =
                    (xcb_configure_request_event_t *)evt;
                xcwm_window_t *window =
                    _xcwm_get_window_node_by_window_id(request->window);
                if (!window) {
                    /* Passing on requests for windows we aren't
                     * managing seems to speed window future mapping
                     * of window */
                    _xcwm_resize_window(event_conn, request->window,
                                        request->x, request->y,
                                        request->width, request->height);
                }
                break;
            }

            case XCB_PROPERTY_NOTIFY:
            {
                xcb_property_notify_event_t *notify =
                    (xcb_property_notify_event_t *)evt;
                xcwm_window_t *window =
                    _xcwm_get_window_node_by_window_id(notify->window);
                if (!window) {
                    break;
                }

                /* If this is WM_PROTOCOLS, do not send event, just
                 * handle internally */
                if (notify->atom == window->context->atoms->ewmh_conn.WM_PROTOCOLS) {
                    _xcwm_atoms_set_wm_delete(window);
                    break;
                }

                /* Change to window name */
                if (notify->atom == window->context->atoms->ewmh_conn._NET_WM_NAME
                    || notify->atom == window->context->atoms->wm_name_atom) {
                    _xcwm_atoms_set_window_name(window);
                    return_evt = malloc(sizeof(xcwm_event_t));
                    return_evt->event_type = XCWM_EVENT_WINDOW_NAME;
                    return_evt->window = window;
                    
                    callback_ptr(return_evt);
                    break;
                }

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
                /* xcb_button_press_event_t *bp = */
                /*     (xcb_button_press_event_t *)evt; */
                /* printf ("mouse motion in window %u, at coordinates (%d,%d)\n", */
                /*        bp->event, bp->event_x, bp->event_y ); */
                break;
            }

            case XCB_MAPPING_NOTIFY:
                break;

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

xcwm_event_type_t
xcwm_event_get_type(xcwm_event_t const *event)
{
    return event->event_type;
}

xcwm_window_t *
xcwm_event_get_window(xcwm_event_t const *event)
{
    return event->window;
}
