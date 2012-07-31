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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
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
xcwm_context_open(char *display)
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
    assert(root_context->root_window);
    root_context->root_window->bounds = malloc(sizeof(xcwm_rect_t));
    root_context->root_window->dmg_bounds = malloc(sizeof(xcwm_rect_t));
    assert(root_context->root_window->bounds);
    assert(root_context->root_window->dmg_bounds);
    root_context->atoms = malloc(sizeof(xcwm_wm_atoms_t));
    assert(root_context->atoms);

    root_context->conn = conn;
    root_context->conn_screen = conn_screen;
    root_context->root_window->parent = 0;
    root_context->root_window->window_id = root_window_id;
    /* FIXME: Should we have a circular assignment like this? */
    root_context->root_window->context = root_context;

    /* Set width, height, x, & y from root_screen into the xcwm_context_t */
    root_context->root_window->bounds->width = root_screen->width_in_pixels;
    root_context->root_window->bounds->height = root_screen->height_in_pixels;
    root_context->root_window->bounds->x = 0;
    root_context->root_window->bounds->y = 0;

    _xcwm_init_composite(root_context);

    _xcwm_init_damage(root_context);

    _xcwm_init_xfixes(root_context);

    /* Add the root window to our list of windows being managed */
    _xcwm_add_window(root_context->root_window);

    syms = xcb_key_symbols_alloc(conn);
    _xcwm_init_extension(conn, "XTEST");
    _xcwm_init_extension(conn, "XKEYBOARD");

    _xcwm_atoms_init(root_context);

    return root_context;
}
/* Close all windows, the connection, as well as the event loop */
void
xcwm_context_close(xcwm_context_t *context)
{

    _xcwm_window_node *head = _xcwm_window_list_head;

    xcb_flush(context->conn);

    // Close all windows
    while (head) {
        xcwm_window_request_close(head->window);
        _xcwm_window_list_head = head->next;
        free(head);
        head = _xcwm_window_list_head;
    }

    /* Free atom related stuff */
    _xcwm_atoms_release(context);
    free(context->atoms);

    // Terminate the event loop
    if (_xcwm_event_stop_loop() != 1) {
        printf("Event loop failed to close\n");
    }

    // Disconnect from the display
    xcb_disconnect(context->conn);

    return;
}

xcwm_window_t *
xcwm_context_get_root_window(xcwm_context_t const *context)
{

    return context->root_window;
}
