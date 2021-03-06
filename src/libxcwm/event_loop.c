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

            xcwm_event_t return_evt;
            return_evt.window = window;
            return_evt.event_type = XCWM_EVENT_WINDOW_CREATE;

            callback_ptr(&return_evt);
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
    xcwm_event_t return_evt;
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
        uint8_t response_type = evt->response_type  & ~0x80;
        if (response_type == context->damage_event_mask) {
            xcb_damage_notify_event_t *dmgevnt =
                (xcb_damage_notify_event_t *)evt;

            /* printf("damage %d,%d @ %d,%d reported against window 0x%08x\n", */
            /*        dmgevnt->area.width, dmgevnt->area.height, dmgevnt->area.x, dmgevnt->area.y, */
            /*        dmgevnt->drawable); */

            xcwm_window_t *window = _xcwm_get_window_node_by_window_id(dmgevnt->drawable);

            return_evt.event_type = XCWM_EVENT_WINDOW_DAMAGE;
            return_evt.window = window;

            if (!window) {
                printf("damage reported against unknown window 0x%08x\n", dmgevnt->drawable);
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
            if (window->initial_damage == 1
                || (dmgevnt->area.width > window->bounds.width)
                || (dmgevnt->area.height > window->bounds.height) ) {
                xcb_xfixes_region_t region =
                    xcb_generate_id(window->context->conn);
                xcb_rectangle_t rect;

                /* printf("initial damage on window 0x%08x\n", dmgevnt->drawable); */

                /* Remove the damage */
                xcb_xfixes_create_region(window->context->conn,
                                         region,
                                         1,
                                         &dmgevnt->area);
                xcb_damage_subtract(window->context->conn,
                                    window->damage,
                                    region,
                                    XCB_NONE);

                /* Add new damage area for entire window */
                rect.x = 0;
                rect.y = 0;
                rect.width = window->bounds.width;
                rect.height = window->bounds.height;
                xcb_xfixes_set_region(window->context->conn,
                                      region,
                                      1,
                                      &rect);
                xcb_damage_add(window->context->conn,
                               window->window_id,
                               region);

                window->initial_damage = 0;
                xcb_xfixes_destroy_region(window->context->conn,
                                          region);
                xcwm_event_release_thread_lock();
                continue;
            }

            window->dmg_bounds.x = dmgevnt->area.x;
            window->dmg_bounds.y = dmgevnt->area.y;
            window->dmg_bounds.width = dmgevnt->area.width;
            window->dmg_bounds.height = dmgevnt->area.height;

            xcwm_event_release_thread_lock();

            callback_ptr(&return_evt);

        }
        else if (response_type == context->shape_event) {
            xcb_shape_notify_event_t *shapeevnt =
                (xcb_shape_notify_event_t *)evt;

            if (shapeevnt->shape_kind == XCB_SHAPE_SK_BOUNDING) {
                xcwm_window_t *window = _xcwm_get_window_node_by_window_id(shapeevnt->affected_window);
                _xcwm_window_set_shape(window, shapeevnt->shaped);

                return_evt.event_type = XCWM_EVENT_WINDOW_SHAPE;
                return_evt.window = window;
                callback_ptr(&return_evt);
            }
        }
        else if (response_type == context->fixes_event_base + XCB_XFIXES_CURSOR_NOTIFY) {
            /* xcb_xfixes_cursor_notify_event_t *cursorevnt = */
            /*     (xcb_xfixes_cursor_notify_event_t *)evt; */

            return_evt.event_type = XCWM_EVENT_CURSOR;
            return_evt.window = NULL;
            callback_ptr(&return_evt);
        }
        else {
            switch (response_type) {
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

                return_evt.event_type = XCWM_EVENT_WINDOW_EXPOSE;
                callback_ptr(&return_evt);
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

                return_evt.event_type = XCWM_EVENT_WINDOW_DESTROY;
                return_evt.window = window;

                callback_ptr(&return_evt);

                // Release memory for the window
                _xcwm_window_release(window);
                break;
            }

            case XCB_MAP_NOTIFY:
            {
                xcb_map_notify_event_t *notify =
                    (xcb_map_notify_event_t *)evt;

                /* notify->event holds parent of the window */

                xcwm_window_t *window =
                    _xcwm_get_window_node_by_window_id(notify->window);
                if (!window)
                {
                    /*
                      No MAP_REQUEST for override-redirect windows, so
                      need to create the xcwm_window_t for it now
                    */
                    /* printf("MAP_NOTIFY without MAP_REQUEST\n"); */
                    window =
                        _xcwm_window_create(context, notify->window,
                                            notify->event);

                    if (window)
                    {
                        _xcwm_window_composite_pixmap_update(window);

                        return_evt.window = window;
                        return_evt.event_type = XCWM_EVENT_WINDOW_CREATE;
                        callback_ptr(&return_evt);
                    }
                }
                else
                {
                    _xcwm_window_composite_pixmap_update(window);
                }

                break;
            }

            case XCB_MAP_REQUEST:
            {
                xcb_map_request_event_t *request =
                    (xcb_map_request_event_t *)evt;

                /* Map the window */
                xcb_map_window(context->conn, request->window);
                xcb_flush(context->conn);

                return_evt.window =
                    _xcwm_window_create(context, request->window,
                                        request->parent);
                if (!return_evt.window) {
                    break;
                }

                return_evt.event_type = XCWM_EVENT_WINDOW_CREATE;
                callback_ptr(&return_evt);
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

                return_evt.event_type = XCWM_EVENT_WINDOW_DESTROY;
                return_evt.window = window;

                callback_ptr(&return_evt);

                _xcwm_window_composite_pixmap_release(window);

                // Release memory for the window
                _xcwm_window_release(window);
                break;
            }

            case XCB_CONFIGURE_NOTIFY:
            {
                xcb_configure_notify_event_t *request =
                    (xcb_configure_notify_event_t *)evt;

                printf("CONFIGURE_NOTIFY: XID 0x%08x %dx%d @ %d,%d\n",
                       request->window, request->width, request->height,
                       request->x, request->y);

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

                printf("CONFIGURE_REQUEST: XID 0x%08x %dx%d @ %d,%d mask 0x%04x\n",
                       request->window, request->width, request->height,
                       request->x, request->y, request->value_mask);

                /*
                   relying on the server's idea of the current values of values not
                   in value_mask is a bad idea, we might have a configure request of
                   our own on this window in flight
                */
                if (request->value_mask &
                    (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT))
                    _xcwm_resize_window(event_conn, request->window,
                                        request->x, request->y,
                                        request->width, request->height);

                /* Ignore requests to change stacking ? */

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
                if (notify->atom == window->context->atoms.ewmh_conn.WM_PROTOCOLS) {
                    _xcwm_atoms_set_wm_delete(window);
                    break;
                }

                xcwm_event_type_t event;
                if (_xcwm_atom_change_to_event(notify->atom, window, &event))
                {
                    /* Send the appropriate event */
                    return_evt.event_type = event;
                    return_evt.window = window;
                    callback_ptr(&return_evt);
                }
                else {
                    printf("PROPERTY_NOTIFY for ignored property atom %d\n", notify->atom);
                    /*
                      We need a mechanism to forward properties we don't know about to WM,
                      otherwise everything needs to be in libXcwm ...?
                    */
                }

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
