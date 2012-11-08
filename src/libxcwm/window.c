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
#include <xcwm/xcwm.h>
#include "xcwm_internal.h"


/* Functions only used within this file */

/* Set the event masks on a window */
void
set_window_event_masks(xcb_connection_t *conn, xcwm_window_t *window);

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

    /* Ignore InputOnly windows */
    xcb_get_window_attributes_reply_t *attrs =
        _xcwm_get_window_attributes(context->conn, new_window);
    if ((!attrs) || (attrs->_class == XCB_WINDOW_CLASS_INPUT_ONLY))
        return NULL;

    xcb_get_geometry_reply_t *geom;
    geom = _xcwm_get_window_geometry(context->conn, new_window);
    if (!geom)
        return NULL;

    /* allocate memory for new xcwm_window_t and rectangles */
    xcwm_window_t *window = malloc(sizeof(xcwm_window_t));
    assert(window);
    window->bounds = malloc(sizeof(xcwm_rect_t));
    assert(window->bounds);
    window->dmg_bounds = malloc(sizeof(xcwm_rect_t));
    assert(window->dmg_bounds);
    window->sizing = calloc(1, sizeof(*window->sizing));
    assert(window->dmg_bounds);;

    /* set any available values from xcb_create_notify_event_t object pointer
       and geom pointer */
    window->context = context;
    window->window_id = new_window;
    window->bounds->x = geom->x;
    window->bounds->y = geom->y;
    window->bounds->width = geom->width;
    window->bounds->height = geom->height;
    window->opacity = ~0;
    window->composite_pixmap_id = 0;
    window->local_data = 0;

    /* Find and set the parent */
    window->parent = _xcwm_get_window_node_by_window_id(parent);
    free(geom);

    /* Get value of override_redirect flag */
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
    _xcwm_atoms_init_window(window);

    /* register for damage */
    init_damage_on_window(context->conn, window);

    /* add window to window list for this context */
    window = _xcwm_add_window(window);

    /* Set the WM_STATE of the window to normal */
    _xcwm_atoms_set_wm_state(window, XCWM_WINDOW_STATE_NORMAL);

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

    /* Remove window from window list for this context */
    _xcwm_remove_window_node(removed->window_id);

    /* Return the pointer to the window that was removed from the list. */
    return removed;
}

void
xcwm_window_configure(xcwm_window_t *window, int x, int y,
                      int width, int height)
{

    /* Set values for xcwm_window_t */
    window->bounds->x = x;
    window->bounds->y = y;
    window->bounds->width = width;
    window->bounds->height = height;

    _xcwm_resize_window(window->context->conn, window->window_id,
                        x, y, width, height);
    /* Set the damage area to the new window size so its redrawn properly */
    window->dmg_bounds->width = width;
    window->dmg_bounds->height = height;
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
    /* check to see if the window is in the list */
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
        event.type = window->context->atoms->ewmh_conn.WM_PROTOCOLS;
        event.format = 32;
        event.data.data32[0] = window->context->atoms->wm_delete_window_atom;
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

unsigned int
xcwm_window_get_opacity(xcwm_window_t const *window)
{
  return window->opacity;
}

xcwm_window_sizing_t const *
xcwm_window_get_sizing(xcwm_window_t const *window)
{
    window->sizing->min_width = window->size_hints.min_width;
    window->sizing->min_height = window->size_hints.min_height;;
    window->sizing->max_width = window->size_hints.max_width;
    window->sizing->max_height = window->size_hints.max_height;
    window->sizing->width_inc = window->size_hints.width_inc;
    window->sizing->height_inc = window->size_hints.height_inc;
    return window->sizing;
}

void
xcwm_window_constrain_sizing(xcwm_window_t const *window, int *widthp, int *heightp)
{
  /*/
   * (based on something taken from TWM sources)
   * Respect WM_NORMAL_HINTS constraints for sizing
   */
  const xcb_size_hints_t *hints = &(window->size_hints);

#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;

  /* Accept input values */
  int dwidth = *widthp, dheight = *heightp;

  /* Interpret hints */
  if (hints->flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
    {
      minWidth = hints->min_width;
      minHeight = hints->min_height;
    }
  else if (hints->flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE)
    {
      minWidth = hints->base_width;
      minHeight = hints->base_height;
    }
  else
    minWidth = minHeight = 1;

  if (hints->flags & XCB_ICCCM_SIZE_HINT_BASE_SIZE)
    {
      baseWidth = hints->base_width;
      baseHeight = hints->base_height;
    }
  else if (hints->flags & XCB_ICCCM_SIZE_HINT_P_MIN_SIZE)
    {
      baseWidth = hints->min_width;
      baseHeight = hints->min_height;
    }
  else
    baseWidth = baseHeight = 0;

  if (hints->flags & XCB_ICCCM_SIZE_HINT_P_MAX_SIZE)
    {
      maxWidth = hints->max_width;
      maxHeight = hints->max_height;
    }
  else
    {
      maxWidth = INT_MAX;
      maxHeight = INT_MAX;
    }

  if (hints->flags & XCB_ICCCM_SIZE_HINT_P_RESIZE_INC)
    {
      xinc = hints->width_inc;
      yinc = hints->height_inc;

      /* Avoid divide-by-zero */
      if (xinc == 0) xinc = 1;
      if (yinc == 0) yinc = 1;
    }
  else
    xinc = yinc = 1;

  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth)
    dwidth = minWidth;
  if (dheight < minHeight)
    dheight = minHeight;

  if (dwidth > maxWidth)
    dwidth = maxWidth;
  if (dheight > maxHeight)
    dheight = maxHeight;

  /*
   * Second, fit to base + N * inc
   */
  dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
  dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;

  /*
   * Third, adjust for aspect ratio
   */

  /*
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   *
   */

  if (hints->flags & XCB_ICCCM_SIZE_HINT_P_ASPECT)
    {
      if (hints->min_aspect_num * dheight > hints->min_aspect_den * dwidth)
        {
	  delta = makemult(hints->min_aspect_num * dheight / hints->min_aspect_den - dwidth, xinc);
	  if (dwidth + delta <= maxWidth)
	    dwidth += delta;
	  else
            {
	      delta = makemult(dheight - dwidth*hints->min_aspect_den/hints->min_aspect_num, yinc);
	      if (dheight - delta >= minHeight)
		dheight -= delta;
            }
        }

      if (hints->max_aspect_num * dheight < hints->max_aspect_den * dwidth)
        {
	  delta = makemult(dwidth * hints->max_aspect_den / hints->max_aspect_num - dheight, yinc);
	  if (dheight + delta <= maxHeight)
	    dheight += delta;
	  else
            {
	      delta = makemult(dwidth - hints->max_aspect_num*dheight/hints->max_aspect_den, xinc);
	      if (dwidth - delta >= minWidth)
		dwidth -= delta;
            }
        }
    }

  /* Return computed values */
  *widthp = dwidth;
  *heightp = dheight;
}
#undef makemult


void
xcwm_window_iconify(xcwm_window_t *window)
{
    _xcwm_atoms_set_wm_state(window, XCWM_WINDOW_STATE_ICONIC);
}

void
xcwm_window_deiconify(xcwm_window_t *window)
{
    _xcwm_atoms_set_wm_state(window, XCWM_WINDOW_STATE_NORMAL);
}

/* Resize the window on server side */
void
_xcwm_resize_window(xcb_connection_t *conn, xcb_window_t window,
                    int x, int y, int width, int height)
{

    uint32_t values[] = { x, y, width, height, 0 };

    xcb_configure_window(conn,
                         window,
                         XCB_CONFIG_WINDOW_X |
                         XCB_CONFIG_WINDOW_Y |
                         XCB_CONFIG_WINDOW_WIDTH |
                         XCB_CONFIG_WINDOW_HEIGHT |
                         XCB_CONFIG_WINDOW_BORDER_WIDTH,
                         values);
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
init_damage_on_window(xcb_connection_t *conn, xcwm_window_t *window)
{
    xcb_damage_damage_t damage_id;
    uint8_t level;
    xcb_void_cookie_t cookie;

    damage_id = xcb_generate_id(conn);

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

    /* Assign this damage object to the window */
    window->damage = damage_id;

    /* Initialize the damaged area in the window to zero */
    window->dmg_bounds->x = 0;
    window->dmg_bounds->y = 0;
    window->dmg_bounds->width = 0;
    window->dmg_bounds->height = 0;
}
