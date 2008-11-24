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

#ifndef Y_THEMES_DEFAULT_WINDOW_H
#define Y_THEMES_DEFAULT_WINDOW_H

#include <Y/widget/window.h>
#include <Y/buffer/painter.h>

void default_window_init (struct Window *);
void default_window_paint (struct Window *, struct Painter *);
int  default_window_get_region (struct Window *, int32_t, int32_t);
int  default_window_pointer_motion (struct Window *, int32_t, int32_t, int32_t, int32_t);
int  default_window_pointer_button (struct Window *, int32_t, int32_t, uint32_t, bool);
void default_window_reconfigure (struct Window *, int32_t *, int32_t *, int32_t *, int32_t *,
                             int32_t *, int32_t *);
void default_window_resize (struct Window *);

#endif
