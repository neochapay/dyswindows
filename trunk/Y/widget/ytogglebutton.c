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
 *                    +---YBin
 *                          +---YButton
 *                                 +----YToggleButton
 */

#include <Y/widget/ytogglebutton.h>
#include <Y/widget/widget_p.h>

#include <Y/util/yutil.h>
#include <Y/buffer/painter.h>

#include <Y/modules/theme.h>

#include <Y/object/class.h>
#include <Y/object/object_p.h>

#include <Y/text/font.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* Prototypes */
static int ytogglebuttonPointerMotion (struct Widget *, int32_t, int32_t, int32_t, int32_t);
static int ytogglebuttonPointerButton (struct Widget *, int32_t, int32_t, uint32_t, bool);
static void ytogglebuttonPointerEnter (struct Widget *, int32_t, int32_t);
static void ytogglebuttonPointerLeave (struct Widget *);

DEFINE_CLASS(YToggleButton);
#include "YToggleButton.yc"

/* SUPER
 * YButton
 */

/* PROPERTY
 */

static struct WidgetTable ytogglebutton_table; //set up in ytogglebuttonInit

/*
 * Creates a new togglebutton, including allocating memory
 */
static struct YToggleButton *
ytogglebuttonCreate (void)
{
  struct YToggleButton *self = ymalloc (sizeof (struct YToggleButton));
  objectInitialise (&(self -> button.bin.container.widget.o), CLASS(YToggleButton));
  CLASS_INIT(self, &ytogglebutton_table.vtable);
  return self;
}

/*
 * Initialize the togglebutton
 */
void
CLASS_INIT (struct YToggleButton *self, VTable *vtable)
{
  SUPER_INIT(self, vtable);
  struct WidgetTable *tab = (struct WidgetTable *)vtable;
  //initialize the WidgetTable

  tab -> pointerButton = ytogglebuttonPointerButton;
  tab -> pointerMotion = ytogglebuttonPointerMotion;
  tab -> pointerEnter = ytogglebuttonPointerEnter;
  tab -> pointerLeave = ytogglebuttonPointerLeave;
}

static inline struct YToggleButton *
castBack (struct YButton *button)
{
  /* assert ( button -> c == ytogglebuttonClass ); */
  return (struct YToggleButton *)button;
}

static inline const struct YToggleButton *
castBackConst (const struct YButton *button)
{
  /* assert ( button -> c == ytogglebuttonClass ); */
  return (const struct YToggleButton *)button;
}

struct YToggleButton * 
ybuttonToYToggleButton (struct YButton *button)
{
  return castBack (button);
}

/*
 * Converts between YToggleButton and Widget
 */
static struct YToggleButton *
widgetToYToggleButton (struct Widget *self)
{
  /* assert ( self -> c == ytogglebuttonClass ); */
  return (struct YToggleButton *)self;
}


struct Widget *
ytogglebuttonToWidget (struct YToggleButton *self)
{
  return &(self -> button.bin.container.widget);
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
ytogglebuttonDestroy (struct YToggleButton *self)
{
  widgetFinalise (ytogglebuttonToWidget (self));
  objectFinalise (ytogglebutton_to_object (self));
  yfree (self);
}



struct Object *
ytogglebutton_to_object (struct YToggleButton *self)
{
  return &(self -> button.bin.container.widget.o);
}

/* METHOD
 * YToggleButton :: () -> (object)
 */
static struct Object *
ytogglebuttonInstantiate (void)
{
  return ytogglebutton_to_object (ytogglebuttonCreate ());
}

/*
 * Really the only change between YToggleButton and YButton is right here
 */
static int
ytogglebuttonPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t b, bool pressed)
{
  struct YToggleButton *self = widgetToYToggleButton(self_w);
  
  if (b==0 && self -> button.bin.container.widget.state != WIDGET_STATE_DISABLED)
    {
      if (pressed)
        {
          self -> button.bin.container.widget.state = WIDGET_STATE_PRESSED;
          if ( self -> button.state == BUTTON_STATE_PRESSED )
           {
             objectEmitSignal (ytogglebutton_to_object (self), "released");
             ybuttonSetPropertyPressed (&self->button, 0);
           } 
          else
           {
             objectEmitSignal (ytogglebutton_to_object (self), "pressed");
             ybuttonSetPropertyPressed (&self->button, 1);
           }
          widget_repaint (self_w, NULL); 
        }        
      else
        {
          if (widget_contains_point_local (self_w, x, y))
            self -> button.bin.container.widget.state = WIDGET_STATE_HOVER;
          else
            self -> button.bin.container.widget.state = WIDGET_STATE_NORMAL;  
          widget_repaint (self_w, NULL);
        }
    }
  return 1;
}

static int
ytogglebuttonPointerMotion (struct Widget *self_w, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
  //do nothing..  doesn't matter if user clicks and drags off
  return 1;
}

static void
ytogglebuttonPointerEnter (struct Widget *self_w, int32_t x, int32_t y)
{
  struct YToggleButton *self = widgetToYToggleButton (self_w);
  
  if (self->button.bin.container.widget.state == WIDGET_STATE_DISABLED )
    return; //do nothing if widget is disabled
  
  self -> button.bin.container.widget.state = WIDGET_STATE_HOVER;
  widget_repaint (ytogglebuttonToWidget (self), NULL);
}

static void
ytogglebuttonPointerLeave (struct Widget *self_w)
{
  struct YToggleButton *self = widgetToYToggleButton (self_w);
  
  if (self->button.bin.container.widget.state == WIDGET_STATE_DISABLED)
    return; //do nothing if widget is disabled or in pressed state
    
  self -> button.bin.container.widget.state = WIDGET_STATE_NORMAL;
  widget_repaint (ytogglebuttonToWidget (self), NULL);
}

/* arch-tag: 791d4e69-40ee-48d3-885f-fcb076464847
 */
