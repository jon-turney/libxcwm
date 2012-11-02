/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>
 * All rights reserved.
 *
 * xcwm_internal.h
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

#ifndef _XCWM_INTERNAL_H_
#define _XCWM_INTERNAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_atom.h>
#include <xcwm/xcwm.h>

/**
 * Strucuture used to pass nesessary data to xcwm_start_event_loop.
 */
typedef struct xcwm_event_connetion {
    xcb_connection_t *conn;               /* Connection to listen to events on */
    xcwm_event_cb_t event_callback;       /* Fuction to call when event caught */
} xcwm_event_connection;

/**
 * Structure to hold WM_* atoms that we care about
 */
struct xcwm_wm_atoms_t {
    xcb_atom_t wm_delete_window_atom;
    xcb_atom_t wm_transient_for_atom;
    xcb_atom_t wm_name_atom;
    xcb_atom_t wm_state_atom;
    xcb_ewmh_connection_t ewmh_conn;
};

/**
 * Local data type for image data.
 */
typedef struct image_data_t image_data_t;

/**
 * Mutex lock supplied to client to lock event loop thread
 */
extern pthread_mutex_t event_thread_lock;


/* util.c */

/**
 * Return the given windows attributes reply. Caller must free memory
 * allocated for reply.
 * @param conn The windows connection.
 * @param window The window.
 * @return The window attributes reply. Null if the request fails.
 */
xcb_get_window_attributes_reply_t *
_xcwm_get_window_attributes(xcb_connection_t *conn, xcb_window_t window);

/**
 * Return the geometry of the window in a geometry reply. Caller must free
 * memory allocated for reply.
 * @param conn The windows connection.
 * @param window The window.
 * @return The window's geometry reply. Null if the request for reply fails.
 */
xcb_get_geometry_reply_t *
_xcwm_get_window_geometry(xcb_connection_t *conn, xcb_window_t window);

/**
 * Print out information about the existing windows attached to our
 * root. Most of this code is taken from src/manage.c from the i3 code
 * by Michael Stapelberg
 * @param conn a connection the the xserver.
 * @param window the window.
 * @return the geometry of the window
 */
void
_xcwm_write_all_children_window_info(xcb_connection_t *conn,
                                     xcb_window_t root);

/**
 * Get the image data for a window.
 * @param conn the connection to the xserver.
 * @param root the window acenstral to all children to be written.
 * @return a structure containing data and data length.
 */
image_data_t
_xcwm_get_window_image_data(xcb_connection_t *conn, xcb_window_t window);

/**
 * Write information about a window out to stdio.
 * TODO: Add the ability to pass in the stream to write to.
 * @param conn The connection with the window.
 * @param window The window.
 */
void
_xcwm_write_window_info(xcb_connection_t *conn, xcb_window_t window);

/**
 * Check the request cookie and determine if there is an error.
 * @param conn The connection the request was sent on.
 * @param cookie The cookie returned by the request.
 * @param msg the string to display if there is an error with the request.
 * @return int The number of the error code, if any. Otherwise zero.
 */
int
_xcwm_request_check(xcb_connection_t *conn, xcb_void_cookie_t cookie,
                    char *msg);

/****************
* init.c
****************/

/**
 * Initializes an extension on the xserver.
 * @param conn The connection to the xserver.
 * @param extension_name The string specifying the name of the extension.
 * @return The reply structure
 */
xcb_query_extension_reply_t *
_xcwm_init_extension(xcb_connection_t *conn, char *extension_name);

/**
 * Initializes the damage extension.
 * @param contxt  The context containing.
 */
void
_xcwm_init_damage(xcwm_context_t *contxt);

/**
 * Initializes the composite extension on the context containg
 * the root window.
 * @param contxt The contxt containing the root window
 */
void
_xcwm_init_composite(xcwm_context_t *contxt);

/**
 * Initialize the xfixes extension.
 * @param contxt The context
 */
void
_xcwm_init_xfixes(xcwm_context_t *contxt);

/****************
* event_loop.c
****************/

/**
 * Stops the thread running the event loop.
 * @return 0 on success, otherwise zero.
 */
int
_xcwm_event_stop_loop(void);

/****************
* context_list.c
****************/

/**
 * A structure (doubly linked list) to hold
 * pointers to the contexts
 */
typedef struct _xcwm_window_node {
    struct xcwm_window_t *window;   /**< Pointer to a window */
    struct _xcwm_window_node * next; /**< Pointer to the next window node */
    struct _xcwm_window_node * prev; /**< Pointer to the previous window node */
} _xcwm_window_node;

/* this is the head pointer */
extern _xcwm_window_node *_xcwm_window_list_head;

/**
 * Add a newly created window to the context_list.
 * @param window The window to be added to the linked list
 * @return Pointer to window added to the list.
 */
xcwm_window_t *
_xcwm_add_window(xcwm_window_t *window);

/**
 * Remove a context to the context_list using the window's id.
 * @param window_id The window_id of the window which should
 * be removed from the context_list
 */
void
_xcwm_remove_window_node(xcb_window_t window_id);

/**
 * Find a context in the doubly linked list using its window_id.
 * @param window_id The window_id of the context which should
 * @return Pointer to window (if found), NULL if not found.
 */
xcwm_window_t *
_xcwm_get_window_node_by_window_id(xcb_window_t window_id);

/****************
* window.c
****************/

/**
 * Create a new context for the window specified in the event.
 * @param context The context window was created in.
 * @param new_window ID of the window being created.
 * @param parent ID of the new windows parent.
 * @return Pointer to new window. NULL if window already exists.
 */
xcwm_window_t *
_xcwm_window_create(xcwm_context_t *context, xcb_window_t new_window,
                    xcb_window_t parent);

/**
 * Destroy the damage object associated with the window and
 * remove the window from the list of managed windows. Memory allocated
 * to the window must be removed with a call to _xcwm_window_release().
 * Call the remove function in context_list.c
 * @param conn The connection to xserver
 * @param event The destroy notify event for the window
 * @return Pointer to the window that was removed from the list, NULL if
 * window isn't being managed
 */
xcwm_window_t *
_xcwm_window_remove(xcb_connection_t *conn,
                    xcb_window_t window);
/**
 * Release the window and free its memory. Call after client has done
 * necessary clean up of the window on its side after the window has
 * been closed and a XCWM_DESTROY event has been received for the
 * window.
 * @param window The window to release.
 */
void
_xcwm_window_release(xcwm_window_t *window);

/**
 * Resize the window to given x, y, width and height.
 * @param conn The connection
 * @param window The id of window to resize
 * @param x The new x position of window, relative to root.
 * @param y The new y position of window, relative to root.
 * @param width The new width
 * @param height The new height
 */
void
_xcwm_resize_window(xcb_connection_t *conn, xcb_window_t window,
                    int x, int y, int width, int height);

/**
 * Map the given window.
 * @param conn The connection to xserver
 * @param window The window to map
 */
void
_xcwm_map_window(xcb_connection_t *conn, xcwm_window_t *window);


/****************
 * atoms.c
 ****************/

/**
 * Get the values for the WM_* atoms that we need, as well as checking
 * to make sure another window manager is not running on the
 * connection.
 * @param context The context
 * @return Returns 0 on success, otherwise XCB error code.
 */
int
_xcwm_atoms_init(xcwm_context_t *context);

/**
 * Get and set the initial values ICCCM/EWMH atoms for the given
 * window.
 * @param window The window to get and set atoms values for.
 */
void
_xcwm_atoms_init_window(xcwm_window_t *window);

/**
 * Get and set the WM_NAME of the window.
 * @param window The window
 */
void
_xcwm_atoms_set_window_name(xcwm_window_t *window);

/**
 * Get the set the WM_DELETE_WINDOWatom for the winodw.
 * @param window The window.
 */
void
_xcwm_atoms_set_wm_delete(xcwm_window_t *window);

/**
 * Clean up any atom data necessary.
 * @param context The context to clean up.
 */
void
_xcwm_atoms_release(xcwm_context_t *context);

/**
 * Set the ICCCM WM_STATE and EWMH _NET_WM_STATE for the given
 * window to the state seen by the window manager.
 * @param window The window to set the state on
 * @param state The new state.
 */
void
_xcwm_atoms_set_wm_state(xcwm_window_t *window, xcwm_window_state_t state);

#endif  /* _XCWM_INTERNAL_H_ */
