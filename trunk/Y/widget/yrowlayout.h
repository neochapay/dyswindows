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

#ifndef Y_WIDGET_YROWLAYOUT_H
#define Y_WIDGET_YROWLAYOUT_H

struct YRowLayout;

#include <Y/y.h>
#include <Y/widget/widget.h>
#include <Y/widget/ycontainer.h>
#include <Y/util/llist.h>


struct YContainer *  yrowlayout_to_container (struct YRowLayout *);
struct Widget *      yrowlayout_to_widget  (struct YRowLayout *);
struct Object *      yrowlayout_to_object  (struct YRowLayout *);


#endif /* Y_WIDGET_YROWLAYOUT_H */
