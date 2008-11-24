/************************************************************************
 *   Copyright (C) Mark Thomas <markbt@efaref.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef Y_BUFFER_H
#define Y_BUFFER_H

struct Buffer_t;
typedef struct Buffer_t Buffer;

#include <Y/y.h>

#include <stdint.h>
#include <cairo.h>

void buffer_destroy (Buffer *);

void buffer_destroy_all_painters (Buffer *self);

/** \brief Get the runtime type of the buffer.
 */
const char *buffer_get_type (Buffer *self);

void buffer_get_size (Buffer *, int *w_p, int *h_p);
void buffer_set_size (Buffer *, int w, int h);

/** \brief Give buffer a hint that a series of resizes will follow.
 *  \param w Expected largest width of the buffer during resize.
 *  \param h Expected largest height of the buffer during resize.
 *
 *  This call is for optimizing resizing. Gives the buffer a chance to
 *  allocate memory for expected largest size in a series of resizes.
 *  Must be followed by a call to \ref buffer_end_resize. Calls can not be nested.
 */
void buffer_begin_resize (Buffer *self, int w, int h);

/** \brief End a series of resizes.
 *
 * Gives buffers a chance to reallocate after a resizing operation to not waste memory.
 */
void buffer_end_resize (Buffer *self);

/** \brief Obtain a Painter for this buffer.
 */
struct Painter * buffer_get_painter  (Buffer *);

cairo_t * buffer_get_cairo_context  (Buffer *);
cairo_surface_t * buffer_get_cairo_surface (Buffer *);

#endif
