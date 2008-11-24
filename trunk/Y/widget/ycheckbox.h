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

#ifndef Y_YCHECKBOX_H
#define Y_YCHECKBOX_H

#include <Y/y.h>
#include <Y/widget/widget.h>
#include <Y/buffer/painter.h>
#include <Y/widget/ytogglebutton.h>

struct YCheckbox
{
  struct YToggleButton togglebutton;
};

void ycheckboxInit (struct YCheckbox *self);
struct YCheckbox * ytogglebuttonToYCheckbox (struct YToggleButton *button);
struct YCheckbox * widgetToYCheckbox (struct Widget *self);
struct Widget * ycheckboxToWidget (struct YCheckbox *self);
struct Object * ycheckbox_to_object (struct YCheckbox *self);

#endif 

