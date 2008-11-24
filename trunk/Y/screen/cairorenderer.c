/************************************************************************
 *   Copyright (C) Simon Persson <simpster@users.sourceforge.net>
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

#include <Y/screen/cairorenderer.h>
#include <Y/screen/rendererclass.h>
#include <Y/util/color.h>
#include <Y/util/yutil.h>

struct CairoRenderer_t
{
  Renderer renderer;
  cairo_t *cairo_context;
};

static void
cairo_renderer_destroy (Renderer *self_r)
{
  CairoRenderer *self = (CairoRenderer *)self_r;
  cairo_destroy (self->cairo_context);
  yfree (self);
}

static void
cairo_renderer_render_buffer (Renderer *self_r, Buffer *buffer,
                        int x, int y, int xo, int yo, int rw, int rh)
{
  CairoRenderer *self = (CairoRenderer *)self_r;
  cairo_set_operator (self->cairo_context, CAIRO_OPERATOR_OVER);
  cairo_set_source_surface (self->cairo_context, buffer_get_cairo_surface (buffer), x, y);
  cairo_rectangle (self->cairo_context, x+xo, y+yo, rw, rh);
  cairo_fill (self->cairo_context);
}

static void
cairo_renderer_copy_buffer (Renderer *self_r, Buffer *buffer,
                              int x, int y, int xo, int yo, int rw, int rh)
{
  CairoRenderer *self = (CairoRenderer *)self_r;
  cairo_set_operator (self->cairo_context, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (self->cairo_context, buffer_get_cairo_surface (buffer), x, y);
  cairo_rectangle (self->cairo_context, x+xo, y+yo, rw, rh);
  cairo_fill (self->cairo_context);
}

static void
cairo_renderer_draw_filled_rectangle (Renderer *self_r, uint32_t colour,
                               int x, int y, int w, int h)
{
  CairoRenderer *self = (CairoRenderer *)self_r;
  cairo_set_operator (self->cairo_context, CAIRO_OPERATOR_OVER);
  YColor c = createColorInt32 (colour);
  cairo_set_source_rgba (self->cairo_context, c.red, c.green, c.blue, c.alpha);
  cairo_rectangle (self->cairo_context, x, y, w, h);
  cairo_fill (self->cairo_context);
}

static void
cairo_renderer_complete (Renderer *self_r)
{
}

static RendererClass cairo_renderer_class =
{
    name:                  "CairoRenderer",
    complete:              cairo_renderer_complete,
    destroy:               cairo_renderer_destroy,
    render_buffer:         cairo_renderer_render_buffer,
    copy_buffer:           cairo_renderer_copy_buffer,
    draw_filled_rectangle: cairo_renderer_draw_filled_rectangle
};

CairoRenderer *
cairo_renderer_create (const struct Rectangle *rect, cairo_surface_t *surface)
{
  CairoRenderer *self = ymalloc (sizeof (CairoRenderer));
  self -> renderer.c = &cairo_renderer_class;
  renderer_initialise (&(self->renderer));
  renderer_enter (&(self->renderer), rect, 0, 0);
  self->cairo_context = cairo_create (surface);
  return self;
}

Renderer *
cairo_renderer_get_renderer (CairoRenderer *self)
{
  return &(self->renderer);
}

