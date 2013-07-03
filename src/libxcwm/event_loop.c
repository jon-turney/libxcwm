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
#include <sys/shm.h>

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
    /* Release composite pixmap */
    if (window->composite_pixmap_id) {
      xcb_free_pixmap(window->context->conn, window->composite_pixmap_id);
      window->composite_pixmap_id = 0;
    }

    /* Release SHM resources */
    if (window->context->has_shm) {
        if (window->shminfo.shmseg) {
            xcb_shm_detach(window->context->conn, window->shminfo.shmseg);
            window->shminfo.shmseg = 0;
        }

        if (window->shminfo.shmaddr != (void *)-1) {
            shmdt(window->shminfo.shmaddr);
            window->shminfo.shmaddr = (void *)-1;
        }

        if (window->shminfo.shmid != -1) {
            shmctl(window->shminfo.shmid, IPC_RMID, 0);
            window->shminfo.shmid = -1;
        }
    }
}

static void
_xcwm_window_composite_pixmap_update(xcwm_window_t *window)
{
    _xcwm_window_composite_pixmap_release(window);

    /* Assign a pixmap XID to the composite pixmap */
    window->composite_pixmap_id = xcb_generate_id(window->context->conn);
    xcb_composite_name_window_pixmap(window->context->conn, window->window_id, window->composite_pixmap_id);

    if (window->context->has_shm)
    {
        /* Pre-compute the size of image data needed for this window */
        xcb_image_t *image = xcb_image_create_native(window->context->conn,
                                                 window->bounds.width,
                                                 window->bounds.height,
                                                 XCB_IMAGE_FORMAT_Z_PIXMAP,
                                                 window->context->depth,
                                                 NULL, ~0, NULL);
        size_t image_size = image->size;
        xcb_image_destroy(image);
        printf("window 0x%08x requires %d bytes of SHM\n", window->window_id, image_size);

        /* Allocate SHM resources */
        window->shminfo.shmid = shmget(IPC_PRIVATE, image_size, IPC_CREAT | 0777);
        if (window->shminfo.shmid < 0)
            fprintf(stderr, "shmget failed\n");

        window->shminfo.shmaddr = shmat(window->shminfo.shmid, 0, 0);
        if (window->shminfo.shmaddr < 0)
            fprintf(stderr, "shmget failed\n");

        window->shminfo.shmseg = xcb_generate_id(window->context->conn);
        xcb_shm_attach(window->context->conn, window->shminfo.shmseg, window->shminfo.shmid, 0);
    }
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

static char *
_xcwm_get_atom_name(xcb_connection_t *conn, xcb_atom_t atom)
{
    char *name = NULL;

    xcb_get_atom_name_cookie_t cookie = xcb_get_atom_name(conn, atom);
    xcb_get_atom_name_reply_t *reply = xcb_get_atom_name_reply(conn, cookie, NULL);

    if (reply) {
        int length = xcb_get_atom_name_name_length(reply);
        name = malloc(length+1);
        strncpy(name, xcb_get_atom_name_name(reply), length);
        name[length] = '\0';
        free(reply);
    }
    else {
        name = strdup("unknown");
    }

    return name;
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
#define EVENT_DEBUG (context->flags & XCWM_VERBOSE_LOG_XEVENTS)

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


            if (EVENT_DEBUG) {
            printf("DAMAGE_NOTIFY: XID 0x%08x %d,%d @ %d,%d\n",
                   dmgevnt->drawable,
                   dmgevnt->area.width, dmgevnt->area.height, dmgevnt->area.x, dmgevnt->area.y);
            }

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

            if (EVENT_DEBUG) {
                printf("SHAPE_NOTIFY: XID 0x%08x\n", shapeevnt->affected_window);
            }

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

            if (EVENT_DEBUG) {
                printf("CURSOR_NOTIFY:\n");
            }

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
                fprintf(stderr, "ID: 0x%08x\n"
                        "Major opcode: %i\n"
                        "Minor opcode: %i\n",
                        err->resource_id,
                        err->major_code,
                        err->minor_code);

                break;
            }

            case XCB_CREATE_NOTIFY:
            {
                xcb_create_notify_event_t *notify =
                    (xcb_create_notify_event_t *)evt;

                /* We don't actually allow our client to create its
                 * window here, wait until the XCB_MAP_REQUEST */

                if (EVENT_DEBUG) {
                    printf("CREATE_NOTIFY: XID 0x%08x\n", notify->window);
                }

                break;
            }

            case XCB_DESTROY_NOTIFY:
            {
                // Window destroyed in root window
                xcb_destroy_notify_event_t *notify =
                    (xcb_destroy_notify_event_t *)evt;

                if (EVENT_DEBUG) {
                    printf("DESTROY_NOTIFY: XID 0x%08x\n", notify->window);
                }

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

            case XCB_MAP_NOTIFY:
            {
                xcb_map_notify_event_t *notify =
                    (xcb_map_notify_event_t *)evt;

                if (EVENT_DEBUG) {
                    printf("MAP_NOTIFY: XID 0x%08x\n", notify->window);
                }

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

                if (EVENT_DEBUG) {
                    printf("MAP_REQEUST: XID 0x%08x\n", request->window);
                }

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

                if (EVENT_DEBUG) {
                    printf("UNMAP_NOTIFY: XID 0x%08x\n", notify->window);
                }

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

                if (EVENT_DEBUG) {
                    printf("CONFIGURE_NOTIFY: XID 0x%08x %dx%d @ %d,%d\n",
                           request->window, request->width, request->height,
                           request->x, request->y);
                }

                xcwm_window_t *window =
                    _xcwm_get_window_node_by_window_id(request->window);

                if (!window)
                    break;

		int resized = 0;
                int moved = 0;

		if ((request->width != window->notified_bounds.width) ||
                    (request->height != window->notified_bounds.height))
                    resized = 1;

		if ((request->x != window->notified_bounds.x) ||
                    (request->y != window->notified_bounds.y))
                    moved = 1;

                /*
                   Update the window bounds.  This is the only place that should
                   update them, so we can accurately tell when a window has been moved or resized.
                */
                window->notified_bounds.x = request->x;
                window->notified_bounds.y = request->y;
                window->notified_bounds.width = request->width;
                window->notified_bounds.height = request->height;

                /*
                  We don't receive CONFIGURE_REQUEST for override-redirect windows, so note
                  the actual window bounds here
                */
                if (window->override_redirect) {
                    window->bounds.x = request->x;
                    window->bounds.y = request->y;
                    window->bounds.width = request->width;
                    window->bounds.height = request->height;
                }

                /* if window was resized, we must reallocate composite pixmap */
                if (resized)
                    _xcwm_window_composite_pixmap_update(window);

                /* if window was moved or resized, send configure event */
                if (moved || resized) {
                    return_evt.event_type = XCWM_EVENT_WINDOW_CONFIGURE;
                    return_evt.window = window;
                    callback_ptr(&return_evt);
                }

                break;
            }

            case XCB_CONFIGURE_REQUEST:
            {
                xcb_configure_request_event_t *request =
                    (xcb_configure_request_event_t *)evt;

                if (EVENT_DEBUG) {
                    printf("CONFIGURE_REQUEST: XID 0x%08x %dx%d @ %d,%d mask 0x%04x\n",
                           request->window, request->width, request->height,
                           request->x, request->y, request->value_mask);
                }

                xcwm_window_t *window =
                    _xcwm_get_window_node_by_window_id(request->window);
                if (!window) {
                    break;
                }

                if (request->value_mask &
                    (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT))
                    xcwm_window_configure(window,
                                          request->x, request->y,
                                          request->width, request->height);
                /* XXX: requests to change stacking are currently ignored */

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

                if (EVENT_DEBUG) {
                    char *name = _xcwm_get_atom_name(event_conn, notify->atom);
                    printf("PROPERTY_NOTIFY: XID 0x%08x, property atom '%s' (%d)\n",
                           notify->window, name, notify->atom);
                    free(name);
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
                    if (EVENT_DEBUG) {
                        printf("PROPERTY_NOTIFY: no registered event, ignored\n");
                    }

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
                if (EVENT_DEBUG) {
                    printf("UNKNOWN EVENT: %i\n", (evt->response_type & ~0x80));
                }

                break;
            }
            }
        }
        /* Free the event */
        free(evt);
    }

    /* xcb_wait_for_event() returns NULL for I/O error */
    printf("xcb_wait_for_event() failed, exiting event loop\n");

    _event_thread = 0;

    /* send event indicating we should shutdown */
    return_evt.event_type = XCWM_EVENT_EXIT;
    return_evt.window = 0;
    callback_ptr(&return_evt);

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
