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
 *   CONTRIBUTORS:
 *        Dustin Norlander (dustinn@gmail.com)
 *
 */

#include <Y/buffer/painter.h>
#include <Y/buffer/bufferclass.h>

#include <Y/util/yutil.h>

#include <string.h>


struct Painter *
painter_create ()
{
  struct Painter *self = ymalloc (sizeof (struct Painter));
  /*  if (!pthread_mutex_init (self->mutex, NULL )) {
    Y_TRACE ("Error initializing MUTEX!");
  }
  */
  self -> state = ymalloc (sizeof (struct PainterState));
  self -> buffer = NULL;
  self -> state -> x_origin = 0;
  self -> state -> y_origin = 0;
  self -> state -> clipping = 0;
  self -> state -> clip_rectangle.x = 0xdeadbeef;
  self -> state -> clip_rectangle.y = 0x0a57beef;
  self -> state -> clip_rectangle.w = 0xc01dbeef;
  self -> state -> clip_rectangle.h = 0xc0edbeef;
  self -> state -> previous = NULL;
  self -> state -> widget = NULL;
  return self;
}

void
painter_destroy (struct Painter *self)
{
  struct PainterState *state = self -> state;
  while (state != NULL)
    {
      struct PainterState *prev = state -> previous;
      yfree (state);
      state = prev;
    }
  //now remove painter from the buffer list
  llist_delete_data(self->buffer->painters, self);
  cairo_destroy(self->cairo_context);

  //destroy the mutex
  //  pthread_mutex_destroy(self->mutex);
  yfree (self);
}

void
painter_save_state (struct Painter *self)
{
  struct PainterState *newState = ymalloc (sizeof (struct PainterState));
  memcpy (newState, self -> state, sizeof (struct PainterState));
  newState -> previous = self -> state;
  self -> state = newState;
  //now to save the cairo context state
  cairo_save(self->cairo_context);
}

void
painter_restore_state (struct Painter *self)
{
  if (self -> state -> previous != NULL)
    {
      struct PainterState *oldState = self -> state;
      self -> state = self -> state -> previous;
      yfree (oldState);
    }
  else
    Y_TRACE ("Painter State Stack Underflow");

  //restore cairo state..
  cairo_restore(self->cairo_context);

}

/*
 * This just gets the buffer coordinates from 
 * the widget coordinates.
 */
void 
painter_translate_xy (struct Painter *self, int *x_p, int *y_p)
{
  if (x_p != NULL)
    *x_p += self -> state -> x_origin;
  if (y_p != NULL)
    *y_p += self -> state -> y_origin;
}

void 
painter_translate_double_xy (struct Painter *self, double *x_p, double *y_p)
{
  if (x_p != NULL)
    *x_p += self -> state -> x_origin;
  if (y_p != NULL)
    *y_p += self -> state -> y_origin;
}

/*
 * Clips the current painter based on buffer coordinates.
 */
void
painter_clip_buffer (struct Painter *self, struct Rectangle *rect)
{
  if (self -> state -> clipping)
    {
      if (rectangleIntersect (&(self -> state -> clip_rectangle),
                              &(self -> state -> clip_rectangle), rect) == 0)
        {
          self -> state -> clip_rectangle.w = 0;
          self -> state -> clip_rectangle.h = 0;
        }
    } 
  else
    {
      self -> state -> clipping = 1;
      self -> state -> clip_rectangle.x = rect -> x;
      self -> state -> clip_rectangle.y = rect -> y;
      self -> state -> clip_rectangle.w = rect -> w;
      self -> state -> clip_rectangle.h = rect -> h;
    }
  //clip the cairo_context..
  cairo_rectangle(self->cairo_context, 
		  rect -> x,
		  rect -> y,
		  rect -> w, 
		  rect -> h);

  cairo_clip(self->cairo_context);    

}

/* 
 * This clips the painter to the rectangle.
 * We use the painter clipping initailly 
 * instead of completely relying on cairo
 * in order to have fast access to whether a buffer is fully clipped.
 * 
 * This clips the painter based on its current coordinate system 
 */
void
painter_clip_local (struct Painter *self, struct Rectangle *rect)
{
  struct Rectangle rectT = { rect->x, rect->y, rect->w, rect->h };
  painter_translate_xy (self, &rectT.x, &rectT.y);
  painter_clip_buffer (self, &rectT);
}

/*
 * Sets the coordinate system relative to the 
 * current coordinate system
 */
void
painter_set_origin_local (struct Painter *self, 
				  int x, int y)				
{
  if (!self)
    return;
  self -> state -> x_origin += x;
  self -> state -> y_origin += y;
}


/*
 * Sets the coordinate system relative to the 
 * current buffer
 */
void
painter_set_origin_buffer (struct Painter *self, 
			    int x, int y)
{
  if (!self)
    return;
  self -> state -> x_origin = x;
  self -> state -> y_origin = y;
}

struct Rectangle *
painter_get_clip_rectangle_local (struct Painter *self)
{
  struct Rectangle *r = rectangleDuplicate (&(self -> state -> clip_rectangle));
  r -> x -= self -> state -> x_origin;
  r -> y -= self -> state -> y_origin;
  //if for some reason the clip region is outside the current
  //origin, we just return 0.
  if (r -> x < 0)
    r -> x = 0;
  if (r -> y < 0)
    r -> y = 0;
  return r;
}


struct Rectangle *
painter_get_clip_rectangle_buffer (struct Painter *self)
{
  struct Rectangle *r = rectangleDuplicate (&(self -> state -> clip_rectangle));
  return r;
}

int
painter_is_fully_clipped (struct Painter *self)
{
  return self -> state -> clipping &&
    (self -> state -> clip_rectangle.w == 0 ||
     self -> state -> clip_rectangle.h == 0);
}
