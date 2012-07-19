/* Copyright (c) 2013 Jess VanDerwalker <jvanderw@freedesktop.org>
 *
 * keymap_main.c
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

#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include "keymap.h"
#include "keysym2ucs.h"
#include <stdio.h>

/* Simple driver for a keymap standalone keymap application. Once this
 * application can successfully set the keymap and modifier mapping,
 * this driver will go away, and the keymap.c code will be integrated
 * into XtoQ.  */
int
main (int argc, const char **argv)
{
    printf("Generating keymap on display %s\n", argv[1]);

    xcb_connection_t *conn;
    int conn_screen;

    conn = xcb_connect(argv[1], &conn_screen);

    XtoQKeymapReSync(conn);

    xcb_disconnect(conn);

    return 0;
}
