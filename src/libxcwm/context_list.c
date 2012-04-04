/* Copyright (c) 2012 Benjamin Carr
 *
 * context_list.c
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

#include "xcwm_internal.h"

_xcwm_window_node *_xcwm_window_list_head = NULL;

xcwm_window_t *
_xcwm_add_window(xcwm_window_t *window)
{
    _xcwm_window_node *new_node;
    
    /* Create node to hold the new window */
    new_node = malloc(sizeof(_xcwm_window_node));
    if (!new_node) {
        exit(1);
    }
    new_node->window = window;
    
    /* Handle the case where this is the first node added */
    if (!_xcwm_window_list_head) {
        new_node->prev = NULL;
        new_node->next = NULL;
        _xcwm_window_list_head = new_node;
    } else { 
        /* Add the new node to the beginning of the list */
        new_node->next = _xcwm_window_list_head;
        _xcwm_window_list_head->prev = new_node;
        new_node->prev = NULL;
        _xcwm_window_list_head = new_node;
        
    }
    return new_node->window;
}       

xcwm_window_t *
_xcwm_get_window_node_by_window_id (xcb_window_t window_id)
{
    _xcwm_window_node *curr;
        
    curr = _xcwm_window_list_head;
    while (curr) {
        if (curr->window->window_id == window_id) {
            return curr->window;
        }
        curr = curr->next;
    }
    return NULL;
}


void
_xcwm_remove_window_node(xcb_window_t window_id) {

    _xcwm_window_node *curr;
    
    curr = _xcwm_window_list_head;
    while (curr != NULL) {
        if (curr->window->window_id == window_id) {
            // this will be freed in the event_loop
            if(curr->next){
                curr->next->prev = curr->prev;
            }
            if (curr->prev) {
                curr->prev->next = curr->next;
            }
            else{
                _xcwm_window_list_head = curr->next;
            }
                
            free(curr);
            return;
        }
        else
            curr = curr->next;
    }
    return;
}
