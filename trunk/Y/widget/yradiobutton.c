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
 *                                  +---YToggleButton
 *                                            +---YRadioButton
 */


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

#include <Y/widget/yradiogroup.h>

/* Prototypes */
static void yradiobuttonPaint (struct Widget *, struct Painter *);
static void yradiobuttonPressedSet (struct YRadioButton *);
static int yradiobuttonPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t b, bool pressed);

DEFINE_CLASS(YRadioButton);
#include "YRadioButton.yc"

/* SUPER
 * YToggleButton
 */

/* PROPERTY
 * pressed :: ybool
 */

static struct WidgetTable yradiobutton_table;

/*
 * Creates a new radiobutton, including allocating memory
 */
static struct YRadioButton *
yradiobuttonCreate (void)
{
  struct YRadioButton *self = ymalloc (sizeof (struct YRadioButton));
  objectInitialise (&(self -> togglebutton.button.bin.container.widget.o), CLASS(YRadioButton));
  CLASS_INIT(self, &yradiobutton_table.vtable);
  return self;
}

/*
 * Initialize the radiobutton
 */
void
CLASS_INIT (struct YRadioButton *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  
  this -> group = NULL; //the client must set the radiogroup
  struct WidgetTable *tab = (struct WidgetTable *)vtable; 
  tab -> paint = yradiobuttonPaint;			
  tab -> pointerButton = yradiobuttonPointerButton;
}

static inline struct YRadioButton *
castBack (struct YToggleButton *button)
{
  /* assert ( button -> c == yradiobuttonClass ); */
  return (struct YRadioButton *)button;
}

static inline const struct YRadioButton *
castBackConst (const struct YToggleButton *button)
{
  /* assert ( button -> c == yradiobuttonClass ); */
  return (const struct YRadioButton *)button;
}

struct YRadioButton * 
ytogglebuttonToYRadioButton (struct YToggleButton *button)
{
  return castBack (button);
}

/*
 * Converts between YRadioButton and Widget
 */
struct YRadioButton *
widgetToYRadioButton (struct Widget *self)
{
  /* assert ( self -> c == yradiobuttonClass ); */
  return (struct YRadioButton *)self;
}


struct Widget *
yradiobuttonToWidget (struct YRadioButton *self)
{
  return &(self -> togglebutton.button.bin.container.widget);
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
yradiobuttonDestroy (struct YRadioButton *self)
{
  widgetFinalise (yradiobuttonToWidget (self));
  objectFinalise (yradiobutton_to_object (self));
  yfree (self);
}



struct Object *
yradiobutton_to_object (struct YRadioButton *self)
{
  return &(self -> togglebutton.button.bin.container.widget.o);
}

/* METHOD
 * YRadioButton :: () -> (object)
 */
static struct Object *
yradiobuttonInstantiate (void)
{
  return yradiobutton_to_object (yradiobuttonCreate ());
}

/*
 * Paints the radiobutton
 */
static void
yradiobuttonPaint (struct Widget *self_w, struct Painter *painter)
{
  themeDrawYRadioButton (painter, widgetToYRadioButton(self_w));
}

/*
 * 
 * Here is where the radiobutton is selected.  
 *
 */
static int
yradiobuttonPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t b, bool pressed_value)
{
  struct YRadioButton *self = widgetToYRadioButton(self_w);
  
  if (b==0 && self -> togglebutton.button.bin.container.widget.state != WIDGET_STATE_DISABLED)
    {
      if (pressed_value)
        {
          self -> togglebutton.button.bin.container.widget.state = WIDGET_STATE_PRESSED;
          /* if button is already pressed, do nothing */
          if ( !(self -> togglebutton.button.state == BUTTON_STATE_PRESSED) )
           {
             yradiobuttonSetPropertyPressed (self, 1);	//there is no group associated with this radiobutton
           } 
          widget_repaint (self_w, NULL); 
        }        
      else
        {
          if (widget_contains_point_local (self_w, x, y))
            self -> togglebutton.button.bin.container.widget.state = WIDGET_STATE_HOVER;
          else
            self -> togglebutton.button.bin.container.widget.state = WIDGET_STATE_NORMAL;  
          widget_repaint (self_w, NULL);
        }
    }
  return 1;
}



/* PROPERTY HOOK
 * pressed
 */
static void
yradiobuttonPressedSet (struct YRadioButton *self)
{ 
  if (!self)
    return;
  
  int32_t value = safeGetProperty(self, pressed, 0);
  
  /* if this button has a group associated, we need to let the group know.. 
   */
  if (self->group && value)
    groupSetSelectedRadioButton (self->group, self);
       
  if (value) 
  {
    self->togglebutton.button.state = BUTTON_STATE_PRESSED;
    objectEmitSignal (yradiobutton_to_object (self), "pressed");
  }
  else
  {
    self->togglebutton.button.state = BUTTON_STATE_NORMAL;
    objectEmitSignal (yradiobutton_to_object (self), "released");
  }
  widget_repaint (yradiobuttonToWidget (self), NULL);
}

/*
 * Classes such as YRadioGroup need this function as
 * the setProperty and getProperty are definations not available to other classes
 */
void
yradiobuttonSetPropertyPressed (struct YRadioButton *self, int32_t value)
{
  Y_TRACE ( "Setting property pressed! 1");
  if (!self)
    return;
  Y_TRACE ( "Setting property pressed! 2");
  setProperty (self, pressed, value);
}

/*
 * Get property wrapper
 */
int32_t
yradiobuttonGetPropertyPressed (struct YRadioButton *self)
{
  return safeGetProperty(self, pressed, 0);
}


/*
 * returns the radiogroup for this radiobutton
 * else NULL
 * -DN
 */

/* METHOD
 * getRadioGroup :: () -> (object)
 */
static struct Object *
yradiobuttonGetRadioGroup (struct YRadioButton *self)
{
  if (self->group)
    return yradiogroup_to_object (self->group);
  return NULL;
}

/*
 * This method sets the radiogroup for this radiobutton
 * if there is already a group for this radiobutton, the old one is 
 * replaced - and this radiobutton is removed from the old radiogroup
 * and placed into the new one. 
 */

/* METHOD
 * setRadioGroup :: (object) -> ()
 */
void
yradiobuttonSetRadioGroup (struct YRadioButton *self, struct Object *obj)
{
  if (self->group) 
  {
    //radiobutton is already a member of a group
    //so we first need to remove it.
    groupRemoveRadioButton (self->group, self);
  } 
  groupAddRadioButton (objectToYRadioGroup (obj), self);
}
