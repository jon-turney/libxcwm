/* Copyright (c) 2012 David Snyder, Benjamin Carr, Braden Wooley
 *
 * rootimg_api.h
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

#include <xcwm/xcwm.h>
#include "xcwm_internal.h"
#include "X11/keysym.h"
#include <string.h>

// aaron key stuff
#define XK_Shift_L 0xffe1
xcb_key_symbols_t *syms = NULL;

// This init function needs set the window to be registered for events!
// First one we should handle is damage
xcwm_context_t *
xcwm_init(char *display)
{

    xcwm_context_t *root_context;
    xcb_connection_t *conn;
    int conn_screen;
    xcb_screen_t *root_screen;
    xcb_window_t root_window_id;
    xcb_void_cookie_t cookie;
    uint32_t mask_values[1];

    conn = xcb_connect(display, &conn_screen);

    root_screen = xcb_aux_get_screen(conn, conn_screen);
    root_window_id = root_screen->root;

    // Set the mask for the root window so we know when new windows
    // are created on the root. This is where we add masks for the events
    // we care about catching on the root window.
    mask_values[0] = XCB_EVENT_MASK_KEY_PRESS |
                     XCB_EVENT_MASK_KEY_RELEASE |
                     XCB_EVENT_MASK_BUTTON_PRESS |
                     XCB_EVENT_MASK_BUTTON_RELEASE |
                     XCB_EVENT_MASK_POINTER_MOTION |
                     XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                     XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
                     XCB_EVENT_MASK_ENTER_WINDOW |
                     XCB_EVENT_MASK_LEAVE_WINDOW |
                     XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
    cookie = xcb_change_window_attributes_checked(conn,
                                                  root_window_id,
                                                  XCB_CW_EVENT_MASK,
                                                  mask_values);
    if (_xcwm_request_check(conn, cookie,
                            "Could not set root window mask.")) {
        fprintf(stderr, "Is another window manager running?\n");
        xcb_disconnect(conn);
        exit(1);
    }

    xcb_flush(conn);

    root_context = malloc(sizeof(xcwm_context_t));
    assert(root_context);
    root_context->root_window = malloc(sizeof(xcwm_window_t));
    assert(root_context);

    root_context->conn = conn;
    root_context->root_window->parent = 0;
    root_context->root_window->window_id = root_window_id;

    // Set width, height, x, & y from root_screen into the xcwm_context_t
    root_context->root_window->width = root_screen->width_in_pixels;
    root_context->root_window->height = root_screen->height_in_pixels;
    root_context->root_window->x = 0;
    root_context->root_window->y = 0;

    _xcwm_init_composite(root_context);

    _xcwm_init_damage(root_context);

    _xcwm_init_xfixes(root_context);

    /* Add the root window to our list of windows being managed */
    _xcwm_add_window(root_context->root_window);

    syms = xcb_key_symbols_alloc(conn);
    _xcwm_init_extension(conn, "XTEST");
    _xcwm_init_extension(conn, "XKEYBOARD");

    _xcwm_get_wm_atoms(root_context);

    return root_context;
}

xcwm_image_t *
xcwm_get_image(xcwm_context_t *context, xcwm_window_t *window)
{

    xcb_get_geometry_reply_t *geom_reply;

    xcb_image_t *image;

    geom_reply = _xcwm_get_window_geometry(context->conn, window->window_id);

    //FIXME - right size
    xcwm_image_t * xcwm_image =
        (xcwm_image_t *)malloc(10 * sizeof(xcwm_image_t));

    xcb_flush(context->conn);
    /* Get the full image of the window */
    image = xcb_image_get(context->conn,
                          window->window_id,
                          geom_reply->x,
                          geom_reply->y,
                          geom_reply->width,
                          geom_reply->height,
                          (unsigned int)~0L,
                          XCB_IMAGE_FORMAT_Z_PIXMAP);

    xcwm_image->image = image;
    xcwm_image->x = geom_reply->x;
    xcwm_image->y = geom_reply->y;
    xcwm_image->width = geom_reply->width;
    xcwm_image->height = geom_reply->height;

    free(geom_reply);

    return xcwm_image;
}

int
xcwm_start_event_loop(xcwm_context_t *context,
                      xcwm_event_cb_t callback)
{
    /* Simply call our internal function to do the actual setup */
    return _xcwm_start_event_loop(context->conn, callback);
}

xcwm_image_t *
test_xcwm_get_image(xcwm_context_t *context, xcwm_window_t *window)
{

    xcb_image_t *image;

    xcb_flush(context->conn);
    /* Get the image of the root window */
    image = xcb_image_get(context->conn,
                          window->window_id,
                          window->damaged_x,
                          window->damaged_y,
                          window->damaged_width,
                          window->damaged_height,
                          (unsigned int)~0L,
                          XCB_IMAGE_FORMAT_Z_PIXMAP);

    //FIXME - Calculate memory size correctly
    xcwm_image_t * xcwm_image =
        (xcwm_image_t *)malloc(10 * sizeof(xcwm_image_t));

    xcwm_image->image = image;
    xcwm_image->x = window->damaged_x;
    xcwm_image->y = window->damaged_y;
    xcwm_image->width = window->damaged_width;
    xcwm_image->height = window->damaged_height;

    return xcwm_image;
}

void
xcwm_image_destroy(xcwm_image_t * xcwm_image)
{

    xcb_image_destroy(xcwm_image->image);
    free(xcwm_image);
}

void
xcwm_remove_window_damage(xcwm_context_t *context, xcwm_window_t *window)
{
    xcb_xfixes_region_t region = xcb_generate_id(context->conn);
    xcb_rectangle_t rect;
    xcb_void_cookie_t cookie;

    if (!window) {
        return;
    }

    rect.x = window->damaged_x;
    rect.y = window->damaged_y;
    rect.width = window->damaged_width;
    rect.height = window->damaged_height;

    xcb_xfixes_create_region(context->conn,
                             region,
                             1,
                             &rect);

    cookie = xcb_damage_subtract_checked(context->conn,
                                         window->damage,
                                         region,
                                         0);

    if (!(_xcwm_request_check(context->conn, cookie,
                              "Failed to subtract damage"))) {
        window->damaged_x = 0;
        window->damaged_y = 0;
        window->damaged_width = 0;
        window->damaged_height = 0;
    }
    return;
}

/* Close all windows, the connection, as well as the event loop */
void
xcwm_close(xcwm_context_t *context)
{

    _xcwm_window_node *head = _xcwm_window_list_head;

    xcb_flush(context->conn);

    // Close all windows
    while (head) {
        xcwm_request_close(context, head->window);
        _xcwm_window_list_head = head->next;
        free(head);
        head = _xcwm_window_list_head;
    }

    // Terminate the event loop
    if (_xcwm_stop_event_loop() != 1) {
        printf("Event loop failed to close\n");
    }

    // Disconnect from the display
    xcb_disconnect(context->conn);

    return;

}
