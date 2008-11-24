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
 * These functions are for use with the various layout managers. 
 * Each layout manager is assumed to align a series of 'cells' which
 * can each contain one widget.
 * 
 * The cells control the position/size of the widget they contain.
 * Each widget is positioned within the coordinate system of the cell
 * That is if the widget is at 0,0 it is positioned in the upper left
 * of the cell (V_ALIGN_TOP, H_ALIGN_LEFT).
 * This is helpful, because then the widget doesn't have to care if the
 * cell gets repositioned. 
 */

#include <Y/widget/ylayoutcell.h>
#include <Y/widget/widget_p.h>


struct YLayoutCell *
ylayoutcell_create (void)
{
  struct YLayoutCell *cell = ymalloc (sizeof(struct YLayoutCell));
  cell -> rect.w = 0;
  cell -> rect.h = 0;
  cell -> rect.x = 0;
  cell -> rect.y = 0;
  cell -> padding_top = 0;
  cell -> padding_bottom = 0;
  cell -> padding_left = 0;
  cell -> padding_right = 0;

  //temp, should be set by user
  cell -> h_align = H_ALIGN_LEFT;
  cell -> v_align = V_ALIGN_TOP;
  return cell;
}

void
ylayoutcell_destroy(struct YLayoutCell *cell)
{
  widget_set_container (cell -> widget, NULL);
  yfree (cell);
}

/*
 * This function aligns the widget within the cell.
 * It should only be called after the cell itself has been aligned
 * -DN 
 *
 */
void
ylayoutcell_align_widget (struct YLayoutCell *cell)
{
  struct Widget *widget = cell->widget;
  struct Rectangle *childRect = widget_get_rectangle (cell -> widget);   
  //start with widget in upper left
  int x = 0; 
  int y = 0;
  int width = cell->rect.w;
  if (widget->maxWidth != -1 && widget->maxWidth < cell->rect.w)
    width = widget ->maxWidth;
  int height = cell->rect.h;
  if (widget->maxHeight != -1 && widget->maxHeight < cell->rect.h)
    height = widget ->maxHeight;

  //we only need to worry about the halign if the widgets
  //maxWidth is less then the available cell space.
  if (width != cell->rect.w) 
    {
      switch(cell -> h_align) 
	{
	case (H_ALIGN_LEFT):
	  //do nothing, widget is aligned LEFT
	  break;
	case (H_ALIGN_CENTER):
	  x += (cell->rect.w - width)/2;
	  break;
	case (H_ALIGN_RIGHT):
	  x += cell->rect.w - width;
	  break;
	}
    }

  if (height != cell->rect.h)
    {
      switch(cell->v_align)
	{
	case (V_ALIGN_TOP):
	  //do nothing, already aligned top
	  break;
	case (V_ALIGN_CENTER):
	  y += (cell->rect.h - height)/2;
	  break;
	case (V_ALIGN_BOTTOM):
	  y += cell->rect.h - height;
	  break;
	}
    }

  if (childRect -> x != x || childRect -> y != y)
    widget_move (cell -> widget, x, y);
  
  if (childRect -> w != width || childRect -> h != height)
    widget_resize (cell -> widget, width, height);

  rectangleDestroy (childRect);
}

/*
 * Paints the widget.
 *
 */
void 
ylayoutcell_paint (struct YLayoutCell *cell, struct Painter *painter) 
{
  
  painter_save_state (painter); //need to preserve the current coord sys
  painter_set_origin_local (painter, 
			      cell -> rect.x, 
			      cell -> rect.y);
  widget_paint(cell -> widget, painter);
  painter_restore_state(painter);
}
