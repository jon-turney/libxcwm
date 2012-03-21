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

/* FIXME: Obfuscate these */
struct xcwm_context_t {
    xcb_connection_t *conn;
    xcb_drawable_t window;
    xcb_window_t parent;
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
typedef struct xcwm_context_t xcwm_context_t;

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
 * Context which contains the display's root window.
 *
 * FIXME: We should avoid having global state where at all possible, and
 *        certainly not export such state for client access
 */
extern xcwm_context_t *root_context;

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
 * FIXME: this might be for the root window
 * @return an xcwm_image_t with an the image of a window
 */
xcwm_image_t *
xcwm_get_image(xcwm_context_t *context);

/**
 * Intended for servicing to a client's reaction to a damage notification
 * this window returns the modified subrectangle of a window
 * @param an xcwm_context_t of the damaged window
 * @return an xcwm_image_t with partial image window contents
 */
xcwm_image_t *
test_xcwm_get_image(xcwm_context_t * context);


/**
 * free the memory used by an xcwm_image_t created 
 * during a call to test_xcwm_image_create
 * @param xcwm_image an image to be freed
 */
void 
xcwm_image_destroy(xcwm_image_t * xcwm_image);

/**
 * Set input focus to the window in context
 * @param context The context containing the window
 */
void
xcwm_set_input_focus(xcwm_context_t *context);

/**
 * Set a window to the bottom of the window stack.
 * @param context The context containing the window
 */
void
xcwm_set_window_to_bottom(xcwm_context_t *context);

/**
 * Set a window to the top of the window stack.
 * @param context The context containing the window
 */
void
xcwm_set_window_to_top(xcwm_context_t *context);

/**
 * Remove the damage from the given context.
 * @param context The context to remove the damage from
 */
void
xcwm_remove_context_damage(xcwm_context_t *context);

/**
 * Closes the windows open on the X Server, the connection, and the event
 * loop. 
 */
void 
xcwm_close(void);

/**
 * function
 * @param context xcwm_context_t 
 * @param window The window that the key press was made in.
 * @param keyCode The key pressed.
 */
void
xcwm_key_press (xcwm_context_t *context, int window, uint8_t code);

/**
 * function
 * @param context xcwm_context_t 
 * @param window The window that the key press was made in.
 * @param keyCode The key released.
 */
void
xcwm_key_release (xcwm_context_t *context, int window, uint8_t code);

/**
 * Uses the XTEST protocol to send input events to the X Server (The X Server
 * is usually in the position of sending input events to a client). The client
 * will often choose to send coordinates through mouse motion and set the params 
 * x & y to 0 here.
 * @param context xcwm_context_t 
 * @param x - x coordinate
 * @param y - y coordinate
 * @param window The window that the key press was made in.
 */
void
xcwm_button_press (xcwm_context_t *context, long x, long y, int window, int button);

/**
 * Uses the XTEST protocol to send input events to the X Server (The X Server
 * is usually in the position of sending input events to a client). The client
 * will often choose to send coordinates through mouse motion and set the params 
 * x & y to 0 here.
 * @param context xcwm_context_t 
 * @param x - x coordinate
 * @param y - y coordinate
 * @param window The window that the key release was made in.
 */
void
xcwm_button_release (xcwm_context_t *context, long x, long y, int window, int button);

/**
 * function
 * @param context xcwm_context_t 
 * @param x - x coordinate
 * @param y - y coordinate
 * @param window The window that the key release was made in.
 */
void
xcwm_mouse_motion (xcwm_context_t *context, long x, long y, int window, int button);

/****************
 * window.c
 ****************/

/**
 * kill the window, if possible using WM_DELETE_WINDOW (icccm) 
 * otherwise using xcb_kill_client.
 * @param context The context of the window to be killed
 */
void
xcwm_request_close(xcwm_context_t *context);

/**
 * move and/or resize the window, update the context 
 * @param context the context of the window to configure
 * @param x The new x coordinate
 * @param y The new y coordinate
 * @param height The new height
 * @param width The new width
 */
void
xcwm_configure_window(xcwm_context_t *context, int x, int y, int height, int width);

#endif // _XTOQ_H_
