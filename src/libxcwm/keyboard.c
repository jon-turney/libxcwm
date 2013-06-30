/* Copyright (c) 2012 Jess VanDerwalker <jvanderw@freedesktop.org>
 *
 * keyboard.c
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
#include <xcwm/xcwm.h>
#include <stdlib.h>
#include <stdio.h>

void
xcwm_keyboard_set_mapping (xcwm_context_t *context,
                           uint8_t num_keycodes,
                           xcwm_keycode_t first_keycode,
                           uint8_t keysyms_per_keycode,
                           xcwm_keysym_t *keyMap)
{

    xcb_void_cookie_t cookie;
    xcb_generic_error_t *error;

    cookie = xcb_change_keyboard_mapping_checked(context->conn,
                                                 num_keycodes,
                                                 first_keycode,
                                                 keysyms_per_keycode,
                                                 keyMap);

    error = xcb_request_check(context->conn, cookie);
    if (error) {
        fprintf(stderr, "ERROR: Failed to change keyboard mapping");
        fprintf(stderr, "\nError code: %d\n", error->error_code);
    }

    xcb_flush(context->conn);
}

void
xcwm_keyboard_set_modifier_map (xcwm_context_t *context,
                                uint8_t keycodes_per_modifier,
                                xcwm_keycode_t *modMap)
{

    xcb_set_modifier_mapping_cookie_t map_cookie;
    xcb_set_modifier_mapping_reply_t *map_reply;
    xcb_generic_error_t *error;

    /* Set the modifier mapping */
    map_cookie = xcb_set_modifier_mapping(context->conn,
                                          keycodes_per_modifier,
                                          modMap);
    map_reply = xcb_set_modifier_mapping_reply(context->conn,
                                               map_cookie, &error);
    if (error) {
        fprintf(stderr, "ERROR: Failed to change keyboard modifier mapping");
        fprintf(stderr, "\nError code: %d\n", error->error_code);
    }
    free(map_reply);
    xcb_flush(context->conn);
}
