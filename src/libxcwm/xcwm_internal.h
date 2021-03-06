/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>
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
 * Structure used to pass necessary data to xcwm_start_event_loop.
 */
typedef struct xcwm_event_connection {
    xcb_connection_t *conn;               /* Connection to listen to events on */
    xcwm_event_cb_t event_callback;       /* Function to call when event caught */
} xcwm_event_connection;

/**
 * Structure to hold WM_* atoms that we care about
 */
struct xcwm_wm_atoms_t {
    xcb_atom_t wm_delete_window_atom;
    xcb_atom_t wm_transient_for_atom;
    xcb_atom_t wm_state_atom;
    xcb_atom_t net_wm_window_type_splashscreen;
    xcb_ewmh_connection_t ewmh_conn;
};
typedef struct xcwm_wm_atoms_t xcwm_wm_atoms_t;

/**
 * Structure to hold connection data
 */
struct xcwm_context_t {
    xcb_connection_t *conn;
    int conn_screen;
    xcwm_window_t *root_window;
    int damage_event_mask;
    int shape_event;
    int fixes_event_base;
    xcb_window_t wm_cm_window;
    xcwm_wm_atoms_t atoms;
};

/**
 *  Structure for holding data for a window
 */
struct xcwm_window_t {
    xcb_drawable_t window_id;
    xcwm_context_t *context;
    xcwm_window_type_t type;    /* The type of this window */
    struct xcwm_window_t *parent;
    struct xcwm_window_t *transient_for; /* Window this one is transient for */
    xcb_damage_damage_t damage;
    xcwm_rect_t bounds;
    xcwm_rect_t dmg_bounds;
    xcb_size_hints_t size_hints; /* WM_NORMAL_HINTS */
    char *name;         /* The name of the window */
    int wm_delete_set;  /* Flag for WM_DELETE_WINDOW, 1 if set */
    int override_redirect;
    int initial_damage;         /* Set to 1 for override-redirect windows */
    void *local_data;   /* Area for data client cares about */
    unsigned int opacity;
    xcb_pixmap_t composite_pixmap_id;
    xcb_shape_get_rectangles_reply_t *shape;
};

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
                    const char *msg);

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
_xcwm_init_extension(xcb_connection_t *conn, const char *extension_name);

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

/**
 * Initialize the shape extension.
 * @param contxt The context
 */
void
_xcwm_init_shape(xcwm_context_t *contxt);

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
 * Remove a window from the context_list using the window's id.
 * @param window_id The window_id of the window which should
 * be removed from the context_list
 */
void
_xcwm_remove_window_node(xcb_window_t window_id);

/**
 * Find a window in the doubly linked list using its window_id.
 * @param window_id The window_id of the window
 * @return Pointer to window (if found), NULL if not found.
 */
xcwm_window_t *
_xcwm_get_window_node_by_window_id(xcb_window_t window_id);

/****************
* window.c
****************/

/**
 * Create a new window
 * @param context The context the window was created in.
 * @param new_window ID of the window being created.
 * @param parent ID of the new window's parent.
 * @return Pointer to new window. NULL if window already exists.
 */
xcwm_window_t *
_xcwm_window_create(xcwm_context_t *context, xcb_window_t new_window,
                    xcb_window_t parent);

/**
 * Destroy the damage object associated with the window and
 * remove the window from the list of managed windows. Memory allocated
 * to the window must be removed with a call to _xcwm_window_release().
 * @param conn The connection to xserver
 * @param window The window being removed
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
 * Update the window shape
 * @param window The id of window to resize
 * @param shaped TRUE if the window is shaped
 */
void
_xcwm_window_set_shape(xcwm_window_t *window, uint8_t shaped);

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
 * Set the WM_DELETE_WINDOW atom for the window.
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

/**
 * Note change of a property atom, and look up corresponding event type
 * @param window The property atom which has changed
 * @param event The corresponding event type
 * @return 0 if there is no corresponding event
 */
int
_xcwm_atom_change_to_event(xcb_atom_t atom, xcwm_window_t *window, xcwm_event_type_t *event);


#endif  /* _XCWM_INTERNAL_H_ */
