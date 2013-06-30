/* Copyright (c) 2012 Apple Inc
 *
 * xcwm/keyboard.h
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

#ifndef _XCWM_KEYBOARD_H_
#define _XCWM_KEYBOARD_H_

#ifndef __XCWM_INDIRECT__
#error "Please #include <xcwm/xcwm.h> instead of this file directly."
#endif

#include <xcb/xcb.h>

/**
 * Typedefs for keysym and keycode data type.
 *
 * FIXME: Should we be doing this? It does keep user from having to
 * include xcb.h directly in their code, but perhaps there is a better
 * way.
 *
 */
typedef xcb_keysym_t xcwm_keysym_t;
typedef xcb_keycode_t xcwm_keycode_t;

/**
 * Set the keyboard mappping with the given map.
 * @param context The context to update mapping on.
 * @param num_keycodes The number of keycodes being updated.
 * @param first_keycode The first keycode value being updated.
 * @param keysyms_per_keycode The number of keysym values per unique keycode.
 * @param keyMap The new keymap.
 */
void
xcwm_keyboard_set_mapping (xcwm_context_t *context,
                           uint8_t num_keycodes,
                           xcwm_keycode_t first_keycode,
                           uint8_t keysyms_per_keycode,
                           xcwm_keysym_t *keyMap);

/**
 * Set the modifier mapping with the given map.
 * @param context The context to update mapping on.
 * @param keycodes_per_modifier The number of keycodes allowed for
 * each modifier.
 * @param modMap The modifier keymap.
 */
void
xcwm_keyboard_set_modifier_map (xcwm_context_t *context,
                                uint8_t keycodes_per_modifier,
                                xcwm_keycode_t *modMap);

#endif  /* _XCWM_KEYBOARD_H_ */
