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
 *
 * CONTRIBUTORS:
 *    Dustin Norlander (dustinn@gmail.com)
 */

#include <Y/buffer/buffer.h>
#include <Y/buffer/bufferclass.h>
#include <Y/util/llist.h>
#include <Y/util/yutil.h>
#include <Y/buffer/painter.h>

#include <stdlib.h>

void
buffer_init (Buffer *self, BufferClass *c, cairo_format_t buffer_format)
{
  self->c = c;
  self->width = 0;
  self->height = 0;
  self->surface = NULL;
  self->format = buffer_format;
  self->painters = new_llist();
}

void
buffer_finalise (Buffer *self)
{
}

/*
 * This destroys all the painters that are currently
 * registered with this buffer.
 * Care should be taken to avoid state inconsistencies
 * where Painter pointers are still thought to exist.
 */
void
buffer_destroy_all_painters (Buffer *self)
{
  llist_destroy(self->painters, painter_destroy);
  self->painters = new_llist();
}

void
buffer_destroy (Buffer *self)
{
  buffer_destroy_all_painters (self);
  free_llist (self->painters);
  self->c->destroy (self);
}

const char *
buffer_get_type (Buffer *self)
{
  return self->c->name;
}

void
buffer_get_size (Buffer *self, int *w_p, int *h_p)
{
  if (w_p != NULL)
    *w_p = self->width;
  if (h_p != NULL)
    *h_p = self->height;
}

void
buffer_set_size (Buffer *self, int w, int h)
{
  self->c->set_size (self, w, h);
}

void
buffer_begin_resize (Buffer *self, int w, int h)
{
  self->c->begin_resize (self, w, h);
}

void
buffer_end_resize (Buffer *self)
{
  self->c->end_resize (self);
}

struct Painter *
buffer_get_painter (Buffer *self)
{
  struct Painter *painter = painter_create ();
  struct Rectangle r = { 0, 0, self->width, self->height };

  painter->cairo_context = cairo_create (self->surface);
  painter->buffer = self;
  painter_clip_buffer (painter, &r);

  //add the painter to our list.
  llist_add_head(self->painters, painter);
  return painter;
}

cairo_t *
buffer_get_cairo_context (Buffer *self)
{
  return cairo_create (self->surface);
}

cairo_surface_t *
buffer_get_cairo_surface (Buffer *self)
{
  return self->surface;
}

uint32_t
buffer_get_depth (cairo_format_t buffer_format)
{
  uint32_t bpp = 32;
  switch (buffer_format) {
    case (CAIRO_FORMAT_ARGB32):
      bpp = 32;
      break;
    case (CAIRO_FORMAT_RGB24):
      bpp = 24;
      break;
//    case (CAIRO_FORMAT_RGB16_565):
//      bpp = 16;
//      break;
    case (CAIRO_FORMAT_A8):
      bpp = 8;
      break;
    case (CAIRO_FORMAT_A1):
      bpp = 1;
      break;
  }
  return bpp;
}

