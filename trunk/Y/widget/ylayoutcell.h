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

#ifndef Y_WIDGET_YLAYOUTCELL_H
#define Y_WIDGET_YLAYOUTCELL_H

#include <Y/widget/widget_p.h>
#include <Y/util/yutil.h>
#include <Y/buffer/painter.h>

enum ExpandStyle
  {
    STRETCH = 0,    //Expands to fit all available area..
    BEST_FIT = 1    //Tries to give the item its requested size
  };

enum HAlignment
  {
    H_ALIGN_LEFT = 0,
    H_ALIGN_CENTER = 1,
    H_ALIGN_RIGHT = 2
  };

enum VAlignment
  {
    V_ALIGN_TOP = 0,
    V_ALIGN_CENTER = 1,
    V_ALIGN_BOTTOM = 2
  };


struct YLayoutCell
{
  struct Widget *widget;   // this is the widget that is contained in this slot
  enum ExpandStyle expand;        // how to expand this cell (vertical)
  struct Rectangle rect;         //position and width of this item..
  enum HAlignment h_align;     //Horizontal Alignment (Of contained widget)
  enum VAlignment v_align;     //Vertical Alignment
  int padding_top;
  int padding_bottom;
  int padding_left;
  int padding_right;

  /*
   * allow a border to be painted, 
   * should usually be just for debugging
   */
   int border_width; 
};

struct YLayoutCell *ylayoutcell_create(void);
void ylayoutcell_destroy(struct YLayoutCell *cell);
void ylayoutcell_align_widget(struct YLayoutCell *cell);
void ylayoutcell_get_constraints(struct YLayoutCell *cell,
				 uint32_t *min_width,
				 uint32_t *min_height,
				 uint32_t *req_width,
				 uint32_t *req_height);
//paints the cell then the widget
void ylayoutcell_paint (struct YLayoutCell *cell, 
			struct Painter *painter);
#endif
