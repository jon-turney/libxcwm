/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>
 *
 * xcwm/window.h
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

#ifndef _XCWM_WINDOW_H_
#define _XCWM_WINDOW_H_

#ifndef __XCWM_INDIRECT__
#error "Please #include <xcwm/xcwm.h> instead of this file directly."
#endif

/**
 * Rectangle for defining area of image.
 */
struct xcwm_rect_t {
    int x;
    int y;
    int width;
    int height;
};
typedef struct xcwm_rect_t xcwm_rect_t;

/**
 * Enumeration for different possible window types
 */
enum xcwm_window_type_t {
    XCWM_WINDOW_TYPE_UNKNOWN = 0,
    XCWM_WINDOW_TYPE_COMBO,
    XCWM_WINDOW_TYPE_DESKTOP,
    XCWM_WINDOW_TYPE_DIALOG,
    XCWM_WINDOW_TYPE_DND,
    XCWM_WINDOW_TYPE_DOCK,
    XCWM_WINDOW_TYPE_DROPDOWN_MENU,
    XCWM_WINDOW_TYPE_MENU,
    XCWM_WINDOW_TYPE_NORMAL,
    XCWM_WINDOW_TYPE_NOTIFICATION,
    XCWM_WINDOW_TYPE_POPUP_MENU,
    XCWM_WINDOW_TYPE_SPLASH,
    XCWM_WINDOW_TYPE_TOOLBAR,
    XCWM_WINDOW_TYPE_TOOLTIP,
    XCWM_WINDOW_TYPE_UTILITY,
};
typedef enum xcwm_window_type_t xcwm_window_type_t;

/**
 * Enumeration for possible supported window states.
 */
enum xcwm_window_state_t {
    XCWM_WINDOW_STATE_UNKNOWN = 0,
    XCWM_WINDOW_STATE_NORMAL,
    XCWM_WINDOW_STATE_ICONIC
};
typedef enum xcwm_window_state_t xcwm_window_state_t;

/**
 * Structure defining min/max size for window and resizing
 * increments. A value of 0 in any field implies the field is unset.
 */
struct xcwm_window_sizing_t {
    uint32_t min_width;
    uint32_t min_height;
    uint32_t max_width;
    uint32_t max_height;
    uint32_t width_inc;
    uint32_t height_inc;
};
typedef struct xcwm_window_sizing_t xcwm_window_sizing_t;

/* Structure for holding data for a window */
struct xcwm_window_t {
    xcb_drawable_t window_id;
    xcwm_context_t *context;
    xcwm_window_type_t type;    /* The type of this window */
    struct xcwm_window_t *parent;
    struct xcwm_window_t *transient_for; /* Window this one is transient for */
    xcb_damage_damage_t damage;
    xcwm_rect_t *bounds;
    xcwm_rect_t *dmg_bounds;
    xcwm_window_sizing_t *sizing; /* Sizing information for the window */
    char *name;         /* The name of the window */
    int wm_delete_set;  /* Flag for WM_DELETE_WINDOW, 1 if set */
    int override_redirect;
    int initial_damage;         /* Set to 1 for override-redirect windows */
    void *local_data;   /* Area for data client cares about */
};

/**
 * Set input focus to the window in context
 * @param window The window to set focus to
 */
void
xcwm_window_set_input_focus(xcwm_window_t *window);

/**
 * Set a window to the bottom of the window stack.
 * @param window The window to move to bottom
 */
void
xcwm_window_set_to_bottom(xcwm_window_t *window);
/**
 * Set a window to the top of the window stack.
 * @param window The window to move.
 */
void
xcwm_window_set_to_top(xcwm_window_t *window);

/**
 * Remove the damage from a given window.
 * @param window The window to remove damage from
 */
void
xcwm_window_remove_damage(xcwm_window_t *window);

/**
 * kill the window, if possible using WM_DELETE_WINDOW (icccm)
 * otherwise using xcb_kill_client.
 * @param window The window to be closed.
 */
void
xcwm_window_request_close(xcwm_window_t *window);

/**
 * move and/or resize the window, update the context
 * @param window The window to configure
 * @param x The new x coordinate
 * @param y The new y coordinate
 * @param height The new height
 * @param width The new width
 */
void
xcwm_window_configure(xcwm_window_t *window, int x, int y,
                      int height, int width);

/**
 * Get the window's type.
 * @param window Window to get type for.
 * @return The type of the window.
 */
xcwm_window_type_t
xcwm_window_get_window_type(xcwm_window_t const *window);

/**
 * Get the context for this window.
 * @param window The window to get context from.
 * @return The context of the window.
 */
xcwm_context_t *
xcwm_window_get_context(xcwm_window_t const *window);

/**
 * Get this window's parent window.
 * @param window The child window.
 * @return This window's parent.
 */
xcwm_window_t *
xcwm_window_get_parent(xcwm_window_t const *window);

/**
 * Get the window the given window is transient for.
 * @param window
 * @return This windows "transient for" window, NULL if none.
 */
xcwm_window_t const *
xcwm_window_get_transient_for(xcwm_window_t const *window);

/**
 * Determine if window has override redirect flag set.
 * @param window
 * @return 1 if override redirect set on window, else 0.
 */
int
xcwm_window_is_override_redirect(xcwm_window_t const *window);

/**
 * Get the local data pointer for this window.
 * @param window The window to get local data from.
 * @return Local data pointer.
 */
void *
xcwm_window_get_local_data(xcwm_window_t const *window);

/**
 * Set the local data pointer for window.
 * @param window Window to assign pointer to.
 * @param data_ptr Pointer to data client associates with this window.
 */
void
xcwm_window_set_local_data(xcwm_window_t *window, void *data_ptr);

/**
 * Get the rectangle describing the size and position of the window.
 * @param window Window to get rectangle from.
 * @return The rectangle.
 */
xcwm_rect_t *
xcwm_window_get_full_rect(xcwm_window_t const *window);

/**
 * Get the damaged area within the given window.
 * @param window The window to get damage from.
 * @return Rectangle describing damaged area.
 */
xcwm_rect_t *
xcwm_window_get_damaged_rect(xcwm_window_t const *window);

/**
 * Get a copy of the name of the window. Client is responsible for freeing
 * memory created for the copy.
 * @param window The window to get name from.
 * @return Name of window. 
 */
char *
xcwm_window_copy_name(xcwm_window_t const *window);

/**
 * Get the sizing information for the window.
 * @param window The window to get sizing data for.
 * @return The structure containing the sizing data
 */
xcwm_window_sizing_t const *
xcwm_window_get_sizing(xcwm_window_t const *window);

/**
 * Set the window to an iconic state. Usually this means the window
 * has been minimized.
 * @param window The window to iconify
 */
void
xcwm_window_iconify(xcwm_window_t *window);

/**
 * Set the window to a normal state, meaning that the window has moved
 * from an iconic/minimized state to a deiconic/de-minimized state.
 * @param window The window being deiconified
 */
void
xcwm_window_deiconify(xcwm_window_t *window);

#endif  /* _XCWM_WINDOW_H_ */
