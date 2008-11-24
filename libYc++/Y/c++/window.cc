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

#include <Y/c++/window.h>
#include <Y/c++/connection.h>

#include <string>

Y::Window::Window (Y::Connection *y) : Y::ServerObject::Window(y)
{
  subscribeSignal ("requestClose");
}

Y::Window::Window (Y::Connection *y, std::string t) : Y::ServerObject::Window(y)
{
  subscribeSignal ("requestClose");
  setTitle(t);
}

Y::Window::~Window ()
{
}

void
Y::Window::setChild (Widget *w)
{
  if (w != NULL)
    {
      Y::ServerObject::Window::setChild(*w);
      w->parent = this;
    }
}

void
Y::Window::setFocussed (Widget *w)
{
  if (w != NULL)
    {
      Y::ServerObject::Window::setFocussed(*w);
    }
}

bool
Y::Window::onEvent (const std::string &name, const Y::Message::Members& params)
{
  if (name == "requestClose")
    {
      requestClose ();
      return true;
    }
  return false;
}

/* arch-tag: 4952e419-7783-4f6c-86cc-cf90f1a077b5
 */
