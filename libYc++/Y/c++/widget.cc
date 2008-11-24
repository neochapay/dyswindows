/************************************************************************
 *   Copyright (C) Mark Thomas <markbt@efaref.net>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma implementation

#include <Y/c++/widget.h>

/*
 * Getter for property enabled
 * Return type bool
 */
bool  
Y::Widget::getEnabled (void)
{
  Y::Value<ybool> *temp = enabled.get();
  ybool val = temp->value();
  if (val)
    return true;
  else
    return false; 
}

/*
 * Setter for property enabled
 * Property type bool
 */
void  
Y::Widget::setEnabled (bool p)
{
  if (p)
    enabled.set(1);
  else
    enabled.set(0);
}

/* arch-tag: 54b74d38-d4c7-4294-a891-172297204a43
 */
