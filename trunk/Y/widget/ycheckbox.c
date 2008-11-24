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
 *                                            +---YCheckbox
 */

#include <Y/widget/ycheckbox.h>
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
static void ycheckboxPaint (struct Widget *, struct Painter *);

DEFINE_CLASS(YCheckbox);
#include "YCheckbox.yc"

/* SUPER
 * YToggleButton
 */

/* PROPERTY
 */

static struct WidgetTable ycheckbox_table; 

/*
 * Creates a new checkbox, including allocating memory
 */
static struct YCheckbox *
ycheckboxCreate (void)
{
  struct YCheckbox *self = ymalloc (sizeof (struct YCheckbox));
  objectInitialise (&(self -> togglebutton.button.bin.container.widget.o), CLASS(YCheckbox));
  CLASS_INIT (self, &ycheckbox_table.vtable);
  return self;
}

/*
 * Initialize the checkbox
 */
void
CLASS_INIT (struct YCheckbox *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  struct WidgetTable *tab = (struct WidgetTable *)vtable;
  //initialize the WidgetTable
  tab -> paint = ycheckboxPaint;			
}

static inline struct YCheckbox *
castBack (struct YToggleButton *button)
{
  /* assert ( button -> c == ycheckboxClass ); */
  return (struct YCheckbox *)button;
}

static inline const struct YCheckbox *
castBackConst (const struct YToggleButton *button)
{
  /* assert ( button -> c == ycheckboxClass ); */
  return (const struct YCheckbox *)button;
}

struct YCheckbox * 
ytogglebuttonToYCheckbox (struct YToggleButton *button)
{
  return castBack (button);
}

/*
 * Converts between YCheckbox and Widget
 */
struct YCheckbox *
widgetToYCheckbox (struct Widget *self)
{
  /* assert ( self -> c == ycheckboxClass ); */
  return (struct YCheckbox *)self;
}


struct Widget *
ycheckboxToWidget (struct YCheckbox *self)
{
  return &(self -> togglebutton.button.bin.container.widget);
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
ycheckboxDestroy (struct YCheckbox *self)
{
  widgetFinalise (ycheckboxToWidget (self));
  objectFinalise (ycheckbox_to_object (self));
  yfree (self);
}



struct Object *
ycheckbox_to_object (struct YCheckbox *self)
{
  return &(self -> togglebutton.button.bin.container.widget.o);
}

/* METHOD
 * YCheckbox :: () -> (object)
 */
static struct Object *
ycheckboxInstantiate (void)
{
  return ycheckbox_to_object (ycheckboxCreate ());
}

/*
 * Paints the checkbox
 */
static void
ycheckboxPaint (struct Widget *self_w, struct Painter *painter)
{
  themeDrawYCheckbox (painter, widgetToYCheckbox(self_w));
}

