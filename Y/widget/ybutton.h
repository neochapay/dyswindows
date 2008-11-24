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

#ifndef Y_YBUTTON_H
#define Y_YBUTTON_H

#include <Y/y.h>
#include <Y/widget/widget.h>
#include <Y/buffer/painter.h>
#include <Y/widget/ybin.h>

enum ButtonState
{
  BUTTON_STATE_NORMAL,
  BUTTON_STATE_PRESSED
};

struct YButton
{
  struct YBin bin;
  enum ButtonState state;
};

/* Prototypes */
void ybuttonInit (struct YButton *self);
struct Widget * ybuttonToWidget  (struct YButton *);
struct Object * ybutton_to_object  (struct YButton *);
struct YButton * ybinToYButton (struct YBin *);
struct YButton * widgetToYButton (struct Widget *);
void  ybuttonSetPropertyPressed (struct YButton *, int32_t);
int32_t ybuttonGetPropertyPressed (struct YButton *);


#endif /* Y_YBUTTON_H */

