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

#ifndef Y_YRADIOBUTTON_H
#define Y_YRADIOBUTTON_H

#include <Y/y.h>
#include <Y/widget/widget.h>
#include <Y/buffer/painter.h>
#include <Y/widget/ytogglebutton.h>

struct YRadioButton
{
  struct YToggleButton togglebutton;
  struct YRadioGroup *group;
};

void yradiobuttonInit (struct YRadioButton *self);
struct YRadioButton * ytogglebuttonToYRadioButton (struct YToggleButton *button);
struct YRadioButton * widgetToYRadioButton (struct Widget *self);
struct Widget * yradiobuttonToWidget (struct YRadioButton *self);
struct Object * yradiobutton_to_object (struct YRadioButton *self);

void yradiobuttonSetPropertyPressed (struct YRadioButton *self, int32_t value);
int32_t yradiobuttonGetPropertyPressed (struct YRadioButton *self);

#endif 

