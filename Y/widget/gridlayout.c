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

#include <Y/widget/gridlayout.h>
#include <Y/widget/widget_p.h>

#include <Y/util/yutil.h>

#include <Y/buffer/painter.h>

#include <Y/object/class.h>
#include <Y/object/object_p.h>

#include <Y/text/font.h>

#include <stdio.h>
#include <ctype.h>

#ifndef MAX
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#endif

struct GridLayout
{
  struct Widget widget;
  uint32_t rows, cols;
  uint32_t *rowHeights, *colWidths;
  struct llist *items;
  struct Widget *pointerWidget;
};

struct GridItem
{
  struct Widget *widget;
  uint32_t row, col, rowspan, colspan;
};

static void gridlayoutUnpack (struct Widget *, struct Widget *);
static void gridlayoutPaint (struct Widget *, struct Painter *);
static void gridlayoutResize (struct Widget *);
static void gridlayoutReconfigure (struct Widget *);
static int gridlayoutPointerMotion (struct Widget *, int32_t, int32_t, int32_t, int32_t);
static int gridlayoutPointerButton (struct Widget *, int32_t, int32_t, uint32_t, bool);
static void gridlayoutPointerEnter (struct Widget *, int32_t, int32_t);
static void gridlayoutPointerLeave (struct Widget *);

static void gridlayoutFitChildren (struct GridLayout *self);

DEFINE_CLASS(GridLayout);
#include "GridLayout.yc"

/* SUPER
 * Widget
 */

static struct WidgetTable gridlayoutTable =
{
  unpack:        gridlayoutUnpack,
  reconfigure:   gridlayoutReconfigure,
  resize:        gridlayoutResize,
  paint:         gridlayoutPaint,
  pointerMotion: gridlayoutPointerMotion,
  pointerButton: gridlayoutPointerButton,
  pointerEnter:  gridlayoutPointerEnter,
  pointerLeave:  gridlayoutPointerLeave
};

void
CLASS_INIT(struct GridLayout *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  this -> rows = 0;
  this -> cols = 0;
  this -> rowHeights = NULL;
  this -> colWidths = NULL;
  this -> items = new_llist ();
  this -> pointerWidget = NULL;
}


static inline struct GridLayout *
castBack (struct Widget *widget)
{
  /* assert ( widget -> c == gridlayoutClass ); */
  return (struct GridLayout *)widget;
}

static inline const struct GridLayout *
castBackConst (const struct Widget *widget)
{
  /* assert ( widget -> c == gridlayoutClass ); */
  return (const struct GridLayout *)widget;
}

static void
gridlayoutUnpack (struct Widget *self_w, struct Widget *w)
{
  struct GridLayout *self = castBack (self_w);

  for (struct llist_node *node = llist_head (self->items);
       node != NULL;
       node = llist_node_next (node))
    {
      struct GridItem *item = llist_node_data (node);
      if (item -> widget == w)
        {
          llist_node_delete (node);
          yfree (item);
          widget_set_container (w, NULL);
          return;
        }
    }
}

static void
gridlayoutPaint (struct Widget *self_w, struct Painter *painter)
{
  /* draw each child */
  struct GridLayout *self = castBack (self_w);

  Y_TRACE("Painting Grid layout with origin:: %d, %d",
	  painter -> state -> x_origin,
	  painter -> state -> y_origin);

  for (struct llist_node *node = llist_head (self->items);
       node != NULL;
       node = llist_node_next (node))
    {
      struct GridItem *item = llist_node_data (node);
      struct Rectangle *rect = widget_get_rectangle (item->widget);
      if (!painter_is_fully_clipped (painter))
        widget_paint (item -> widget, painter);
      rectangleDestroy (rect);
    }
}

static struct GridLayout *
gridlayoutCreate (void)
{
  struct GridLayout *self = ymalloc (sizeof (struct GridLayout));
  objectInitialise (&(self -> widget.o), CLASS(GridLayout));
  CLASS_INIT(self, &gridlayoutTable.vtable);
  return self;
}

static void
griditemDestroy (struct GridItem *item)
{
  widget_set_container (item -> widget, NULL);
  yfree (item);
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
gridlayoutDestroy (struct GridLayout *self)
{
  llist_destroy (self -> items, griditemDestroy);
  widgetFinalise (gridlayoutToWidget (self));
  objectFinalise (gridlayout_to_object (self));
  yfree(self->rowHeights);
  yfree(self->colWidths);
  yfree (self);
}

struct Widget *
gridlayoutToWidget (struct GridLayout *self)
{
  return &(self -> widget);
}

struct Object *
gridlayout_to_object (struct GridLayout *self)
{
  return &(self -> widget.o);
}

/* METHOD
 * GridLayout :: () -> (object)
 */
static struct Object *
gridlayoutInstantiate (void)
{
  return gridlayout_to_object (gridlayoutCreate ());
}

static void
gridlayoutResizeGrid (struct GridLayout *self, uint32_t cols, uint32_t rows)
{
  self -> cols = cols;
  self -> rows = rows;

  /* check that we aren't resizing to smaller than any child */
  for (struct llist_node *node = llist_head (self->items);
       node != NULL;
       node = llist_node_next (node))
    {
      struct GridItem *item = llist_node_data (node);
      self -> cols = MAX (self -> cols, item -> col + item -> colspan);
      self -> rows = MAX (self -> rows, item -> row + item -> rowspan);
    }

  yfree (self -> rowHeights);
  yfree (self -> colWidths);
  self -> rowHeights = ymalloc (sizeof (int) * self -> rows);
  self -> colWidths  = ymalloc (sizeof (int) * self -> cols);

  gridlayoutFitChildren (self);
}

static int
rowHeight (struct GridLayout *self, uint32_t row, uint32_t span)
{
  uint32_t i, h;
  for (i=0, h=0; i<span; ++i)
    h += self -> rowHeights[row + i];
  return h;
}

static int
colWidth (struct GridLayout *self, uint32_t col, uint32_t span)
{
  uint32_t i, w;
  for (i=0, w=0; i<span; ++i)
    w += self -> colWidths[col + i];
  return w;
}

static void
gridlayoutFitChildren (struct GridLayout *self)
{
  /* homogeneous, for now */
  int cumulativeWidth, cumulativeHeight;

  /* work out column widths and row heights */
  cumulativeWidth = 0;
  for (uint32_t i = 0; i < self->cols; ++i)
    {
      self->colWidths[i] = (self->widget.w - cumulativeWidth) /
                           (self -> cols - i);
      cumulativeWidth += self->colWidths[i];
    }

  cumulativeHeight = 0;
  for (uint32_t j = 0; j < self->rows; ++j)
    {
      self->rowHeights[j] = (self->widget.h - cumulativeHeight) /
                            (self -> rows - j);
      cumulativeHeight += self->rowHeights[j];
    }

  int count = 0;
  for (struct llist_node *node = llist_head (self->items);
       node != NULL;
       node = llist_node_next (node))
    {
      struct GridItem *item = llist_node_data (node);
      struct Rectangle *childRect = widget_get_rectangle (item -> widget);
      int32_t x, y, w, h;
      x = colWidth (self, 0, item -> col);
      y = rowHeight (self, 0, item -> row);
      w = colWidth (self, item -> col, item -> colspan);
      h = rowHeight (self, item -> row, item -> rowspan);
      if (childRect -> x != x || childRect -> y != y)
        widget_move (item -> widget, x, y);
      if (childRect -> w != w || childRect -> h != h)
        widget_resize (item -> widget, w, h);
      rectangleDestroy (childRect);
    }
}

/* METHOD
 * addWidget :: (object, uint32, uint32, uint32, uint32) -> ()
 */
void
gridlayoutAddWidget (struct GridLayout *self, struct Object *obj, uint32_t col, uint32_t row, uint32_t colspan, uint32_t rowspan)
{
  struct GridItem *item;

  item = ymalloc (sizeof (struct GridItem));
  item -> widget = (struct Widget *) obj;
  item -> col     = col;
  item -> row     = row;
  item -> colspan = colspan;
  item -> rowspan = rowspan;

  llist_add_tail (self -> items, item);

  if (item -> col + item -> colspan > self -> cols ||
      item -> row + item -> rowspan > self -> rows)
    {
       gridlayoutResizeGrid (self, self -> cols, self -> rows);
    }
  else
    {
       gridlayoutFitChildren (self);
       widget_repaint (gridlayoutToWidget (self),
                      widget_get_rectangle (item -> widget));
    }
  widget_reconfigure (gridlayoutToWidget (self));
  widget_set_container (item -> widget, gridlayoutToWidget (self));
}

/* METHOD
 * removeWidget :: (object) -> ()
 */
void
gridlayoutRemoveWidget (struct GridLayout *self, struct Object *obj)
{
  struct Widget *widget = (struct Widget *) obj;

  for (struct llist_node *node = llist_head (self->items);
       node != NULL;
       node = llist_node_next (node))
    {
      struct GridItem *item = llist_node_data (node);
      if (item -> widget == widget)
        {
          llist_node_delete (node);
          yfree (item);
          break;
        }
    }
  gridlayoutResizeGrid (self, 0, 0);
  widget_reconfigure (gridlayoutToWidget (self));
  widget_set_container (widget, NULL);
}

void
gridlayoutReconfigure (struct Widget *self_w)
{
  struct GridLayout *self = castBack (self_w);
  int minRowHeight = -1, minColWidth = -1;
  int reqRowHeight = -1, reqColWidth = -1;

  /* homogeneous */

  for (struct llist_node *node = llist_head (self->items);
       node != NULL;
       node = llist_node_next (node))
    {
      struct GridItem *item = llist_node_data (node);
      int mcw = item -> widget -> minWidth / item -> colspan;
      int mrh = item -> widget -> minHeight / item -> rowspan;
      int rcw = item -> widget -> reqWidth / item -> colspan;
      int rrh = item -> widget -> reqHeight / item -> rowspan;
      if (mcw > minColWidth)
        minColWidth = mcw;
      if (mrh > minRowHeight)
        minRowHeight = mrh;
      if (rcw > reqColWidth)
        reqColWidth = rcw;
      if (rrh > reqRowHeight)
        reqRowHeight = rrh;
    }

  if (minColWidth > 0)
    self -> widget.minWidth = minColWidth * self -> cols;
  else
    self -> widget.minWidth = -1;
  if (minRowHeight > 0)
    self -> widget.minHeight = minRowHeight * self -> rows;
  else
    self -> widget.minHeight = -1;

  if (reqColWidth < minColWidth)
    reqColWidth = minColWidth;
  if (reqRowHeight < minRowHeight)
    reqRowHeight = minRowHeight;

  if (reqColWidth > 0)
    self -> widget.reqWidth = reqColWidth * self -> cols;
  else
    self -> widget.reqWidth = -1;
  if (reqRowHeight > 0)
    self -> widget.reqHeight = reqRowHeight * self -> rows;
  else
    self -> widget.reqHeight = -1;

  widget_reconfigure (self -> widget.container);
}

void
gridlayoutResize (struct Widget *self_w)
{
  struct GridLayout *self = castBack (self_w);
  gridlayoutFitChildren (self);
}

int
gridlayoutPointerMotion (struct Widget *self_w, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
  struct GridLayout *self = castBack (self_w);

  /*convert to local*/
  struct Rectangle *r = widget_get_rectangle (self_w);
  x -= r->x;
  y -= r->y;
  rectangleDestroy (r);


  /* iterate backwards, so items that appear on top get served first */
  for (struct llist_node *node = llist_tail (self->items);
       node != NULL;
       node = llist_node_prev (node))
    {
      struct GridItem *item = llist_node_data (node);
      //      int32_t lx = x - item -> widget -> x;
      //      int32_t ly = y - item -> widget -> y;
      if (widget_contains_point_local (item -> widget, x, y))
        {
          if (self -> pointerWidget != item -> widget)
            {
              if (self -> pointerWidget != NULL)
                widget_pointer_leave (self -> pointerWidget);
              self -> pointerWidget = item -> widget;

              widget_pointer_enter (item -> widget, x, y);
            }
          widget_pointer_motion (item -> widget, x, y, dx, dy);
          return 1;
        }
    }

  if (self -> pointerWidget != NULL)
    {
      widget_pointer_leave (self -> pointerWidget);
      self -> pointerWidget = NULL;
    }
  return 0;
}

int
gridlayoutPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t b, bool p)
{
  struct GridLayout *self = castBack (self_w);

  /*convert to local*/
  struct Rectangle *r = widget_get_rectangle (self_w);
  x -= r->x;
  y -= r->y;
  rectangleDestroy (r);

  /* iterate backwards, so items that appear on top get served first */
  for (struct llist_node *node = llist_tail (self->items);
       node != NULL;
       node = llist_node_prev (node))
    {
      struct GridItem *item = llist_node_data (node);
      //      int32_t lx = x - item -> widget -> x;
      //      int32_t ly = y - item -> widget -> y;
      if (widget_contains_point_local (item -> widget, x, y)
          && widget_pointer_button (item -> widget, x, y, b, p))
        return 1;
    }
  return 0;
}

void
gridlayoutPointerEnter (struct Widget *self_w, int32_t x, int32_t y)
{
  struct GridLayout *self = castBack (self_w);

  /*convert to local*/
  struct Rectangle *r = widget_get_rectangle (self_w);
  x -= r->x;
  y -= r->y;
  rectangleDestroy (r);
 

  /* iterate backwards, so items that appear on top get served first */
  for (struct llist_node *node = llist_tail (self->items);
       node != NULL;
       node = llist_node_prev (node))
    {
      struct GridItem *item = llist_node_data (node);
      //      int32_t lx = x - item -> widget -> x;
      //      int32_t ly = y - item -> widget -> y;
      if (widget_contains_point_local (item -> widget, x, y))
        {
          self -> pointerWidget = item -> widget;
          widget_pointer_enter (item -> widget, x, y);
          return;
        }
    }
  return;
}

void
gridlayoutPointerLeave (struct Widget *self_w)
{
  struct GridLayout *self = castBack (self_w);

  if (self -> pointerWidget != NULL)
    {
      widget_pointer_leave (self -> pointerWidget);
      self -> pointerWidget = NULL;
    }
}


/* arch-tag: 24031e9a-1ee1-4239-aa61-d876e524b995
 */
