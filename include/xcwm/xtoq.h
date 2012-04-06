/* Copyright (c) 2012 David Snyder
 *
 * xtoq.h
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

/* FIXME: This header needs to go away.
 *        Each subsystem should have its own header file.  structs
 *        should not be API since that makes changes hard in the future.
 *        See event.h for an example.
 */

#ifndef _XTOQ_H_
#define _XTOQ_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_aux.h>
#include <xcb/damage.h>
#include <xcb/composite.h>
#include <xcb/xtest.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_keysyms.h>

/* Abstract types for xcwm data types */
struct xcwm_window_t;
typedef struct xcwm_window_t xcwm_window_t;

struct xcwm_context_t {
    xcb_connection_t *conn;
    xcwm_window_t *root_window;
};
typedef struct xcwm_context_t xcwm_context_t;

struct xcwm_window_t {
    xcb_drawable_t window_id;
    xcwm_context_t *context;
    struct xcwm_window_t *parent;
    xcb_damage_damage_t damage;
    int x;
    int y;
    int width;
    int height;
    int damaged_x;
    int damaged_y;
    int damaged_width;
    int damaged_height;
    char *name;         /* The name of the window */
    int wm_delete_set;  /* Flag for WM_DELETE_WINDOW, 1 if set */
    void *local_data;   /* Area for data client cares about */
};

struct image_data_t {
    uint8_t *data;
    int length;
};
typedef struct image_data_t image_data_t;

struct xcwm_image_t {
    xcb_image_t *image;
    int x;
    int y;
    int width;
    int height;
};
typedef struct xcwm_image_t xcwm_image_t;

/**
 * Sets up the connection and grabs the root window from the specified screen
 * @param display the display to connect to
 * @return The root context which contains the root window
 */
xcwm_context_t *
xcwm_init(char *display);

/**
 * Returns a window's entire image
 * @param an xcwm_context_t
 * @param window The window to get image from.
 * @return an xcwm_image_t with an the image of a window
 */
xcwm_image_t *
xcwm_get_image(xcwm_context_t *context, xcwm_window_t *window);

/**
 * Intended for servicing to a client's reaction to a damage notification
 * this window returns the modified subrectangle of a window
 * @param window The window to get image from
 * @return an xcwm_image_t with partial image window contents
 */
xcwm_image_t *
test_xcwm_get_image(xcwm_window_t *window);

/**
 * free the memory used by an xcwm_image_t created
 * during a call to test_xcwm_image_create
 * @param xcwm_image an image to be freed
 */
void
xcwm_image_destroy(xcwm_image_t * xcwm_image);

/**
 * Set input focus to the window in context
 * @param window The window to set focus to
 */
void
xcwm_set_input_focus(xcwm_window_t *window);

/**
 * Set a window to the bottom of the window stack.
 * @param window The window to move to bottom
 */
void
xcwm_set_window_to_bottom(xcwm_window_t *window);
/**
 * Set a window to the top of the window stack.
 * @param window The window to move.
 */
void
xcwm_set_window_to_top(xcwm_window_t *window);

/**
 * Remove the damage from a given window.
 * @param window The window to remove damage from
 */
void
xcwm_remove_window_damage(xcwm_window_t *window);

/**
 * Closes the windows open on the X Server, the connection, and the event
 * loop.
 * @param context The context to close.
 */
void
xcwm_close(xcwm_context_t *context);

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

/****************
* window.c
****************/

/**
 * kill the window, if possible using WM_DELETE_WINDOW (icccm)
 * otherwise using xcb_kill_client.
 * @param context The context of the window
 * @param window The window to be closed.
 */
void
xcwm_request_close(xcwm_context_t *context, xcwm_window_t *window);

/**
 * move and/or resize the window, update the context
 * @param context the context of the window to configure
 * @param window The window to configure
 * @param x The new x coordinate
 * @param y The new y coordinate
 * @param height The new height
 * @param width The new width
 */
void
xcwm_configure_window(xcwm_context_t *context, xcwm_window_t *window, int x,
                      int y, int height,
                      int width);

#endif // _XTOQ_H_
