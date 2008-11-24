/************************************************************************
 *   Copyright (C) Dustin Norlander <dustinn@gmail.com>
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

#ifndef Y_THEMES_DEFAULT_DRAW_H
#define Y_THEMES_DEFAULT_DRAW_H

#include <Y/buffer/painter.h>
#include <Y/widget/widget.h>

void default_draw_background_pane (struct Painter *, int32_t, int32_t, int32_t, int32_t);
void default_draw_ybutton_pane (struct Painter *, int32_t, int32_t, int32_t, int32_t,
                     	enum WidgetState, enum ButtonState);

void default_draw_ybutton (struct Painter *, struct YButton *);
void default_draw_ycheckbox (struct Painter *, struct YCheckbox *);
void default_draw_yradiobutton (struct Painter *, struct YRadioButton *);

void default_draw_label (struct Painter *, struct Label *);

#endif

