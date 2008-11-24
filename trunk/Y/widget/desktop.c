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

#include <Y/widget/desktop.h>
#include <Y/widget/widget_p.h>

#include <Y/util/yutil.h>
#include <Y/buffer/buffer.h>
#include <Y/buffer/bufferclass.h>
#include <Y/buffer/bufferio.h>

#include <Y/buffer/painter.h>
#include <Y/screen/screen.h>

#include <Y/main/treeinfo.h>

#include <Y/modules/theme.h>

#include <Y/object/class.h>
#include <Y/object/object_p.h>

#include <Y/modules/windowmanager.h>

#include <Y/text/font.h>

#include <Y/util/zorder.h>

#include <stdio.h>

struct Desktop
{
  struct Widget widget;
  struct ZOrder *windows;
  struct Widget *pointerWidget;
  Buffer *background;
};

static int desktopPointerMotion (struct Widget *, int32_t, int32_t, int32_t, int32_t);
static int desktopPointerButton (struct Widget *, int32_t, int32_t, uint32_t, bool);
                                
static void desktopRender (struct Widget *, Renderer *);
static void desktopResize (struct Widget *);

DEFINE_CLASS(Desktop);
#include "Desktop.yc"

/* SUPER
 * Widget
 */

static struct WidgetTable desktopTable =
{
  pointerMotion: desktopPointerMotion,
  pointerButton: desktopPointerButton,
  render:        desktopRender,
  resize:        desktopResize
};

static inline struct Desktop *
castBack (struct Widget *widget)
{
  /* assert ( widget -> c == desktopClass ); */
  return (struct Desktop *)widget;
}

static inline const struct Desktop *
castBackConst (const struct Widget *widget)
{
  /* assert ( widget -> c == desktopClass ); */
  return (const struct Desktop *)widget;
}

static int
windowsKeyFunction (const void *key_v, const void *obj_v)
{
  const int key = *((const int *)key_v);
  const struct Object *obj = obj_v;
  int objKey = objectGetID (obj);
  if (key == objKey)
    return 0;
  if (key < objKey)
    return -1;
  else
    return 1;
}

static int
windowsComparisonFunction (const void *obj1_v, const void *obj2_v)
{
  const struct Object *obj1 = obj1_v;
  const struct Object *obj2 = obj2_v;
  int obj1Key = objectGetID (obj1);
  int obj2Key = objectGetID (obj2);
  if (obj1Key == obj2Key)
    return 0;
  if (obj1Key < obj2Key)
    return -1;
  else
    return 1;
}


void
CLASS_INIT(struct Desktop *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  
  this -> widget.x = 0;
  this -> widget.y = 0;
  this -> widget.w = 1;
  this -> widget.h = 1;

  this -> windows = zorderCreate (windowsKeyFunction, windowsComparisonFunction);

  this -> pointerWidget = NULL;

  //get the path to the dsy_logo
  const char filename[] = "dsy_logo.png";
  size_t l = strlen (yImageDir);
  size_t path_len = l + 1 + sizeof(filename) + 1;
  char path[path_len];
  snprintf(path, path_len, "%s/%s", yImageDir, filename);

    
  this -> background = buffer_load_from_png(path);
}


struct Desktop *
desktopCreate (void)
{
  struct Desktop *self = ymalloc (sizeof (struct Desktop));
  objectInitialise (&(self -> widget.o), CLASS(Desktop));
  CLASS_INIT(self, &desktopTable.vtable);
  return self;
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
desktopDestroy (struct Desktop *self)
{
  zorderDestroy (self -> windows, NULL);
  buffer_destroy (self -> background);
  objectFinalise (desktop_to_object (self));
  yfree (self);
}

struct Widget *
desktopToWidget (struct Desktop *self)
{
  return &(self -> widget);
}

struct Object *
desktop_to_object (struct Desktop *self)
{
  return &(self -> widget.o);
}

int
desktopPointerMotion (struct Widget *self_w, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
  struct Desktop *self = castBack (self_w);
  struct ZOrderIterator *iterator; 
  iterator = zorderGetTopIterator (self -> windows);
  while (zorderiteratorHasValue (iterator)) 
    {
      struct Widget *widget = zorderiteratorGet (iterator);
      int wx, wy;
      widget_get_position_local (widget, &wx, &wy);

      if (widget_contains_point_local (widget, x, y))
        {
          if (widget != self -> pointerWidget)
            {
              if (self -> pointerWidget != NULL)
                widget_pointer_leave (self -> pointerWidget);
              widget_pointer_enter (widget, x, y);
              self -> pointerWidget =  widget;
            }
          if (widget_pointer_motion (widget, x, y, dx, dy))
            {
              zorderiteratorDestroy (iterator);
              return 1;
            }
        }
      zorderiteratorMoveDown (iterator);
    }
  zorderiteratorDestroy (iterator);
 
  if (self -> pointerWidget != NULL)
    {
      widget_pointer_leave (self -> pointerWidget);
      self -> pointerWidget = NULL;
    } 

  return 1;
}

int
desktopPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t b, bool pressed)
{
  struct Desktop *self = castBack (self_w);
  struct ZOrderIterator *iterator; 
  iterator = zorderGetTopIterator (self -> windows);
  while (zorderiteratorHasValue (iterator)) 
    {
      struct Widget *widget = zorderiteratorGet (iterator);
      int wx, wy;
      widget_get_position_local (widget, &wx, &wy);
      if (widget_contains_point_local (widget, x, y))
        {
          if (widget_pointer_button (widget, x, y, b, pressed))
            {
              zorderiteratorDestroy (iterator);
              return 1;
            }
        }
      zorderiteratorMoveDown (iterator);
    }
  zorderiteratorDestroy (iterator);
  

  return 1;
}

void
desktopRender (struct Widget *self_w, Renderer *renderer)
{
  struct Desktop *self = castBack (self_w);
  struct ZOrderIterator *iter;

  /* This should be in paint?
   */
  renderer_draw_filled_rectangle (renderer, 0xFF404080,
                               self -> widget.x, self -> widget.y,
                               self -> widget.w, self -> widget.h);
  //render the background image..
  renderer_render_buffer (renderer, self->background, 0, 0);

  /* render the windows */
  iter = zorderGetBottomIterator (self -> windows);

  while (zorderiteratorHasValue (iter))
    {
      struct Widget *widget = zorderiteratorGet (iter);
      struct Rectangle *widgetRectangle = widget_get_rectangle (widget);
      if (renderer_enter (renderer, widgetRectangle,
                         widgetRectangle->x, widgetRectangle->y))
        {
          widget_render (widget, renderer);
          renderer_leave (renderer);
        }
      rectangleDestroy (widgetRectangle);
      zorderiteratorMoveUp (iter);
    }
  zorderiteratorDestroy (iter);

}

void
desktopResize (struct Widget *self_w)
{
  struct Desktop *self = castBack (self_w);
  struct ZOrderIterator *iter;

  iter = zorderGetBottomIterator (self -> windows);
  while (zorderiteratorHasValue (iter))
    {
      struct Window *win = zorderiteratorGet (iter);
      enum WindowSizeState winState = windowGetSizeState (win);
      struct Rectangle *winRect = widget_get_rectangle (windowToWidget (win));
      struct Rectangle *newRect = rectangleDuplicate (winRect);
      switch (winState)
        {
          case WINDOW_SIZE_NORMAL:
            /* push any off bottom/right windows back on-to the screen,
             * if possible */
            if (newRect -> w > self -> widget.w)
              newRect -> w = self -> widget.w;
            if (newRect -> x + newRect -> w > self -> widget.w)
              newRect -> x = self -> widget.w - newRect -> w;
            if (newRect -> h > self -> widget.h)
              newRect -> h = self -> widget.h;
            if (newRect -> y + newRect -> h > self -> widget.h)
              newRect -> y = self -> widget.h - newRect -> h;
            if (winRect -> x != newRect -> x || winRect -> y != newRect -> y)
              widget_move (windowToWidget (win), newRect -> x, newRect -> y);
            if (winRect -> w != newRect -> w || winRect -> h != newRect -> h)
              widget_resize (windowToWidget (win), newRect -> w, newRect -> h);
            break;
          case WINDOW_SIZE_MAXIMISE:
            /* just resize it */
            widget_resize (windowToWidget (win), self -> widget.w, self -> widget.h);
            break;
        }
      rectangleDestroy (winRect);
      rectangleDestroy (newRect);
      zorderiteratorMoveUp (iter);
    }
  zorderiteratorDestroy (iter);
}

void
desktopAddWindow (struct Desktop *self, struct Window *win)
{
  zorderAddAtTop (self -> windows, win);
  widget_set_container (windowToWidget (win), desktopToWidget (self));
  wmSelectWindow (win);
  widget_repaint (desktopToWidget (self),
                    widget_get_rectangle (windowToWidget (win)));
}

void
desktopRaiseWindow (struct Desktop *self, struct Window *win)
{
  uint32_t id = objectGetID (window_to_object (win));
  struct ZOrderIterator *iter = zorderGetTopIterator (self -> windows);
  struct Object *top = zorderiteratorGet (iter);
  if (top != NULL && objectGetID (top) != id)
    {
      zorderMoveToTop (self -> windows, &id);
      widget_repaint (desktopToWidget (self),
                        widget_get_rectangle (windowToWidget (win)));
    }
  zorderiteratorDestroy (iter);
}

void
desktopRemoveWindow (struct Desktop *self, struct Window *win)
{
  int id = objectGetID (window_to_object (win));
  zorderRemove (self -> windows, &id);
  if (self -> pointerWidget == windowToWidget (win))
    self -> pointerWidget = NULL;
  struct Window *w = zorderGetTop (self -> windows);
  if (w != NULL)
    wmSelectWindow (w);
  widget_rerender (desktopToWidget (self),
                  widget_get_rectangle (windowToWidget (win)));
}

void
desktopCycleWindows (struct Desktop *self, int direction)
{
  struct Window *win;
  if (direction == 1)
    {
      win = zorderGetTop (self -> windows);
      if (win == NULL)
        return;
      zorderMoveToBottom (self -> windows, win);
      widget_rerender (windowToWidget (win), NULL);
    }
  else
    {
      win = zorderGetBottom (self -> windows);
      if (win == NULL)
        return;
      zorderMoveToTop (self -> windows, win);
      widget_rerender (windowToWidget (win), NULL);
    }
  win = zorderGetTop (self -> windows);
  if (win != NULL)
    wmSelectWindow (win);
  widget_rerender (windowToWidget (win), NULL);
}

/* arch-tag: c944bbcb-243c-491e-af68-1953384e05ac
 */
