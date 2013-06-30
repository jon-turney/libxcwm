/* Copyright (c) 2013 Jess VanDerwalker <jvanderw@freedesktop.org>
 *
 * atoms.c
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

#include <string.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <xcwm/xcwm.h>
#include "xcwm_internal.h"

/* Local functions */

/* Check to make sure another EWMH window manager is not running */
int
check_wm_cm_owner(xcwm_context_t *context);

/* Create the window required to show a EWMH compliant window manager
 * is running */
void
create_wm_cm_window(xcwm_context_t *context);

/* Determine the window type */
static void
setup_window_type(xcwm_window_t *window);

/* Get and set the size hints for the window */
static void
set_window_size_hints(xcwm_window_t *window);

/* Get opacity hint */
static void
set_window_opacity(xcwm_window_t *window);

static xcb_atom_t
_xcwm_atom_get(xcwm_context_t *context, const char *atomName)
{
    xcb_intern_atom_reply_t *atom_reply;
    xcb_intern_atom_cookie_t atom_cookie;
    xcb_atom_t atom = XCB_NONE;

    atom_cookie = xcb_intern_atom(context->conn,
                                  0,
                                  strlen(atomName),
                                  atomName);
    atom_reply = xcb_intern_atom_reply(context->conn,
                                       atom_cookie,
                                       NULL);
    if (atom_reply) {
        atom = atom_reply->atom;
        free(atom_reply);
    }
    return atom;
}

int
_xcwm_atoms_init(xcwm_context_t *context)
{
    xcb_intern_atom_cookie_t *atom_cookies;
    xcb_generic_error_t *error;

    /* Initialization for the xcb_ewmh connection and EWMH atoms */
    atom_cookies = xcb_ewmh_init_atoms(context->conn,
                                       &context->atoms->ewmh_conn);
    if (!xcb_ewmh_init_atoms_replies(&context->atoms->ewmh_conn,
                                     atom_cookies, &error)) {
        return error->major_code;;
    }

    /* Set the _NET_SUPPORTED atom for this context
     * Most of these are defined as MUSTs in the
     * EWMH standards for window managers.
     * NOTE: Starting with only a limited set of _NET_WM_STATE. This
     * may be expanded. */
    xcb_atom_t supported[] =
        {
            context->atoms->ewmh_conn.WM_PROTOCOLS,
            context->atoms->ewmh_conn._NET_SUPPORTED,
            context->atoms->ewmh_conn._NET_SUPPORTING_WM_CHECK,
            context->atoms->ewmh_conn._NET_CLOSE_WINDOW,
            context->atoms->ewmh_conn._NET_WM_NAME,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_TOOLBAR,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_MENU,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_UTILITY,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_SPLASH,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_DIALOG,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_POPUP_MENU,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_TOOLTIP,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_NOTIFICATION,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_COMBO,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_DND,
            context->atoms->ewmh_conn._NET_WM_WINDOW_TYPE_NORMAL,
            context->atoms->ewmh_conn._NET_WM_STATE,
            context->atoms->ewmh_conn._NET_WM_STATE_MODAL,
            context->atoms->ewmh_conn._NET_WM_STATE_HIDDEN
        };

    xcb_ewmh_set_supported(&context->atoms->ewmh_conn,
                           context->conn_screen,
                           21,  /* Length of supported[] */
                           supported);

    /* Get the ICCCM atoms we need that are not included in the
     * xcb_ewmh_connection_t. */

    /* WM_DELETE_WINDOW atom */
    context->atoms->wm_delete_window_atom = _xcwm_atom_get(context, "WM_DELETE_WINDOW");

    /* WM_NAME */
    context->atoms->wm_name_atom = _xcwm_atom_get(context, "WM_NAME");

    if (!check_wm_cm_owner(context)) {
        return XCB_WINDOW;
    }
    create_wm_cm_window(context);

    /* WM_STATE atom */
    context->atoms->wm_state_atom = _xcwm_atom_get(context, "WM_STATE");

    /* NET_WM_OPACITY */
    context->atoms->wm_opacity_atom = _xcwm_atom_get(context, "_NET_WM_WINDOW_OPACITY");

    return 0;
}

int
check_wm_cm_owner(xcwm_context_t *context)
{
    xcb_get_selection_owner_cookie_t cookie;
    xcb_window_t wm_owner;

    cookie = xcb_ewmh_get_wm_cm_owner(&context->atoms->ewmh_conn,
                                      context->conn_screen);
    xcb_ewmh_get_wm_cm_owner_reply(&context->atoms->ewmh_conn,
                                   cookie, &wm_owner, NULL);
    if (wm_owner != XCB_NONE) {
        return 0;
    }
    return 1;
}

void
create_wm_cm_window(xcwm_context_t *context)
{
    context->wm_cm_window = xcb_generate_id(context->conn);

    printf("Created CM window with XID 0x%08x\n", context->wm_cm_window);

    xcb_create_window(context->conn,
                      XCB_COPY_FROM_PARENT,
                      context->wm_cm_window,
                      context->root_window->window_id,
                      0, 0, 1, 1, 0, /* 1x1 size, no border */
                      XCB_COPY_FROM_PARENT,
                      XCB_COPY_FROM_PARENT,
                      0, NULL);

    /* Set the atoms for the window */
    xcb_ewmh_set_wm_name(&context->atoms->ewmh_conn,
                         context->wm_cm_window,
                         strlen("xcwm"), "xcwm");

    xcb_ewmh_set_supporting_wm_check(&context->atoms->ewmh_conn,
                                     context->root_window->window_id,
                                     context->wm_cm_window);

    xcb_ewmh_set_supporting_wm_check(&context->atoms->ewmh_conn,
                                     context->wm_cm_window,
                                     context->wm_cm_window);
}

void
_xcwm_atoms_init_window(xcwm_window_t *window)
{
    _xcwm_atoms_set_window_name(window);
    _xcwm_atoms_set_wm_delete(window);
    setup_window_type(window);
    set_window_size_hints(window);
    set_window_opacity(window);
}


void
_xcwm_atoms_set_window_name(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_text_property_reply_t reply;
    xcb_ewmh_get_utf8_strings_reply_t data;

    /* Check _NET_WM_NAME first */
    cookie = xcb_ewmh_get_wm_name(&window->context->atoms->ewmh_conn,
                                  window->window_id);
    if (xcb_ewmh_get_wm_name_reply(&window->context->atoms->ewmh_conn,
                                   cookie, &data, NULL)) {
        window->name = strndup(data.strings, data.strings_len);
        xcb_ewmh_get_utf8_strings_reply_wipe(&data);
        return;
    }

    cookie = xcb_icccm_get_wm_name(window->context->conn, window->window_id);
    if (!xcb_icccm_get_wm_name_reply(window->context->conn,
                                     cookie, &reply, NULL)) {
        window->name = malloc(sizeof(char));
        window->name[0] = '\0';
        return;
    }

    window->name = strndup(reply.name, reply.name_len);
    xcb_icccm_get_text_property_reply_wipe(&reply);
}


void
_xcwm_atoms_set_wm_delete(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_wm_protocols_reply_t reply;
    xcb_generic_error_t *error;
    int i;

    /* Get the WM_PROTOCOLS */
    cookie = xcb_icccm_get_wm_protocols(window->context->conn,
                                        window->window_id,
                                        window->context->atoms->ewmh_conn.WM_PROTOCOLS);

    if (xcb_icccm_get_wm_protocols_reply(window->context->conn,
                                         cookie, &reply, &error) == 1) {
        /* See if the WM_DELETE_WINDOW is set in WM_PROTOCOLS */
        for (i = 0; i < reply.atoms_len; i++) {
            if (reply.atoms[i] == window->context->atoms->wm_delete_window_atom) {
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

static void
setup_window_type(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_window_t transient;
    xcb_ewmh_get_atoms_reply_t type;
    xcb_ewmh_connection_t ewmh_conn = window->context->atoms->ewmh_conn;
    int i;

    /* if nothing below matches, set the default to unknown */
    window->type = XCWM_WINDOW_TYPE_UNKNOWN;

    /* Get the window this one is transient for */
    cookie = xcb_icccm_get_wm_transient_for(window->context->conn,
                                            window->window_id);
    if (xcb_icccm_get_wm_transient_for_reply(window->context->conn, cookie,
                                             &transient, NULL)) {
        window->transient_for = _xcwm_get_window_node_by_window_id(transient);
        window->type = XCWM_WINDOW_TYPE_DIALOG;
        // not if override-redirect
    } else {
        window->transient_for = NULL;
        window->type = XCWM_WINDOW_TYPE_NORMAL;
    }

    /* Check and see if the client has set the _NET_WM_WINDOW_TYPE
     * atom. Since the "type" is a list of window types, ordered by
     * preference, we need to loop through to make sure we get a
     * match. */
    cookie = xcb_ewmh_get_wm_window_type(&ewmh_conn, window->window_id);
    if (xcb_ewmh_get_wm_window_type_reply(&ewmh_conn, cookie, &type, NULL)) {
        for (i = 0; i <= type.atoms_len; i++) {
            if (type.atoms[i] ==  ewmh_conn._NET_WM_WINDOW_TYPE_TOOLBAR) {
                window->type = XCWM_WINDOW_TYPE_TOOLBAR;
                break;
            } else if (type.atoms[i] == ewmh_conn._NET_WM_WINDOW_TYPE_MENU) {
                window->type = XCWM_WINDOW_TYPE_MENU;
                break;
            } else if (type.atoms[i]
                       == ewmh_conn._NET_WM_WINDOW_TYPE_UTILITY) {
                window->type = XCWM_WINDOW_TYPE_UTILITY;
                break;
            } else if (type.atoms[i] == ewmh_conn._NET_WM_WINDOW_TYPE_SPLASH) {
                window->type = XCWM_WINDOW_TYPE_SPLASH;
                break;
            } else if (type.atoms[i] == ewmh_conn._NET_WM_WINDOW_TYPE_DIALOG) {
                window->type = XCWM_WINDOW_TYPE_DIALOG;
                break;
            } else if (type.atoms[i]
                       == ewmh_conn._NET_WM_WINDOW_TYPE_DROPDOWN_MENU) {
                window->type = XCWM_WINDOW_TYPE_DROPDOWN_MENU;
                break;
            } else if (type.atoms[i]
                       == ewmh_conn._NET_WM_WINDOW_TYPE_POPUP_MENU) {
                window->type = XCWM_WINDOW_TYPE_POPUP_MENU;
                break;
            } else if (type.atoms[i]
                       == ewmh_conn._NET_WM_WINDOW_TYPE_TOOLTIP) {
                window->type = XCWM_WINDOW_TYPE_TOOLTIP;
                break;
            } else if (type.atoms[i]
                       == ewmh_conn._NET_WM_WINDOW_TYPE_NOTIFICATION) {
                window->type = XCWM_WINDOW_TYPE_NOTIFICATION;
                break;
            } else if (type.atoms[i] == ewmh_conn._NET_WM_WINDOW_TYPE_COMBO) {
                window->type = XCWM_WINDOW_TYPE_COMBO;
                break;
            } else if (type.atoms[i] == ewmh_conn._NET_WM_WINDOW_TYPE_DND) {
                window->type = XCWM_WINDOW_TYPE_DND;
                break;
            } else if (type.atoms[i] == ewmh_conn._NET_WM_WINDOW_TYPE_NORMAL) {
                window->type = XCWM_WINDOW_TYPE_NORMAL;
                break;
            }
        }
    }
}

void
set_window_size_hints(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    cookie = xcb_icccm_get_wm_normal_hints(window->context->conn,
                                           window->window_id);
    if (!xcb_icccm_get_wm_normal_hints_reply(window->context->conn,
                                             cookie, &(window->size_hints), NULL)) {
        /* Use 0 for all values (as set in calloc), or previous values */
        return;
    }
}

void
set_window_opacity(xcwm_window_t *window)
{
  xcb_get_property_cookie_t cookie;
  cookie = xcb_get_property(window->context->conn, 0, window->window_id, window->context->atoms->wm_opacity_atom, XCB_ATOM_CARDINAL, 0L, 4L);

  xcb_get_property_reply_t *reply = xcb_get_property_reply(window->context->conn, cookie, NULL);
  if (reply)
    {
      int nitems = xcb_get_property_value_length(reply);
      uint32_t *value = xcb_get_property_value(reply);

      if (value && nitems == 4)
        {
          window->opacity = *value;
        }

      free(reply);
    }
}

void
_xcwm_atoms_set_wm_state(xcwm_window_t *window, xcwm_window_state_t state)
{
    /* xcb_icccm_wm_state_t icccm_state; */

    uint32_t icccm_state[2];
    xcb_atom_t *ewmh_state = NULL;
    int ewmh_atom_cnt = 0;

    switch (state) {
    case XCWM_WINDOW_STATE_NORMAL:
    {
        icccm_state[0] = XCB_ICCCM_WM_STATE_NORMAL;
        icccm_state[1] = XCB_NONE;
        break;
    }

    case XCWM_WINDOW_STATE_ICONIC:
    {
        ewmh_atom_cnt = 1;
        icccm_state[0] = XCB_ICCCM_WM_STATE_ICONIC;
        icccm_state[1] = XCB_NONE;

        ewmh_state = calloc(ewmh_atom_cnt, sizeof(xcb_atom_t));
        ewmh_state[0] = window->context->atoms->ewmh_conn._NET_WM_STATE_HIDDEN;
        break;
    }
    default:
    {
        /* No need to attempt to update the state */
        return;
    }
    }

    /* Only set for top-level windows */
    if (!window->transient_for && !window->override_redirect) {
        xcb_change_property(window->context->conn,
                            XCB_PROP_MODE_REPLACE,
                            window->window_id,
                            window->context->atoms->wm_state_atom,
                            window->context->atoms->wm_state_atom,
                            32,
                            2,
                            icccm_state);
    }

    xcb_ewmh_set_wm_state(&window->context->atoms->ewmh_conn,
                          window->window_id,
                          ewmh_atom_cnt,
                          ewmh_state);

    xcb_flush(window->context->conn);

    if (ewmh_state) {
        free(ewmh_state);
    }
}

void
_xcwm_atoms_release(xcwm_context_t *context)
{

    /* Free the xcb_ewmh_connection_t */
    xcb_ewmh_connection_wipe(&context->atoms->ewmh_conn);

    /* Close the wm window */
    xcb_destroy_window(context->conn, context->wm_cm_window);
}
