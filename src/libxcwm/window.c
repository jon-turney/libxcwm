/* Copyright (c) 2012 Benjamin Carr
 *
 * window.c
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

#include <xcb/xfixes.h>
#include <xcb/damage.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcwm/xcwm.h>
#include "xcwm_internal.h"


/* Functions only used within this file */

/* Sets the WM_* properties we care about in context */
void
set_icccm_properties(xcb_connection_t *conn, xcwm_window_t *window);

/* Set the event masks on a window */
void
set_window_event_masks(xcb_connection_t *conn, xcwm_window_t *window);

/* Set the WM_NAME property in context */
void
set_wm_name_in_context(xcb_connection_t *conn, xcwm_window_t *window);

/* Determine values of window WM_SIZE_HINTS */
void
set_wm_size_hints_for_window(xcb_connection_t *conn, xcwm_window_t *window);

/* Initialize damage on a window */
void
init_damage_on_window(xcb_connection_t *conn, xcwm_window_t *window);

/* Set window to the top of the stack */
void
xcwm_window_set_to_top(xcwm_window_t *window)
{

    const static uint32_t values[] = { XCB_STACK_MODE_ABOVE };

    /* Move the window on the top of the stack */
    xcb_configure_window(window->context->conn, window->window_id,
                         XCB_CONFIG_WINDOW_STACK_MODE, values);
}

/* Set window to the bottom of the stack */
void
xcwm_window_set_to_bottom(xcwm_window_t *window)
{

    const static uint32_t values[] = { XCB_STACK_MODE_BELOW };

    /* Move the window on the top of the stack */
    xcb_configure_window(window->context->conn, window->window_id,
                         XCB_CONFIG_WINDOW_STACK_MODE, values);
}

/* Set input focus to window */
void
xcwm_window_set_input_focus(xcwm_window_t *window)
{

    // Test -- David
    xcb_get_input_focus_cookie_t cookie =
        xcb_get_input_focus(window->context->conn);
    xcb_get_input_focus_reply_t *reply =
        xcb_get_input_focus_reply(window->context->conn, cookie, NULL);
    printf("Focus was in window #%d, now in #%d (window.c)\n",
           reply->focus, window->window_id);
    free(reply);

    // End test -- David

    xcb_set_input_focus(window->context->conn, XCB_INPUT_FOCUS_PARENT,
                        window->window_id, XCB_CURRENT_TIME);
    xcb_flush(window->context->conn);
}

xcwm_window_t *
_xcwm_window_create(xcwm_context_t *context, xcb_window_t new_window,
                     xcb_window_t parent)
{

    /* Check to see if the window is already being managed */
    if (_xcwm_get_window_node_by_window_id(new_window)) {
        return NULL;
    }

    /* allocate memory for new xcwm_window_t and rectangles */
    xcwm_window_t *window = malloc(sizeof(xcwm_window_t));
    assert(window);
    window->bounds = malloc(sizeof(xcwm_rect_t));
    assert(window->bounds);
    window->dmg_bounds = malloc(sizeof(xcwm_rect_t));
    assert(window->dmg_bounds);
    window->sizing = calloc(1, sizeof(*window->sizing));
    assert(window->dmg_bounds);;

    xcb_get_geometry_reply_t *geom;
    geom = _xcwm_get_window_geometry(context->conn, new_window);

    /* set any available values from xcb_create_notify_event_t object pointer
       and geom pointer */
    window->context = context;
    window->window_id = new_window;
    window->bounds->x = geom->x;
    window->bounds->y = geom->y;
    window->bounds->width = geom->width;
    window->bounds->height = geom->height;

    /* Find an set the parent */
    window->parent = _xcwm_get_window_node_by_window_id(parent);

    free(geom);
    
    /* Get value of override_redirect flag */
    xcb_get_window_attributes_reply_t *attrs =
        _xcwm_get_window_attributes(context->conn, new_window);
    window->override_redirect = attrs->override_redirect;
    /* FIXME: Workaround for initial damage reporting for
     * override-redirect windows. Initial damage event is relative to
     * root, not window */
    if (window->override_redirect) {
        window->initial_damage = 1;
    } else {
        window->initial_damage = 0;
    }
    free(attrs);

    /* Set the event masks for the window */
    set_window_event_masks(context->conn, window);

    /* Set the ICCCM properties we care about */
    set_icccm_properties(context->conn, window);

    /* register for damage */
    init_damage_on_window(context->conn, window);

    /* add context to context_list */
    window = _xcwm_add_window(window);

    return window;
}

xcwm_window_t *
_xcwm_window_remove(xcb_connection_t *conn, xcb_window_t window)
{

    xcwm_window_t *removed =
        _xcwm_get_window_node_by_window_id(window);
    if (!removed) {
        /* Window isn't being managed */
        return NULL;
    }

    /* Destroy the damage object associated with the window. */
    xcb_damage_destroy(conn, removed->damage);

    /* Call the remove function in context_list.c */
    _xcwm_remove_window_node(removed->window_id);

    /* Return the pointer for the context that was removed from the list. */
    return removed;
}

void
xcwm_window_configure(xcwm_window_t *window, int x, int y,
                      int height, int width)
{

    /* Set values for xcwm_window_t */
    window->bounds->x = x;
    window->bounds->y = y;
    window->bounds->width = width;
    window->bounds->height = height;

    uint32_t values[] = {
        (uint32_t)x,     (uint32_t)y,
        (uint32_t)width, (uint32_t)height
    };

    xcb_configure_window(window->context->conn,
                         window->window_id,
                         XCB_CONFIG_WINDOW_X |
                         XCB_CONFIG_WINDOW_Y |
                         XCB_CONFIG_WINDOW_WIDTH |
                         XCB_CONFIG_WINDOW_HEIGHT,
                         values);

    /* Set the damage area to the new window size so its redrawn properly */
    window->dmg_bounds->width = width;
    window->dmg_bounds->height = height;

    xcb_flush(window->context->conn);
    return;
}

void
xcwm_window_remove_damage(xcwm_window_t *window)
{
    xcb_xfixes_region_t region = xcb_generate_id(window->context->conn);
    xcb_rectangle_t rect;
    xcb_void_cookie_t cookie;

    if (!window) {
        return;
    }

    rect.x = window->dmg_bounds->x;
    rect.y = window->dmg_bounds->y;
    rect.width = window->dmg_bounds->width;
    rect.height = window->dmg_bounds->height;

    xcb_xfixes_create_region(window->context->conn,
                             region,
                             1,
                             &rect);

    cookie = xcb_damage_subtract_checked(window->context->conn,
                                         window->damage,
                                         region,
                                         0);

    if (!(_xcwm_request_check(window->context->conn, cookie,
                              "Failed to subtract damage"))) {
        window->dmg_bounds->x = 0;
        window->dmg_bounds->y = 0;
        window->dmg_bounds->width = 0;
        window->dmg_bounds->height = 0;
    }
    return;
}

void
xcwm_window_request_close(xcwm_window_t *window)
{

    /* check to see if the context is in the list */

    if (!_xcwm_get_window_node_by_window_id(window->window_id))
        return;

    /* kill using xcb_kill_client */
    if (!window->wm_delete_set == 1) {
        xcb_kill_client(window->context->conn, window->window_id);
        xcb_flush(window->context->conn);
        return;
    }
    /* kill using WM_DELETE_WINDOW */
    if (window->wm_delete_set == 1) {
        xcb_client_message_event_t event;

        memset(&event, 0, sizeof(xcb_client_message_event_t));

        event.response_type = XCB_CLIENT_MESSAGE;
        event.window = window->window_id;
        event.type = _wm_atoms->wm_protocols_atom;
        event.format = 32;
        event.data.data32[0] = _wm_atoms->wm_delete_window_atom;
        event.data.data32[1] = XCB_CURRENT_TIME;

        xcb_send_event(window->context->conn, 0, window->window_id,
                       XCB_EVENT_MASK_NO_EVENT,
                       (char *)&event);
        xcb_flush(window->context->conn);
        return;
    }
    return;
}

void
_xcwm_window_release(xcwm_window_t *window)
{

    if (!window) {
        return;
    }

    free(window->bounds);
    free(window->sizing);
    if (window->dmg_bounds) {
        free(window->dmg_bounds);
    }
    if (window->name) {
        free(window->name);
    }
    free(window);
}

/* Accessor functions into xcwm_window_t */

xcwm_window_type_t
xcwm_window_get_window_type(xcwm_window_t const *window)
{

    return window->type;
}

xcwm_context_t *
xcwm_window_get_context(xcwm_window_t const *window)
{

    return window->context;
}

xcwm_window_t *
xcwm_window_get_parent(xcwm_window_t const *window)
{

    return window->parent;
}

xcwm_window_t const *
xcwm_window_get_transient_for(xcwm_window_t const *window)
{

    return window->transient_for;
}

int
xcwm_window_is_override_redirect(xcwm_window_t const *window)
{

    return window->override_redirect;
}

void *
xcwm_window_get_local_data(xcwm_window_t const *window)
{

    return window->local_data;
}

void
xcwm_window_set_local_data(xcwm_window_t *window, void *data_ptr)
{

    window->local_data = data_ptr;
}

xcwm_rect_t *
xcwm_window_get_full_rect(xcwm_window_t const *window)
{

    return window->bounds;
}

xcwm_rect_t *
xcwm_window_get_damaged_rect(xcwm_window_t const *window)
{

    return window->dmg_bounds;
}

char *
xcwm_window_copy_name(xcwm_window_t const *window)
{

    return strdup(window->name);
}

xcwm_window_sizing_t const *
xcwm_window_get_sizing(xcwm_window_t const *window)
{

    return window->sizing;
}

/* Resize the window on server side */
void
_xcwm_resize_window(xcb_connection_t *conn, xcb_window_t window,
                    int width, int height)
{

    uint32_t values[2] = { width, height };

    xcb_configure_window(conn,
                         window,
                         XCB_CONFIG_WINDOW_WIDTH |
                         XCB_CONFIG_WINDOW_HEIGHT,
                         values);
    xcb_flush(conn);
}

void
_xcwm_map_window(xcb_connection_t *conn, xcwm_window_t *window)
{
    /* Map the window. May want to handle other things here */
    xcb_map_window(conn, window->window_id);
    xcb_flush(conn);
}

void
set_window_event_masks(xcb_connection_t *conn, xcwm_window_t *window)
{
    uint32_t values[1] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
    
    xcb_change_window_attributes(conn, window->window_id,
                                 XCB_CW_EVENT_MASK, values);
}

void
set_icccm_properties(xcb_connection_t *conn, xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_window_t transient;
    xcb_generic_error_t *error;
    uint8_t success;

    set_wm_name_in_context(conn, window);
    _xcwm_window_set_wm_delete(conn, window);
    set_wm_size_hints_for_window(conn, window);

    /* Get the window this one is transient for */
    cookie = xcb_icccm_get_wm_transient_for(conn, window->window_id);
    success = xcb_icccm_get_wm_transient_for_reply(conn, cookie,
                                                   &transient, &error);
    if (success) {
        window->transient_for = _xcwm_get_window_node_by_window_id(transient);
        /* FIXME: Currently we assume that any window that is
         * transient for another is a dialog. */
        window->type = XCWM_WINDOW_TYPE_DIALOG;
    } else {
        window->transient_for = NULL;
        window->type = XCWM_WINDOW_TYPE_NORMAL;
    }
}

void
set_wm_name_in_context(xcb_connection_t *conn, xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_text_property_reply_t reply;
    xcb_generic_error_t *error;

    cookie = xcb_icccm_get_wm_name(conn, window->window_id);
    if (!xcb_icccm_get_wm_name_reply(conn, cookie, &reply, &error)) {
        window->name = malloc(sizeof(char));
        window->name[0] = '\0';
        return;
    }

    window->name = malloc(sizeof(char) * (reply.name_len + 1));
    strncpy(window->name, reply.name, reply.name_len);
    window->name[reply.name_len] = '\0';
    xcb_icccm_get_text_property_reply_wipe(&reply);
}

void
_xcwm_window_set_wm_delete(xcb_connection_t *conn, xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_wm_protocols_reply_t reply;
    xcb_generic_error_t *error;
    int i;

    /* Get the WM_PROTOCOLS */
    cookie = xcb_icccm_get_wm_protocols(conn, window->window_id,
                                        _wm_atoms->wm_protocols_atom);

    if (xcb_icccm_get_wm_protocols_reply(conn, cookie, &reply, &error) == 1) {
        /* See if the WM_DELETE_WINDOW is in WM_PROTOCOLS */
        for (i = 0; i < reply.atoms_len; i++) {
            if (reply.atoms[i] == _wm_atoms->wm_delete_window_atom) {
                window->wm_delete_set = 1;
                break;
            } 
        }
    } else {
        window->wm_delete_set = 0;
        return;
    }
    xcb_icccm_get_wm_protocols_reply_wipe(&reply);

    return;
}

void
set_wm_size_hints_for_window(xcb_connection_t *conn, xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_size_hints_t hints;
    
    cookie = xcb_icccm_get_wm_normal_hints(conn, window->window_id);
    if (!xcb_icccm_get_wm_normal_hints_reply(conn, cookie, &hints, NULL)) {
        /* Use 0 for all values (as set in calloc), or previous values */
        return;
    }
    window->sizing->min_width = hints.min_width;
    window->sizing->min_height = hints.min_height;;
    window->sizing->max_width = hints.max_width;
    window->sizing->max_height = hints.max_height;
    window->sizing->width_inc = hints.width_inc;
    window->sizing->height_inc = hints.height_inc;
}

void
init_damage_on_window(xcb_connection_t *conn, xcwm_window_t *window)
{
    xcb_damage_damage_t damage_id;
    uint8_t level;
    xcb_void_cookie_t cookie;

    damage_id = xcb_generate_id(conn);

    // Refer to the Damage Protocol. level = 0 corresponds to the level
    // DamageReportRawRectangles.  Another level may be more appropriate.
    level = XCB_DAMAGE_REPORT_LEVEL_BOUNDING_BOX;
    cookie = xcb_damage_create(conn,
                               damage_id,
                               window->window_id,
                               level);

    if (_xcwm_request_check(conn, cookie,
                            "Could not create damage for window")) {
        window->damage = 0;
        return;
    }
    /* Assign this damage object to the roots window's context */
    window->damage = damage_id;

    /* Set the damage area in the context to zero */
    window->dmg_bounds->x = 0;
    window->dmg_bounds->y = 0;
    window->dmg_bounds->width = 0;
    window->dmg_bounds->height = 0;
}
