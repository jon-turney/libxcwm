/*
 * Copyright (c) 2012 Apple Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#ifndef XTOQ_KEYBOARD_H
#define XTOQ_KEYBOARD_H

/* It is assumed that appropriate serialization is done by the *CALLER* of these APIs
 * These should all be called on the same serial queue that handles sending NSEvents
 * to the server.
 */

int
XtoQModifierNXKeycodeToNXKey(unsigned char keycode, int *outSide);
int
XtoQModifierNXKeyToNXKeycode(int key, int side);
int
XtoQModifierNXKeyToNXMask(int key);
int
XtoQModifierNXMaskToNXKey(int mask);
int
XtoQModifierStringToNXMask(const char *string, int separatelr);

void
XtoQKeymapReSync(xcb_connection_t *conn);

#endif /* XTOQ_KEYBOARD_H */
