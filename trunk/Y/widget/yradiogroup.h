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

#ifndef Y_YRADIOGROUP_H
#define Y_YRADIOGROUP_H

#include <Y/y.h>
#include <Y/widget/yradiobutton.h>
#include <Y/util/llist.h>

struct YRadioGroup
{
  struct Object o;
  struct YRadioButton *selected; //the currently selected radiobutton
  struct llist *list; //list of all members of this group
  int numElements; //number of elements in this group
};

struct YRadioGroup * yradiogroupCreate (void);
struct YCheckbox * widgetToYCheckbox (struct Widget *self);
struct Object * yradiogroup_to_object (struct YRadioGroup *self);
struct YRadioGroup *objectToYRadioGroup (struct Object *object);
void yradiogroupDestroy (struct YRadioGroup *self);

void groupRemoveRadioButton (struct YRadioGroup *self, struct YRadioButton *rad);
bool groupAddRadioButton (struct YRadioGroup *self, struct YRadioButton *rad);
void groupSetSelectedRadioButton (struct YRadioGroup *self, struct YRadioButton *rad);

#endif /* Y_YRADIOGROUP_H */

