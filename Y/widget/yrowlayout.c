/************************************************************************
 *   Copyright (C) Dustin Norlander <dustin@dustismo.com>
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

/*
 * Object hierarchy:
 *
 * Object
 *   +---Widget
 *           +---YContainer
 *                   +---YRowLayout
 */
 
/*
 * This is a row layout.  it lays widgets out in a row (obviously)
 * It is comparable to the GTK+ HBox
 *
 * Each cell can be set to STRETCH or BEST_FIT ExpandStyle.
 * BEST_FIT attempts to give the widget its requested width.
 * STRETCH cells stretch to fit all available space..
 * 
 * If all cells are set to BEST_FIT the behavior is the same is
 * if all cells are set to STRETCH..
 *
 * -DN
 */

#include <Y/widget/yrowlayout.h>
#include <Y/widget/ylayoutcell.h>
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


#ifndef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#endif


struct YRowLayout
{
  struct YContainer container;
  struct llist *cells;  
  uint32_t num_cells;
  uint32_t hgap, //horizontal gap between sections
    vgap; //vertical gap between sections
  struct Widget * pointer_widget;   //this is the pointer.. 
  //XXX: pointerWidget should probably be handled in the SUPER class
};

static void yrowlayout_unpack (struct Widget *, struct Widget *);
static void yrowlayout_paint (struct Widget *, struct Painter *);
static void yrowlayout_resize (struct Widget *);
static void yrowlayout_reconfigure (struct Widget *);
static int yrowlayout_pointer_motion (struct Widget *, int32_t, int32_t, int32_t, int32_t);
static int yrowlayout_pointer_button (struct Widget *, int32_t, int32_t, uint32_t, bool);
static void yrowlayout_pointer_enter (struct Widget *, int32_t, int32_t);
static void yrowlayout_pointer_leave (struct Widget *);

static void yrowlayout_arrange_cells (struct YRowLayout *self);
static void yrowlayout_init (struct YRowLayout *self);


DEFINE_CLASS(YRowLayout);
#include "YRowLayout.yc"

/* SUPER
 * YContainer
 */

static struct WidgetTable yrowlayoutTable;

void
CLASS_INIT (struct YRowLayout *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  this -> cells = new_llist ();
  this -> pointer_widget = NULL;
  struct WidgetTable *tab = (struct WidgetTable *)vtable;
  //initialize the WidgetTable
  tab -> unpack = yrowlayout_unpack;
  tab -> reconfigure = yrowlayout_reconfigure;
  tab -> resize = yrowlayout_resize;
  tab -> paint = yrowlayout_paint;
  tab -> pointerMotion = yrowlayout_pointer_motion;
  tab -> pointerButton = yrowlayout_pointer_button;
  tab -> pointerEnter = yrowlayout_pointer_enter;
  tab -> pointerLeave = yrowlayout_pointer_leave;

}

static inline struct YRowLayout *
cast_back (struct Widget *widget)
{
  /* assert ( widget -> c == yrowlayoutClass ); */
  return (struct YRowLayout *)widget;
}

static inline const struct YRowLayout *
cast_back_const (const struct Widget *widget)
{
  /* assert ( widget -> c == yrowlayoutClass ); */
  return (const struct YRowLayout *)widget;
}

/* 
 * This removes the inputed widget from the layout
 * and sets the widgets container to NULL
 */
static void
yrowlayout_unpack (struct Widget *self_w, struct Widget *w)
{
  struct YRowLayout *self = cast_back (self_w);
  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      if (cell -> widget == w) 
	{
	  cell -> widget = NULL;
	  widget_set_container (w, NULL);
	  llist_node_delete (node);
	  yfree(cell);
	  return;
	}
    }
}

/*
 * paints each child
 */
static void
yrowlayout_paint (struct Widget *self_w, struct Painter *painter)
{
  /* draw each child */
  struct YRowLayout *self = cast_back (self_w);
  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      struct Rectangle *rect = widget_get_rectangle (cell -> widget);
      if (!painter_is_fully_clipped (painter))
	ylayoutcell_paint(cell, painter);
      rectangleDestroy (rect);
    }
}

static struct YRowLayout *
yrowlayout_create (void)
{
  struct YRowLayout *self = ymalloc (sizeof (struct YRowLayout));
  objectInitialise (&(self -> container.widget.o), CLASS(YRowLayout));
  CLASS_INIT(self, &yrowlayoutTable.vtable);
  return self;
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
yrowlayout_destroy (struct YRowLayout *self)
{
  llist_destroy (self -> cells, ylayoutcell_destroy);
  widgetFinalise (yrowlayout_to_widget (self));
  objectFinalise (yrowlayout_to_object (self));
  yfree(self);
}

struct YContainer *  
yrowlayout_to_container (struct YRowLayout *self)
{
  return &(self -> container);
}

struct Widget *
yrowlayout_to_widget (struct YRowLayout *self)
{
  return &(self -> container.widget);
}

struct Object *
yrowlayout_to_object (struct YRowLayout *self)
{
  return &(self -> container.widget.o);
}

/* METHOD
 * YRowLayout :: () -> (object)
 */
static struct Object *
yrowlayout_instantiate (void)
{
  struct Object * temp = yrowlayout_to_object (yrowlayout_create ());
  return temp;
}

/* 
 * Need a second algorithm to resize everything
 * If the current widget size is less then necessary size 
 * to hold all of the children..
 *
 * Description of packing algorithm:
 * Algorith adapted from the GTK+ hbox packing algorithm
 * See gtkhbox.c gtk_hbox_size_allocate()
 * 
 * varaiables:
 *  necWidth -> minimum width necessary to hold all widgets
 *  actualWidth - > actual width allocated to this yrowlayout
 *  width -> 
 *  height -> height of this row.
 *  extra -> number of extra pixels available for stretching widgets
 *  numVisible -> number of visible cells
 *  numStretch -> number of cells set to STRETCH
 *  allStretch -> boolean telling whether to stretch all cells
 *                
 *  
 * 1. Determine the necWidth, numVisible, height, and numStretch
 * height = 0
 * For Each cell:
 *    necessaryWidth += required width of each cell
 *    if (current cell is visible){
 *        -> numVisible++
 *        if (current cell is STRETCH) 
 *             -> numStretch++
 *    }
 *    height = MAX(height, current cell height)   
 * End For Each
 *
 * 2. if (numStretch==0) -> allStretch=true
 * 
 * 3. if (necWidth > actualWidth) -> some widgets need to get shrunk..
 *
 * 4. set width = necWidth - actualWidth
 *
 * 5. set extra = width / numStretch
 *       if (allStretch) -> extra = width / numVisible
 *
 * 6. assign widths to each cell 
 * For Each Cell:
 *    if (cell -> STRETCH or allStretch) {
 *       if (numStretch == 1)
 *          cell.width += width
 *       else
 *          cell.width += extra
 *       
 *       numStretch--
 *       width -= extra
 *    }
 *    Determine X and Y position for the cell
 * End For Each
 *
 * 
 */
static void
yrowlayout_arrange_cells (struct YRowLayout *self)
{

  int numStretch = 0;
  int extra = 0;
  int width = 0;
  int height = yrowlayout_to_widget(self) -> h;
  int necWidth = 0;
  int actualWidth = yrowlayout_to_widget(self) -> w;
  int numVisible = 0;
  bool allStretch = FALSE;

  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      struct Rectangle *childRect = widget_get_rectangle (cell -> widget);      
      int childWidth = MAX(cell -> widget -> minWidth, childRect -> w);
      int childHeight = MAX(cell -> widget -> minHeight, childRect -> h);
      
      necWidth += childWidth;
      if (childWidth > 0 && childHeight > 0) {
	numVisible++;
	if (cell->expand == STRETCH)
	  numStretch++;
      }
      rectangleDestroy (childRect);
    }
  
  if (numStretch ==0)
    allStretch = TRUE;
  
  width = actualWidth - necWidth;
  
  if (numStretch > 0)
    extra = (int) width / numStretch;
  
  if (allStretch && numVisible > 0)
    extra = width / numVisible;

/*
 * 6. assign widths to each cell 
 */
  int x = 0;//yrowlayoutToWidget(self) -> x;
  int y = 0;//yrowlayoutToWidget(self) -> y;

  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      struct Rectangle *childRect = widget_get_rectangle (cell -> widget);   
      int childWidth = MAX(cell -> widget -> minWidth, childRect -> w);
      int childHeight = MAX(cell -> widget -> minHeight, childRect -> h);
   
      cell->rect.w = childWidth;
      cell->rect.h = height; 
      if (allStretch || cell->expand == STRETCH) {
	if (numStretch == 1)
	  cell->rect.w += width;
	else
	  cell->rect.w += extra;
	numStretch--;
	width -= extra;
      }

      //now determine x and y position of cell..
      cell->rect.x = x;
      cell->rect.y = y;
      
      rectangleDestroy (childRect);
      
      ylayoutcell_align_widget(cell);
      x += cell->rect.w;
    }
}

/*
 * Adds the widget to the end of the list.  
 */

/* METHOD
 * addWidget :: (object, uint32) -> ()
 */
void
yrowlayout_add_widget (struct YRowLayout *self, struct Object *obj, enum ExpandStyle cellExpandStyle)
{
  struct YLayoutCell *cell = ylayoutcell_create();
  cell -> expand = cellExpandStyle;
  cell -> widget = (struct Widget *) obj;


  self -> num_cells++;  
  llist_add_tail (self -> cells, cell);

  yrowlayout_arrange_cells (self);
  widget_repaint (yrowlayout_to_widget (self),
		 widget_get_rectangle (cell -> widget));
  widget_reconfigure (yrowlayout_to_widget (self));
  widget_set_container (cell -> widget, yrowlayout_to_widget (self));
}

/* METHOD
 * removeWidget :: (object) -> ()
 */
void
yrowlayout_remove_widget (struct YRowLayout *self, struct Object *obj)
{
  struct Widget *widget = (struct Widget *) obj;
  
  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      if (cell -> widget == widget)
        {
          llist_node_delete (node);
          ylayoutcell_destroy(cell);
	  self -> num_cells--;  
          break;
        }
    }
  widget_reconfigure (yrowlayout_to_widget (self));
}

/*
 * this reconfigures the size of the rowlayout widget.
 * 
 * This function is called whenever widget_reconfigure(YRowLayout) is called.
 * it sets minWidth = TOTAL(minWidths of all children)
 * and reqWidth = TOTAL(reqWidth of all children)
 *
 * -DN
 */

void
yrowlayout_reconfigure (struct Widget *self_w)
{
  struct YRowLayout *self = cast_back (self_w);
  int minWidth = 0;
  int minHeight = 0;
  int reqWidth = 0;
  int reqHeight = 0;

  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      minWidth += cell -> widget -> minWidth;
      reqWidth += cell -> widget -> reqWidth;
      minHeight = MAX (minHeight, cell -> widget -> minHeight);
      reqHeight = MAX (reqHeight, cell ->  widget -> reqHeight);
    }

  self_w -> minWidth = minWidth;
  self_w -> minHeight = minHeight;
  self_w -> reqWidth = reqWidth;
  self_w -> reqHeight = reqHeight;
  widget_reconfigure (self_w -> container);
}

void
yrowlayout_resize (struct Widget *self_w)
{
  struct YRowLayout *self = cast_back (self_w);
  yrowlayout_arrange_cells (self); 
}

/** 
 *
 */
int
yrowlayout_pointer_motion (struct Widget *self_w, 
			   int32_t x, 
			   int32_t y, 
			   int32_t dx, 
			   int32_t dy)
{
  struct YRowLayout *self = cast_back (self_w);
  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      int32_t lx = x - cell -> rect.x; //XXX: not sure if this is right
      int32_t ly = y - cell -> rect.y; //XXX: not sure if this is right
      if (widget_contains_point_local (cell -> widget, lx, ly))
        {

	  if (self -> pointer_widget != cell -> widget)
	    {
              if (self -> pointer_widget != NULL)
                widget_pointer_leave (self -> pointer_widget);
              self -> pointer_widget = cell -> widget;
              widget_pointer_enter (cell -> widget, lx, ly);
            }
          widget_pointer_motion (cell -> widget, x, y, dx, dy);
          return 1;
        } 
    }


  if (self -> pointer_widget != NULL)
    {
      widget_pointer_leave (self -> pointer_widget);
      self -> pointer_widget = NULL;
    }

  return 0;
}

int
yrowlayout_pointer_button (struct Widget *self_w, 
			   int32_t x, 
			   int32_t y, 
			   uint32_t b, 
			   bool p)
{
  struct YRowLayout *self = cast_back (self_w);
  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      int32_t lx = x - cell -> rect.x;
      int32_t ly = y - cell -> rect.y;
      if (widget_contains_point_local (cell -> widget, lx, ly)
          && widget_pointer_button (cell -> widget, lx, ly, b, p))
        return 1;
    }

  return 0;
}

void
yrowlayout_pointer_enter (struct Widget *self_w, 
			  int32_t x, 
			  int32_t y)
{
  struct YRowLayout *self = cast_back (self_w);
  for (struct llist_node *node = llist_head (self->cells);
       node != NULL;
       node = llist_node_next (node))
    {
      struct YLayoutCell *cell = llist_node_data (node);
      int32_t lx = x - cell -> widget -> x;
      int32_t ly = y - cell -> widget -> y;
      if (widget_contains_point_local (cell -> widget, lx, ly))
        {
          self -> pointer_widget = cell -> widget;
          widget_pointer_enter (cell -> widget, lx, ly);
          return;
        } 
    }
  return;
}

void
yrowlayout_pointer_leave (struct Widget *self_w)
{
  
  struct YRowLayout *self = cast_back (self_w);

  if (self -> pointer_widget != NULL)
    {
      widget_pointer_leave (self -> pointer_widget);
      self -> pointer_widget = NULL;
    }
}
