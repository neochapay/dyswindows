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

#ifndef Y_BUFFER_PAINTER_H
#define Y_BUFFER_PAINTER_H

struct Painter;

#include <Y/y.h>
#include <Y/buffer/buffer.h>
#include <Y/util/rectangle.h>
#include <Y/text/font.h>
#include <Y/widget/widget_p.h>

/* This defines an ABSTRACT Painter.
 *
 * Painters are very closely tied to buffers, i.e. a buffer might have
 * several painters associated with it, and painter implementations will
 * usually be defined within the file for a buffer implementation, since
 * it might need access to the internal structures of the buffer.
 */


struct PainterState
{
  int x_origin;
  int y_origin;
  int clipping;
  struct Rectangle clip_rectangle;
  struct PainterState *previous;
  struct Widget *widget;
};

struct Painter
{
  const char *name;
  Buffer *buffer;
  struct PainterState *state;
  cairo_t *cairo_context;
};

struct Painter *painter_create (void);

void     painter_destroy (struct Painter *);

void     painter_save_state (struct Painter *);
void     painter_restore_state (struct Painter *);

int      painter_is_fully_clipped (struct Painter *);

void     painter_translate_xy (struct Painter *self, 
			       int *x_p, int *y_p);
void     painter_translate_double_xy (struct Painter *self, 
				      double *x_p, double *y_p);
void     painter_clip_buffer (struct Painter *, struct Rectangle *);
void     painter_clip_local (struct Painter *, struct Rectangle *);

void     painter_set_widget (struct Painter *self, 
			     struct Widget *widget);

void     painter_set_origin_local (struct Painter *self, 
				     int x, int y);

void     painter_set_origin_buffer (struct Painter *self, 
				     int x, int y);

struct Rectangle * painter_get_clip_rectangle_buffer (struct Painter *);
struct Rectangle * painter_get_clip_rectangle_local (struct Painter *);


#endif /* Y_BUFFER_PAINTER_H */
