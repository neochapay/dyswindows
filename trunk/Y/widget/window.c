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

#include <Y/widget/window.h>
#include <Y/widget/widget_p.h>

#include <Y/util/yutil.h>
#include <Y/util/llist.h>
#include <Y/buffer/buffer.h>
#include <Y/buffer/painter.h>
#include <Y/screen/screen.h>

#include <Y/object/class.h>
#include <Y/object/object_p.h>

#include <Y/modules/windowmanager.h>
#include <Y/modules/theme.h>

#include <Y/text/font.h>

#include <stdio.h>
#include <ctype.h>

struct Window
{
  struct Widget widget;
  Buffer *buffer;
  struct Widget *child;
  struct Widget *focus;
  enum WindowSizeState sizeState;
  int pointerInChild : 1;
  int storeX, storeY, storeW, storeH;
  int dragging, dragX, dragY;
  struct llist *invalidRectangles;
};

enum WindowAnchor
{
  WINDOW_ANCHOR_NONE   = 0,
  WINDOW_ANCHOR_LEFT   = 1<<0,
  WINDOW_ANCHOR_RIGHT  = 1<<1,
  WINDOW_ANCHOR_TOP    = 1<<2,
  WINDOW_ANCHOR_BOTTOM = 1<<3
};

static int windowPointerMotion (struct Widget *, int32_t, int32_t, int32_t, int32_t);
static int windowPointerButton (struct Widget *, int32_t, int32_t, uint32_t, bool);
static void windowPointerEnter (struct Widget *, int32_t x, int32_t y);
static void windowPointerLeave (struct Widget *);
static struct Window *windowGetWindow (struct Widget *);
static void windowRender (struct Widget *, Renderer *);
static void windowUnpack (struct Widget *, struct Widget *);
static void windowPaint (struct Widget *, struct Painter *);
static void windowRepaint (struct Widget *, struct Rectangle *rect);
static void windowResize (struct Widget *);
static void windowReconfigure (struct Widget *);

DEFINE_CLASS(Window);
#include "Window.yc"

/* SUPER
 * Widget
 */

/* PROPERTY
 * title :: string
 * background :: uint32
 */

static struct WidgetTable windowTable =
{
  getWindow:     windowGetWindow,
  unpack:        windowUnpack,
  pointerMotion: windowPointerMotion,
  pointerButton: windowPointerButton,
  pointerEnter:  windowPointerEnter,
  pointerLeave:  windowPointerLeave,
  render:        windowRender,
  paint:         windowPaint,
  repaint:       windowRepaint,
  reconfigure:   windowReconfigure,
  resize:        windowResize,
};

void
CLASS_INIT (struct Window *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  this -> child = NULL;
  this -> focus = NULL;
  this -> widget.x = (this -> widget.o.oid % 15) * 24;
  this -> widget.y = (this -> widget.o.oid % 10) * 24;
  this -> widget.w = 64;
  this -> widget.h = 48;
  this -> sizeState = WINDOW_SIZE_NORMAL;
  this -> pointerInChild = 0;
  this -> dragging = 0;
  this -> invalidRectangles = new_llist ();
  this -> buffer = screen_get_new_buffer (this -> widget.w, this -> widget.h,
                                          CAIRO_FORMAT_ARGB32); //create the buffer

  windowSaveGeometry (this);
  themeWindowInit (this);

  widget_reconfigure (&(this -> widget));
}

static inline struct Window *
castBack (struct Widget *widget)
{
  /* assert ( widget -> c == windowClass ); */
  return (struct Window *)widget;
}

static inline const struct Window *
castBackConst (const struct Widget *widget)
{
  /* assert ( widget -> c == windowClass ); */
  return (const struct Window *)widget;
}

static void
windowUnpack (struct Widget *self_w, struct Widget *w)
{
  struct Window *self = castBack (self_w);
  if (w == self -> child)
    {
      widget_repaint (w, NULL);
      widget_set_container (w, NULL);
      self -> child = NULL;
    }
}

static struct Window *
windowGetWindow (struct Widget *self_w)
{
  struct Window *self = castBack (self_w);
  return self;
}

static void
windowPaint (struct Widget *self_w, struct Painter *painter)
{
  struct Window *self = castBack (self_w);
  themeWindowPaint (self, painter);
}

static void
windowRepaint (struct Widget *self_w, struct Rectangle *rect)
{
  struct Window *self = castBack (self_w);
  llist_add_tail (self -> invalidRectangles, rect);
  widget_rerender (windowToWidget (self), rectangleDuplicate (rect));
}

/* PROPERTY HOOK
 * title
 */
static void
windowTitleSet (struct Window *self)
{
  widget_repaint (windowToWidget (self), NULL);
}

static struct Window *
windowCreate (void)
{
  struct Window *self = ymalloc (sizeof (struct Window));
  objectInitialise (&(self -> widget.o), CLASS(Window));
  CLASS_INIT(self, &windowTable.vtable);
  return self;
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
windowDestroy (struct Window *self)
{
  wmUnregisterWindow (self);
  if (self -> child != NULL)
    widget_set_container (self -> child, NULL);
  llist_destroy (self -> invalidRectangles, rectangleDestroy);
  buffer_destroy (self->buffer);
  widgetFinalise (windowToWidget (self));
  objectFinalise (window_to_object (self));
  yfree (self);
}

struct Widget *
windowToWidget (struct Window *self)
{
  return &(self -> widget);
}

struct Object *
window_to_object (struct Window *self)
{
  return &(self -> widget.o);
}

struct Widget *
windowGetChild (struct Window *self)
{
  if (self == NULL)
    return NULL;
  return self -> child;
}

struct Widget *
windowGetFocussedWidget (struct Window *self)
{
  if (self == NULL)
    return NULL;
  return self -> focus;
}

void
windowSetFocussedWidget (struct Window *self, struct Widget *f)
{
  if (self == NULL)
    return;
  if (f == self -> focus)
    return;
  if (self -> focus != NULL)
    {
      widget_repaint (self -> focus, NULL);
      if (wmSelectedWindow () == self)
        ykbUnsetFocus (self -> focus);
    }
  self -> focus = f;
  if (self -> focus != NULL)
    {
      widget_repaint (self -> focus, NULL);
      if (wmSelectedWindow () == self)
        ykbSetFocus (self -> focus);
    }
}

void
windowStartReshape (struct Window *self, int xHandle, int yHandle,
                    enum WindowReshapeMode mode)
{
  if (self -> sizeState != WINDOW_SIZE_NORMAL)
    return;
  self -> dragging = mode;
  if (mode != WINDOW_RESHAPE_NONE)
    {
      self -> dragX = xHandle;
      self -> dragY = yHandle;
      buffer_begin_resize (self->buffer, 800, 600);
      pointerGrab (windowToWidget (self));
    }
}

void
windowStopReshape (struct Window *self)
{
  self -> dragging = WINDOW_RESHAPE_NONE;
  pointerRelease ();
  buffer_end_resize (self->buffer);
}

void
windowSetSizeState (struct Window *self, enum WindowSizeState state)
{
  if (self -> sizeState != state)
    {
      self -> sizeState = state;
      widget_repaint (windowToWidget (self), NULL);
    }
}

enum WindowSizeState
windowGetSizeState (struct Window *self)
{
  return self -> sizeState;
}

int
windowPointerMotion (struct Widget *self_w, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
  struct Window *self = castBack (self_w);


  if (self -> dragging == WINDOW_RESHAPE_NONE
      && !widget_contains_point_local (self_w, x, y))
    return 0;
  if (self -> dragging != WINDOW_RESHAPE_NONE)
    {
      int32_t mdx, mdy;

      mdx = x - self -> dragX;
      mdy = y - self -> dragY;

      switch (self -> dragging)
        {
        case WINDOW_RESHAPE_MOVE:
          widget_move (self_w, self_w -> x + mdx, self_w -> y + mdy);
          break;
        case WINDOW_RESHAPE_RESIZE_NW:
          if (self_w -> w - mdx < self_w -> minWidth)
            mdx = self_w -> w - self_w -> minWidth;
          if (self_w -> h - mdy < self_w -> minHeight)
            mdy = self_w -> h - self_w -> minHeight;
          widget_move (self_w, self_w -> x + mdx,  self_w -> y + mdy);
          widget_resize (self_w, self_w -> w - mdx,  self_w -> h - mdy);
          break;
        case WINDOW_RESHAPE_RESIZE_N:
          if (self_w -> h - mdy < self_w -> minHeight)
            mdy = self_w -> h - self_w -> minHeight;
          widget_move (self_w, self_w -> x,  self_w -> y + mdy);
          widget_resize (self_w, self_w -> w,  self_w -> h - mdy);
          break;
        case WINDOW_RESHAPE_RESIZE_NE:
          if (self_w -> w + mdx < self_w -> minWidth)
            mdx = self_w -> minWidth - self_w -> w;
          if (self_w -> h - mdy < self_w -> minHeight)
            mdy = self_w -> h - self_w -> minHeight;
          widget_move (self_w, self_w -> x,  self_w -> y + mdy);
          widget_resize (self_w, self_w -> w + mdx,  self_w -> h - mdy);
          self -> dragX += mdx;
          break;
        case WINDOW_RESHAPE_RESIZE_W:
          if (self_w -> w - mdx < self_w -> minWidth)
            mdx = self_w -> w - self_w -> minWidth;
          widget_move (self_w, self_w -> x + mdx,  self_w -> y);
          widget_resize (self_w, self_w -> w - mdx,  self_w -> h);
          break;
        case WINDOW_RESHAPE_RESIZE_E:
          if (self_w -> w + mdx < self_w -> minWidth)
            mdx = self_w -> minWidth - self_w -> w;
          widget_resize (self_w, self_w -> w + mdx,  self_w -> h);
          self -> dragX += mdx;
          break;
        case WINDOW_RESHAPE_RESIZE_SW:
          if (self_w -> w - mdx < self_w -> minWidth)
            mdx = self_w -> w - self_w -> minWidth;
          if (self_w -> h + mdy < self_w -> minHeight)
            mdy = self_w -> minHeight - self_w -> h;
          widget_move (self_w, self_w -> x + mdx,  self_w -> y);
          widget_resize (self_w, self_w -> w - mdx,  self_w -> h + mdy);
          self -> dragY += mdy;
          break;
        case WINDOW_RESHAPE_RESIZE_S:
          if (self_w -> h + mdy < self_w -> minHeight)
            mdy = self_w -> minHeight - self_w -> h;
          widget_resize (self_w, self_w -> w,  self_w -> h + mdy);
          self -> dragY += mdy;
          break;
        case WINDOW_RESHAPE_RESIZE_SE:
          if (self_w -> w + mdx < self_w -> minWidth)
            mdx = self_w -> minWidth - self_w -> w;
          if (self_w -> h + mdy < self_w -> minHeight)
            mdy = self_w -> minHeight - self_w -> h;
          widget_resize (self_w, self_w -> w + mdx,  self_w -> h + mdy);
          self -> dragX += mdx;
          self -> dragY += mdy;
          break;
        }
      return 1;
    }
  if (self -> child != NULL && self -> dragging == WINDOW_RESHAPE_NONE)
    {
      int32_t childX, childY, childW, childH;
      widget_get_position_local (self -> child, &childX, &childY);
      widget_get_size (self -> child, &childW, &childH);

	  /* convert global x,y to window x,y */
	  int32_t xp, yp;
	  xp = x;
	  yp = y;
	  widget_global_to_local (self_w, &xp, &yp);
      if (self -> pointerInChild)
        {
          if (xp < childX || xp >= childX + childW
              || yp < childY || yp >= childY + childH)
            {
              self -> pointerInChild = 0;
              widget_pointer_leave (self -> child);
            }
        }
      else
        {
          if (xp >= childX && xp < childX + childW
              && yp >= childY && yp < childY + childH)
            {
              self -> pointerInChild = 1;
              widget_pointer_enter (self -> child, xp - childX, yp - childY);
            }
        }
    }
  themeWindowPointerMotion (self, x, y, dx, dy);
  return 1;
}

int
windowPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t b, bool pressed)
{
  struct Window *self = castBack (self_w);
  wmWindowPointerButton (self, x, y, b, pressed);
  themeWindowPointerButton (self, x, y, b, pressed);
  return 1;
}

void
windowPointerEnter (struct Widget *self_w, int32_t x, int32_t y)
{
  struct Window *self = castBack (self_w);
  wmWindowPointerEnter (self, x, y);
  if (self -> child != NULL)
    {
      int32_t childX, childY, childW, childH;
      widget_get_position_local (self -> child, &childX, &childY);
      widget_get_size (self -> child, &childW, &childH);
      if (x >= childX && x < childX + childW
          && y >= childY && y < childY + childH)
        {
          self -> pointerInChild = 1;
          widget_pointer_enter (self -> child, x - childX, y - childY);
        }
    }
}

void
windowPointerLeave (struct Widget *self_w)
{
  struct Window *self = castBack (self_w);
  wmWindowPointerLeave (self);
  if (self -> child != NULL && self -> pointerInChild)
    {
      self -> pointerInChild = 0;
      widget_pointer_leave (self -> child);
    }
}

/* METHOD
 * Window :: () -> (object)
 */
static struct Object *
windowInstantiate (void)
{
  return window_to_object (windowCreate ());
}

/* METHOD
 * setChild :: (object) -> ()
 */
void
windowSetChild (struct Window *self, struct Object *obj)
{
  struct Widget *child = (struct Widget *)obj;

  if (child == NULL)
    return;
  self -> child = child;
  self -> widget.reqWidth = -1;
  self -> widget.reqHeight = -1;
  widget_set_container (child, windowToWidget (self));
  widget_repaint (child, NULL);
}

/* METHOD
 * setFocussed :: (object) -> ()
 */
void
windowSetFocussed (struct Window *self, struct Object *obj)
{
  struct Widget *focus = (struct Widget *)obj;

  if (focus == NULL)
    return;
  windowSetFocussedWidget (self, focus);
}

/* METHOD
 * show :: () -> ()
 */
void
windowShow (struct Window *self)
{
  if (self != NULL)
    {
      self -> widget.reqHeight = -1;
      self -> widget.reqWidth = -1;
      wmRegisterWindow (self);
      widget_reconfigure (windowToWidget (self));
      widget_repaint (windowToWidget (self), NULL);
    }
}

void
windowRender (struct Widget *self_w, Renderer *renderer)
{
  struct Window *self = castBack (self_w);
  struct llist_node *node;
  rectanglelistUnionOverlaps (self -> invalidRectangles);
  node = llist_head (self->invalidRectangles);
  while (node != NULL)
    {
      struct llist_node *next_node = llist_node_next (node);
      struct Rectangle *rect = llist_node_data (node);
      struct Painter *painter = buffer_get_painter (self->buffer);
      painter_clip_buffer (painter, rect);
      windowPaint (self_w, painter);
      painter_destroy (painter);
      llist_node_delete (node);
      rectangleDestroy (rect);
      node = next_node;
    }

  renderer_render_buffer (renderer, self->buffer, 0, 0);

  if (self -> child != NULL)
    {
      struct Rectangle *childRect = widget_get_rectangle (self -> child);
      if (renderer_enter (renderer, childRect, childRect->x, childRect->y))
        {
          widget_render (self -> child, renderer);
          renderer_leave (renderer);
        }
      rectangleDestroy (childRect);
    }
}

void
windowReconfigure (struct Widget *self_w)
{
  struct Window *self = castBack (self_w);
  themeWindowReconfigure (self,
                          &(self->widget.minWidth), &(self->widget.minHeight),
                          &(self->widget.reqWidth), &(self->widget.reqHeight),
                          &(self->widget.maxWidth), &(self->widget.maxHeight));
  widget_resize (self_w, self->widget.reqWidth, self->widget.reqHeight);
}

void
windowResize (struct Widget *self_w)
{
  struct Window *self = castBack (self_w);

  buffer_set_size (self->buffer, self_w->w, self_w->h);
  themeWindowResize (self);

  widget_repaint (windowToWidget (self), NULL);

  /* windows like to stay the same size, if they can */
  self -> widget.reqWidth = self -> widget.w;
  self -> widget.reqHeight = self -> widget.h;
}

void
windowRequestClose (struct Window *self)
{
  objectEmitSignal (window_to_object (self), "requestClose");
}

void
windowSaveGeometry (struct Window *self)
{
  self -> storeX = self -> widget.x;
  self -> storeY = self -> widget.y;
  self -> storeW = self -> widget.w;
  self -> storeH = self -> widget.h;
}

void
windowRestoreGeometry (struct Window *self)
{
  widget_move (windowToWidget (self), self -> storeX, self -> storeY);
  widget_resize (windowToWidget (self), self -> storeW, self -> storeH);
}

/* arch-tag: 979f2694-e407-4555-9518-3d8ba5fc512d
 */
