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

#ifndef Y_SCREEN_CAIRORENDERER_H
#define Y_SCREEN_CAIRORENDERER_H

#include <Y/screen/renderer.h>
#include <Y/util/rectangle.h>
#include <cairo.h>

typedef struct CairoRenderer_t CairoRenderer;

CairoRenderer *cairo_renderer_create (const struct Rectangle *, cairo_surface_t *surface);
Renderer *cairo_renderer_get_renderer (CairoRenderer *self);

#endif
