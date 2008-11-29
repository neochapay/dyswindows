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

#include <Y/object/class.h>
#include <Y/widget/widget_p.h>
#include <Y/screen/screen.h>
#include <stdlib.h>
#include <Y/object/class.h>


struct Widget *castBack (struct Object *);

DEFINE_CLASS(Widget);
#include "Widget.yc"

/* SUPER
 * Object
 */

/* PROPERTY
 * enabled :: ybool
 */
 
void
CLASS_INIT (struct Widget *this, VTable *t)
{
  SUPER_INIT(this, t);
  this -> tab = (struct WidgetTable *)t;
  this -> parent = NULL;
  this -> container = NULL;
  this -> state = WIDGET_STATE_NORMAL;
  this -> x = 0;
  this -> y = 0;
  this -> w = 0;
  this -> h = 0;
  this -> minWidth = -1;
  this -> minHeight = -1;
  this -> reqWidth = -1;
  this -> reqHeight = -1;
  this -> maxWidth = -1;
  this -> maxHeight = -1;
  setProperty ( this, enabled, 1 );//widget is enabled by default
}

void
widgetFinalise (struct Widget *self)
{
  ykbUnsetFocus (self);
  widget_unpack (self -> container, self);
}

struct Object *
widget_to_object (struct Widget *self)
{
  return &(self -> o);
}

struct Widget *
castBack (struct Object *object)
{
  /* assert ( object -> c == widgetClass ); */
  return (struct Widget *)object;
}

struct Widget *
object_to_widget (struct Object *object)
{
  return castBack(object);
}

void
widget_global_to_local (const struct Widget *self, int *x_p, int *y_p)
{
  if (self->container != NULL)
    widget_global_to_local (self->container, x_p, y_p); 
  if (x_p != NULL)
    *x_p -= self -> x;
  if (y_p != NULL)
    *y_p -= self -> y;
}

void
widget_local_to_global (const struct Widget *self, int *x_p, int *y_p)
{
  if (x_p != NULL)
    *x_p += self -> x;
  if (y_p != NULL)
    *y_p += self -> y;
  if (self->container != NULL)
    widget_local_to_global (self->container, x_p, y_p);
}

void
widget_local_to_buffer (const struct Widget *self, int *x_p, int *y_p)
{
  if (self->container == NULL)
    return; //this is the top level container (window);
  if (x_p != NULL)
    *x_p += self -> x;
  if (y_p != NULL)
    *y_p += self -> y;
  widget_local_to_buffer (self->container, x_p, y_p);
}

struct Rectangle *
widget_get_rectangle (const struct Widget *self)
{
  return rectangleCreate (self -> x, self -> y, self -> w, self -> h);
}

/*
 * Gets the rectangle of this widget in buffer coordinates
 */
struct Rectangle *
widget_get_rectangle_buffer (const struct Widget *self)
{
  int w_x;
  int w_y;
  widget_get_position_buffer (self, &w_x, &w_y);
  return rectangleCreate (w_x, w_y, self -> w, self -> h);
}

void
widget_get_position_local (const struct Widget *self, int32_t *x_p, int32_t *y_p)
{
  if (self == NULL)
    return;
  if (x_p != NULL)
    *x_p = self -> x;
  if (y_p != NULL)
    *y_p = self -> y;
}

/*
 * Gets the position of this widget in buffer coordinates
 */ 
void
widget_get_position_buffer (const struct Widget *self, int32_t *x_p, int32_t *y_p)
{
  if (self == NULL || x_p == NULL || y_p == NULL)
    return;
  *x_p = 0;
  *y_p = 0;
  widget_local_to_buffer(self, x_p, x_p);
}

void
widget_get_size (const struct Widget *self, int32_t *w_p, int32_t *h_p)
{
  if (self == NULL)
    return;
  if (w_p != NULL)
    *w_p = self -> w;
  if (h_p != NULL)
    *h_p = self -> h;
}

void
widget_get_constraints (const struct Widget *self,
                      int32_t *minWidth_p, int32_t *minHeight_p,
                      int32_t *reqWidth_p, int32_t *reqHeight_p,
                      int32_t *maxWidth_p, int32_t *maxHeight_p)
{
  if (self == NULL)
    return;
  if (minWidth_p != NULL)
    *minWidth_p = self -> minWidth;
  if (minHeight_p != NULL)
    *minHeight_p = self -> minHeight;
  if (reqWidth_p != NULL)
    *reqWidth_p = self -> reqWidth;
  if (reqHeight_p != NULL)
    *reqHeight_p = self -> reqHeight;
  if (maxWidth_p != NULL)
    *maxWidth_p = self -> maxWidth;
  if (maxHeight_p != NULL)
    *maxHeight_p = self -> maxHeight;
}

enum WidgetState
widget_get_state (const struct Widget *self)
{
    return self->state;
}

/*
 * Tests whether the widget contains the given point in buffer coordinates
 */
bool widget_contains_point_buffer (const struct Widget *self, int32_t x, int32_t y)
{
  if (self == NULL)
    return false;
  int w_x = 0;
  int w_y = 0;
  bool ret_val = false;
  struct Rectangle *rect = widget_get_rectangle_buffer (self);
  if (w_x > rect -> x 
      && w_x < (rect -> x + rect -> w)
      && w_y > rect -> y
      && w_y < (rect -> y + rect -> h)) {
    ret_val = true;
  }
  rectangleDestroy(rect);
  return ret_val;
}

bool
widget_contains_point_local (const struct Widget *self, int32_t x, int32_t y)
{
  if (self == NULL)
    return false;
  bool ret_val = false;
  struct Rectangle *rect = widget_get_rectangle (self);
  if (x > rect -> x 
      && x < (rect -> x + rect -> w)
      && y > rect -> y
      && y < (rect -> y + rect -> h)) {
    ret_val = true;
  }
  rectangleDestroy(rect);
  return ret_val;
}

struct Window *
widget_get_window (struct Widget *self)
{
  if (self == NULL)
    return NULL;
  else if (self -> tab -> getWindow != NULL)
    return self -> tab -> getWindow (self);
  else if (self -> container != NULL)
    return widget_get_window (self -> container);
  else
    return NULL;
}

void
widget_move (struct Widget *self, int32_t x, int32_t y)
{
  widget_rerender (self, NULL);
  self -> x = x;
  self -> y = y;
  widget_rerender (self, NULL);
}

void
widget_reconfigure (struct Widget *self)
{
  if (self == NULL)
    return;
  if (self -> tab -> reconfigure != NULL)
    self -> tab -> reconfigure (self);
  else
    widget_reconfigure (self -> container);
}

void
widget_resize (struct Widget *self, int32_t w, int32_t h)
{
  widget_rerender (self, NULL);
  if (w < 0)
    w = 0;
  if (h < 0)
    h = 0;
  self -> w = w;
  self -> h = h;
  if (self -> minWidth != -1 && self -> w < self -> minWidth)
    self -> w = self -> minWidth;
  if (self -> minHeight != -1 && self -> h < self -> minHeight)
    self -> h = self -> minHeight;
  if (self -> maxWidth != -1 && self -> w > self -> maxWidth)
    self -> w = self -> maxWidth;
  if (self -> maxHeight != -1 && self -> h > self -> maxHeight)
    self -> h = self -> maxHeight;
  if (self -> tab -> resize != NULL)
    self -> tab -> resize (self);
  widget_rerender (self, NULL);
}

void
widget_set_container (struct Widget *self, struct Widget *container)
{
  self -> container = container;
  if (container != NULL)
    widget_reconfigure (self);
}

void
widget_unpack (struct Widget *self, struct Widget *w)
{
  if (self != NULL && self -> tab -> unpack != NULL)
    {
      self -> tab -> unpack (self, w);
    }
}

void
widget_render (struct Widget *self, Renderer *renderer)
{
  if (self != NULL && self -> tab -> render != NULL)
    self -> tab -> render (self, renderer);
}

void
widget_paint (struct Widget *self, struct Painter *painter)
{
  if (self != NULL && self -> tab -> paint != NULL) {
    painter_save_state(painter);
    painter_set_origin_local (painter, self -> x, self -> y);
    self -> tab -> paint (self, painter);
    painter_restore_state(painter);
  }
}

void
widget_rerender (struct Widget *self, struct Rectangle *rect)
{
  if (self == NULL)
    return;
  if (rect == NULL)
    rect = rectangleCreate (0, 0, self -> w, self -> h);
  rect -> x += self -> x;
  rect -> y += self -> y;
  if (self -> container != NULL)
    widget_rerender (self -> container, rect);
  else if (screenGetRootWidget () == self)
    screenInvalidateRectangle (rect);
  else
    rectangleDestroy (rect);
}

void
widget_repaint (struct Widget *self, struct Rectangle *rect)
{
  if (self == NULL)
    return;
  if (rect == NULL)
    rect = rectangleCreate (0, 0, self -> w, self -> h);
  if (self -> tab -> repaint != NULL)
    self -> tab -> repaint (self, rect);
  else if (self -> container != NULL)
    {
      rect -> x += self -> x;
      rect -> y += self -> y;
      widget_repaint (self -> container, rect);
    }
  else
    rectangleDestroy (rect);
}

int
widget_pointer_motion (struct Widget *self, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
  if (self == NULL || self -> tab -> pointerMotion == NULL)
    return 0;
  return self -> tab -> pointerMotion (self, x, y, dx, dy);
}

int
widget_pointer_button (struct Widget *self, int32_t x, int32_t y, uint32_t b, bool pressed)
{
  if (self == NULL || self -> tab -> pointerButton == NULL)
    return 0;
  return self -> tab -> pointerButton (self, x, y, b, pressed);
}

void
widget_pointer_enter (struct Widget *self, int32_t x, int32_t y)
{
  if (self != NULL && self -> tab -> pointerEnter != NULL)
    self -> tab -> pointerEnter (self, x, y);
}

void
widget_pointer_leave (struct Widget *self)
{
  if (self != NULL && self -> tab -> pointerLeave != NULL)
    self -> tab -> pointerLeave (self);
}

void
widget_ykb_string(struct Widget *self, const char *str, uint16_t modifiers)
{
  if (self && self->tab->ykbString)
    self->tab->ykbString(self, str, modifiers);
}

void
widget_ykb_event(struct Widget *self, const char *event, uint16_t modifiers)
{
  if (self && self->tab->ykbEvent)
    self->tab->ykbEvent(self, event, modifiers);
}

void
widget_ykb_stroke(struct Widget *self, bool direction, uint16_t keycode, uint16_t modifiers)
{
  if (self && self->tab->ykbStroke)
    self->tab->ykbStroke(self, direction, keycode, modifiers);
}

bool
widget_ykb_get_cursor(struct Widget *self, int32_t *x, int32_t *y)
{
  if (self && self->tab->ykbGetCursor)
    return self->tab->ykbGetCursor(self, x, y);
  else
    return false;
}

/* PROPERTY HOOK
 * enabled
 */
static void
widget_enabled_set (struct Widget *self)
{ 
  
  int e = safeGetProperty(self, enabled, 1);
  if (e)
    self -> state = WIDGET_STATE_NORMAL;
  else
    self -> state = WIDGET_STATE_DISABLED;
//  widget_repaint (self, NULL); //can't call until widget is drawn the first time
}

