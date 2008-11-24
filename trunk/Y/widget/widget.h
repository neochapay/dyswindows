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

#ifndef Y_WIDGET_WIDGET_H
#define Y_WIDGET_WIDGET_H

#include <stdbool.h>
#include <Y/ytypes.h>

struct Widget;
struct WidgetTable;

enum WidgetState
{
  WIDGET_STATE_NORMAL,
  WIDGET_STATE_HOVER,
  WIDGET_STATE_FOCUS,
  WIDGET_STATE_SELECTED,
  WIDGET_STATE_PRESSED,
  WIDGET_STATE_CANCELLING,
  WIDGET_STATE_DISABLED
};

#include <Y/y.h>
#include <Y/const.h>
#include <Y/util/rectangle.h>
#include <Y/screen/renderer.h>
#include <Y/buffer/painter.h>
#include <Y/input/pointer.h>
#include <Y/input/ykb.h>

struct Object *
       widget_to_object      (struct Widget *);
struct Widget * 
       object_to_widget (struct Object *);
void   widget_global_to_local (const struct Widget *, int *, int *);
void   widget_local_to_global (const struct Widget *, int *, int *);

struct Rectangle *
       widget_get_rectangle  (const struct Widget *);


void   widget_get_size       (const struct Widget *, int32_t *, int32_t *);
void   widget_get_constraints(const struct Widget *, int32_t *, int32_t *,
                            int32_t *, int32_t *, int32_t *, int32_t *);
enum WidgetState
       widget_get_state      (const struct Widget *);
//bool   widgetContainsPoint (const struct Widget *, int32_t x, int32_t y); 

struct Window *
       widget_get_window     (struct Widget *);

void   widget_move          (struct Widget *, int32_t, int32_t);
void   widget_resize        (struct Widget *, int32_t, int32_t);
void   widget_reconfigure   (struct Widget *);

void   widget_set_container  (struct Widget *, struct Widget *);
void   widget_unpack        (struct Widget *, struct Widget *);

void   widget_render        (struct Widget *, Renderer *);
void   widget_paint         (struct Widget *, struct Painter *);
void   widget_repaint       (struct Widget *, struct Rectangle *);
void   widget_rerender      (struct Widget *, struct Rectangle *);

int    widget_pointer_motion (struct Widget *, int32_t x, int32_t y, int32_t dx, int32_t dy);
int    widget_pointer_button (struct Widget *, int32_t x, int32_t y, uint32_t b, bool pressed);
void   widget_pointer_enter  (struct Widget *, int32_t x, int32_t y);
void   widget_pointer_leave  (struct Widget *);

void   widget_local_to_buffer (const struct Widget *self, int *x_p, int *y_p);

void   widget_get_position_buffer (const struct Widget *self, int32_t *x_p, int32_t *y_p);
void   widget_get_position_local   (const struct Widget *, int32_t *, int32_t *);

struct Rectangle *
widget_get_rectangle_buffer (const struct Widget *self);

bool   widget_contains_point_buffer (const struct Widget *self, int32_t x, int32_t y);

bool   widget_contains_point_local (const struct Widget *self, int32_t x, int32_t y);






ykbStringHandler widget_ykb_string;
ykbEventHandler widget_ykb_event;
ykbStrokeHandler widget_ykb_stroke;
ykbGetCursor widget_ykb_get_cursor;

#endif /* header guard */

