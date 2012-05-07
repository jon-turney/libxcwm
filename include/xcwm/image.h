/* Copyright (c) 2012 David Snyder
 *
 * xcwm/image.h
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

#ifndef _XCWM_IMAGE_H_
#define _XCWM_IMAGE_H_


#ifndef __XCWM_INDIRECT__
#error "Please #include <xcwm/xcwm.h> instead of this file directly."
#endif

#include <xcb/xcb_image.h>

/**
 * Abstract data type for image data
 */
struct xcwm_image_t {
    xcb_image_t *image;
    int x;
    int y;
    int width;
    int height;
};
typedef struct xcwm_image_t xcwm_image_t;

/**
 * Returns a window's entire image
 * @param window The window to get image from.
 * @return an xcwm_image_t with an the image of a window
 */
xcwm_image_t *
xcwm_image_copy_full (xcwm_window_t *window);

/**
 * Intended for servicing to a client's reaction to a damage notification
 * Returns the portion of the window's image that has been damaged.
 * @param window The window to get image from
 * @return an xcwm_image_t with partial image window contents or
 * NULL if damaged area has zero width or height
 */
xcwm_image_t *
xcwm_image_copy_damaged(xcwm_window_t *window);

/**
 * Free the memory used by an xcwm_image_t created
 * during a call to xcwm_image_get_*.
 * @param image The image to be freed.
 */
void
xcwm_image_destroy(xcwm_image_t *image);


#endif  /* _XCWM_IMAGE_H_ */
