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

#ifndef Y_BUFFERCLASS_H
#define Y_BUFFERCLASS_H

#include <Y/y.h>
#include <Y/buffer/buffer.h>
#include <Y/util/index.h>
#include <Y/util/llist.h>

#include <stdint.h>
#include <stdbool.h>

#include <cairo.h>

/*******************************************************************************
 *                              GLOBAL DATATYPES
 ******************************************************************************/
typedef struct
{
  const char * name;
  void (*destroy)      (Buffer *);
  void (*set_size)     (Buffer *, int w, int h);
  void (*begin_resize) (Buffer *, int w, int h);
  void (*end_resize)   (Buffer *);
} BufferClass;

struct Buffer_t
{
  BufferClass *c;
  int width;			//width of the pixelbuffer (in pixels)
  int height;			//height of the pixelbuffer (in pixels)
  cairo_surface_t *surface; //this is the cairo surface..
  cairo_format_t format; //pixel format of this buffer

  //think we need to keep a running list of all painters 
  //in order to make sure that the painter and the buffer remain
  //in a consistent state..
  struct llist *painters;       //this holds all the currently active painters
};

void buffer_init (Buffer *self, BufferClass *c, cairo_format_t buffer_format);

void buffer_finalise (Buffer *self);


/** \brief return number of bits required for a buffer format
 */
uint32_t buffer_get_depth (cairo_format_t buffer_format);

#endif
