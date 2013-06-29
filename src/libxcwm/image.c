/* Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>
 *
 * image.c
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

#include <xcwm/xcwm.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_image.h>
#include "xcwm_internal.h"

xcwm_image_t *
xcwm_image_copy_partial(xcwm_window_t *window, xcwm_rect_t *area)
{
    xcb_image_t *image;

    /* Return null if image is 0 by 0 */
    if (area->width == 0 || area->height == 0) {
        return NULL;
    }

    /* Get the image of the specified area of the window */
    image = xcb_image_get(window->context->conn,
                          window->composite_pixmap_id,
                          area->x,
                          area->y,
                          area->width,
                          area->height,
                          (unsigned int)~0L,
                          XCB_IMAGE_FORMAT_Z_PIXMAP);

    /* Failed to get a valid image, return null */
    if (!image) {
        return NULL;
    }

    xcwm_image_t * xcwm_image = malloc(sizeof(xcwm_image_t));

    xcwm_image->image = image;
    xcwm_image->x = area->x;
    xcwm_image->y = area->y;
    xcwm_image->width = area->width;
    xcwm_image->height = area->height;

    return xcwm_image;
}

xcwm_image_t *
xcwm_image_copy_full(xcwm_window_t *window)
{
    xcb_get_geometry_reply_t *geom_reply;

    geom_reply = _xcwm_get_window_geometry(window->context->conn,
                                           window->window_id);

    if (!geom_reply)
        return NULL;

    xcb_flush(window->context->conn);

    xcwm_rect_t area;
    area.x = 0;
    area.y = 0;
    area.width = geom_reply->width;
    area.height = geom_reply->height;

    /* Get the full image of the window */
    xcwm_image_t *xcwm_image = xcwm_image_copy_partial(window, &area);

    /* The x and y position of the window (unused, but historical) */
    if (xcwm_image) {
        xcwm_image->x = geom_reply->x;
        xcwm_image->y = geom_reply->y;
    }

    free(geom_reply);

    return xcwm_image;
}

xcwm_image_t *
xcwm_image_copy_damaged(xcwm_window_t *window)
{
    xcb_flush(window->context->conn);

    /* Get the image of the damaged area of the window */
    return xcwm_image_copy_partial(window, &(window->dmg_bounds));
}

void
xcwm_image_destroy(xcwm_image_t * image)
{
    xcb_image_destroy(image->image);
    free(image);
}
