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

#ifndef Y_SCREEN_RENDERER_H
#define Y_SCREEN_RENDERER_H

struct Renderer_t;
typedef struct Renderer_t Renderer;

#include <Y/y.h>
#include <Y/util/rectangle.h>
#include <Y/buffer/buffer.h>
#include <stdint.h>

/* This represents an ABSTRACT renderer.
 *
 * A Renderer is a "visitor" in the Visitor pattern.  A specific
 * implementation if visitor is selected by the video driver, and
 * is then passed over the Widget hierarchy.  The renderer provides
 * the interface by which Buffer-to-Video transfers may occur.
 */

/* Signal that all processing for this renderer is complete. Renderers
 * that buffer information should flush the buffer here.
 */
void renderer_complete (Renderer *);

/* Destroy the renderer.  If the renderer has not been "Complete"d, then
 * the state of the display for the renderer's rectangle is undefined.
 */
void renderer_destroy (Renderer *);

/* Attempt to enter the region (X, Y, W, H).
 * Translate all further coordinates by DX and DY (until the corresponding
 * Leave).
 * Entering succeeds if there is a non-zero resulting region.
 *
 * Returns 1 if entering succeeds.
 */
int renderer_enter (Renderer *, const struct Rectangle *rect,
                   int dx, int dy);

/* Leave the last region enterered.
 */
void renderer_leave (Renderer *);

/* \brief Render the buffer to the given co-ordinates.
 * The buffer will be alpha-blended onto the renderer's surface.
 */
void renderer_render_buffer (Renderer *, Buffer *, int x, int y);

/* \brief Copy the buffer to the given co-ordinates.
 */
void renderer_copy_buffer (Renderer *, Buffer *, int x, int y);

/* Draw a rectangle filled with a solid colour, COLOUR. The rectangle should
 * placed with the upper-left corner at the device independent co-ordinates
 * (X,Y), and be W by H in size.
 */
void renderer_draw_filled_rectangle (Renderer *, uint32_t colour,
                                  int x, int y, int w, int h);

/* Stash string key/value pairs in the renderer
 */
void renderer_set_option (Renderer *self, const char *key, const char *value);
const char *renderer_get_option (Renderer *self, const char *key);

#endif /* Y_BUFFER_RENDERER_H */

