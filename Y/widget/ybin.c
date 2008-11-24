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
  * This is the YBin class
  * For now it doesn't do much except help to recreate the GTK class hierarchy
  * - DN
  */
  
#include <Y/widget/ybin.h>
#include <Y/widget/widget_p.h>


DEFINE_CLASS(YBin);
#include "YBin.yc"

/* SUPER
 * YContainer
 */

void
CLASS_INIT (struct YBin *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
}
 
static inline struct YBin *
castBack (struct YContainer *container)
{
  /* assert ( container -> c == ybinClass ); */
  return (struct YBin *)container;
}

static inline const struct YBin *
constcastBack (struct YContainer *container)
{
  /* assert ( container -> c == ybinClass ); */
  return (const struct YBin *)container;
}

struct YBin * 
ycontainerToYBin  (struct YContainer *container)
{
  return castBack (container);
}

