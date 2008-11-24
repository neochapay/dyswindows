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
 */
 
#include <Y/widget/ybutton.h>
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



DEFINE_CLASS(YButton);
#include "YButton.yc"

/* SUPER
 * YBin
 */

/* PROPERTY
 * label :: string
 * focus_on_click :: ybool
 * pressed :: ybool
 */

/* Function prototypes */
static void ybuttonReconfigure (struct Widget *);
static void ybuttonPaint (struct Widget *, struct Painter *);
static int ybuttonPointerMotion (struct Widget *, int32_t, int32_t, int32_t, int32_t);
static int ybuttonPointerButton (struct Widget *, int32_t, int32_t, uint32_t, bool);
static void ybuttonPointerEnter (struct Widget *, int32_t, int32_t);
static void ybuttonPointerLeave (struct Widget *);

static struct WidgetTable ybutton_table;


/*
 * Creates a new button, including allocating memory
 */
static struct YButton *
ybuttonCreate (void)
{
  struct YButton *self = ymalloc (sizeof (struct YButton));
  objectInitialise (&(self -> bin.container.widget.o), CLASS(YButton));
  CLASS_INIT(self, &ybutton_table.vtable);
  return self;
}

/*
 * Initializes the YButton
 */
void
CLASS_INIT (struct YButton *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  struct WidgetTable *tab = (struct WidgetTable *)vtable;
  //initialize the WidgetTable
  tab -> reconfigure = ybuttonReconfigure;
  tab -> paint = ybuttonPaint;
  tab -> pointerButton = ybuttonPointerButton;
  tab -> pointerMotion = ybuttonPointerMotion;
  tab -> pointerEnter = ybuttonPointerEnter;
  tab -> pointerLeave = ybuttonPointerLeave;
  this -> state = BUTTON_STATE_NORMAL;
}

static inline struct YButton *
castBack (struct YBin *bin)
{
  /* assert ( bin -> c == ybuttonClass ); */
  return (struct YButton *)bin;
}

static inline const struct YButton *
castBackConst (const struct YBin *bin)
{
  /* assert ( bin -> c == ybuttonClass ); */
  return (const struct YButton *)bin;
}

struct YButton * 
ybinToYButton (struct YBin *bin)
{
  return castBack (bin);
}

/*
 * Converts between YButton and Widget
 */
struct YButton *
widgetToYButton (struct Widget *self)
{
  /* assert ( self -> c == ybuttonClass ); */
  return (struct YButton *)self;
}

struct Widget *
ybuttonToWidget (struct YButton *self)
{
  return &(self -> bin.container.widget);
}

struct Object *
ybutton_to_object (struct YButton *self)
{
  return &(self -> bin.container.widget.o);
}

static void
ybuttonPaint (struct Widget *self_w, struct Painter *painter)
{
  themeDrawYButton (painter, widgetToYButton(self_w));
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
ybuttonDestroy (struct YButton *self)
{
  widgetFinalise (ybuttonToWidget (self));
  objectFinalise (ybutton_to_object (self));
  yfree (self);
}

/* METHOD
 * YButton :: () -> (object)
 */
static struct Object *
ybuttonInstantiate (void)
{
  return ybutton_to_object (ybuttonCreate ());
}

static void ybuttonReconfigure (struct Widget *self_w)
{
  struct YButton *self = widgetToYButton (self_w);
  struct Font *font;
  int offset, width, ascender, descender;
  const char *text = safeGetProperty(self, label, "");
  font = themeGetDefaultFont ();
  fontGetMetrics (font, &ascender, &descender, NULL);
  fontMeasureString (font, text, &offset, &width, NULL);

  self -> bin.container.widget.minWidth = width - offset + 10;
  self -> bin.container.widget.minHeight = ascender + descender + 12;
  self -> bin.container.widget.reqWidth = width - offset + 10;
  self -> bin.container.widget.reqHeight =  ascender + descender + 12;

  /* we would prefer to be square rather than narrow... */
  if (self -> bin.container.widget.reqWidth < self -> bin.container.widget.reqHeight)
    self -> bin.container.widget.reqWidth = self -> bin.container.widget.reqHeight;

  widget_reconfigure (self -> bin.container.widget.container);
}

/* PROPERTY HOOK
 * label
 */
static void
ybuttonLabelSet (struct YButton *self)
{
  widget_reconfigure (ybuttonToWidget (self));
  widget_repaint (ybuttonToWidget (self), NULL);
}

/* PROPERTY HOOK
 * pressed
 */
static void
ybuttonPressedSet (struct YButton *self)
{ 
  if (safeGetProperty(self, pressed, 0))
    self -> state = BUTTON_STATE_PRESSED;
  else
    self -> state = BUTTON_STATE_NORMAL;
  widget_repaint (ybuttonToWidget (self), NULL);
}

/*
 * Classes which use YButton as a super class need this function as
 * the setProperty and getProperty are definations not available to other classes
 */
void
ybuttonSetPropertyPressed (struct YButton *self, int32_t value)
{
  setProperty(self, pressed, value);
  if (value)
    self -> state = BUTTON_STATE_PRESSED;
  else
    self -> state = BUTTON_STATE_NORMAL;
}

/*
 * Get property wrapper
 */
int32_t
ybuttonGetPropertyPressed (struct YButton *self)
{
  return safeGetProperty(self, pressed, 0);
}

static int
ybuttonPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t b, bool pressed)
{
  struct YButton *self = widgetToYButton(self_w);
  
  if (b==0 && self -> bin.container.widget.state != WIDGET_STATE_DISABLED)
    {
      if (pressed)
        {
          pointerGrab (self_w);
          self -> state = BUTTON_STATE_PRESSED;
          self -> bin.container.widget.state = WIDGET_STATE_PRESSED;
          objectEmitSignal (ybutton_to_object (self), "pressed");
          setProperty(self, pressed, 1);
          widget_repaint (self_w, NULL); 
        }
      else
        {
          pointerRelease ();
          if (self -> state == BUTTON_STATE_PRESSED)
            {
              objectEmitSignal (ybutton_to_object (self), "clicked");
            }
          if (widget_contains_point_local (self_w, x, y))
            self -> bin.container.widget.state = WIDGET_STATE_HOVER;
          else
            self -> bin.container.widget.state = WIDGET_STATE_NORMAL;
            
          objectEmitSignal (ybutton_to_object (self), "released");
          setProperty(self, pressed, 0);
          
          widget_repaint (self_w, NULL);
        }
    }
  return 1;
}

static int
ybuttonPointerMotion (struct Widget *self_w, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
  struct YButton *self = widgetToYButton (self_w);
  if (self -> state == BUTTON_STATE_PRESSED
      && !widget_contains_point_local (self_w, x, y))
    {
      self -> bin.container.widget.state = WIDGET_STATE_CANCELLING;
      widget_repaint (self_w, NULL);
    }
  else if (self -> bin.container.widget.state == WIDGET_STATE_CANCELLING
      && widget_contains_point_local (self_w, x, y))
    {
      self -> state = BUTTON_STATE_PRESSED;
      widget_repaint (self_w, NULL);
    }
  return 1;
}

static void
ybuttonPointerEnter (struct Widget *self_w, int32_t x, int32_t y)
{
  struct YButton *self = widgetToYButton (self_w);
  
  if (self->bin.container.widget.state == WIDGET_STATE_DISABLED)
    return; //do nothing if widget is disabled
  
  self -> bin.container.widget.state = WIDGET_STATE_HOVER;
  widget_repaint (ybuttonToWidget (self), NULL);
}

static void
ybuttonPointerLeave (struct Widget *self_w)
{
  struct YButton *self = widgetToYButton (self_w);
  
  if (self->bin.container.widget.state == WIDGET_STATE_DISABLED)
    return; //do nothing if widget is disabled
    
  self -> bin.container.widget.state = WIDGET_STATE_NORMAL;
  widget_repaint (ybuttonToWidget (self), NULL);
}

/* arch-tag: 791d4e69-40ee-48d3-885f-fcb076464847
 */
