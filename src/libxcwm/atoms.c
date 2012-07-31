/* Copyright (c) 2013 Jess VanDerwalker <jvanderw@freedesktop.org>
 *
 * atom.c
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

/* Get and set the size hints for the window */
void
set_window_size_hints(xcwm_window_t *window);


void
_xcwm_atoms_init(xcwm_context_t *context)
{
    xcb_intern_atom_reply_t *atom_reply;
    xcb_intern_atom_cookie_t atom_cookie;

    /* WM_PROTOCOLS */
    atom_cookie = xcb_intern_atom(context->conn,
                                  0,
                                  strlen("WM_PROTOCOLS"),
                                  "WM_PROTOCOLS");
    atom_reply = xcb_intern_atom_reply(context->conn,
                                       atom_cookie,
                                       NULL);
    if (!atom_reply) {
        context->atoms->wm_protocols_atom = 0;
    }
    else {
        context->atoms->wm_protocols_atom = atom_reply->atom;
        free(atom_reply);
    }

    /* WM_DELETE_WINDOW atom */
    atom_cookie = xcb_intern_atom(context->conn,
                                  0,
                                  strlen("WM_DELETE_WINDOW"),
                                  "WM_DELETE_WINDOW");
    atom_reply = xcb_intern_atom_reply(context->conn,
                                       atom_cookie,
                                       NULL);
    if (!atom_reply) {
        context->atoms->wm_delete_window_atom = 0;
    }
    else {
        context->atoms->wm_delete_window_atom = atom_reply->atom;
        free(atom_reply);
    }

}

void
_xcwm_atoms_init_window(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_window_t transient;
    xcb_generic_error_t *error;
    uint8_t success;

    _xcwm_atoms_set_window_name(window);
    _xcwm_atoms_set_wm_delete(window);
    set_window_size_hints(window);

    /* Get the window this one is transient for */
    cookie = xcb_icccm_get_wm_transient_for(window->context->conn,
                                            window->window_id);
    success = xcb_icccm_get_wm_transient_for_reply(window->context->conn,
                                                   cookie,
                                                   &transient,
                                                   &error);
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
_xcwm_atoms_set_window_name(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_text_property_reply_t reply;
    xcb_generic_error_t *error;

    cookie = xcb_icccm_get_wm_name(window->context->conn, window->window_id);
    if (!xcb_icccm_get_wm_name_reply(window->context->conn,
                                     cookie, &reply, &error)) {
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
_xcwm_atoms_set_wm_delete(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_icccm_get_wm_protocols_reply_t reply;
    xcb_generic_error_t *error;
    int i;

    /* Get the WM_PROTOCOLS */
    cookie = xcb_icccm_get_wm_protocols(window->context->conn,
                                        window->window_id,
                                        window->context->atoms->wm_protocols_atom);

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

void
set_window_size_hints(xcwm_window_t *window)
{
    xcb_get_property_cookie_t cookie;
    xcb_size_hints_t hints;
    
    cookie = xcb_icccm_get_wm_normal_hints(window->context->conn,
                                           window->window_id);
    if (!xcb_icccm_get_wm_normal_hints_reply(window->context->conn,
                                             cookie, &hints, NULL)) {
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
