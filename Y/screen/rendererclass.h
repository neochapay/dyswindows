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

#ifndef Y_SCREEN_RENDERERCLASS_H
#define Y_SCREEN_RENDERERCLASS_H

#include <Y/screen/renderer.h>
#include <Y/util/llist.h>
#include <Y/util/index.h>

/* This defines the internal characteristics of an ABSTRACT Renderer */

typedef struct
{
  const char *name; /* may be used for rtti */
  void (*complete)     (Renderer *);
  void (*destroy)      (Renderer *);
  void (*render_buffer) (Renderer *, Buffer *, int, int,
                        int, int, int, int);
  void (*copy_buffer) (Renderer *, Buffer *, int, int,
                         int, int, int, int);
  void (*draw_filled_rectangle) (Renderer *, uint32_t,
                               int, int, int, int);
} RendererClass;

struct Renderer_t
{
  RendererClass *c;
  struct llist *regions;
  struct Index *options;
};

void renderer_initialise (Renderer *);

#endif /* Y_BUFFER_RENDERERCLASS_H */
