/************************************************************************
 *   Copyright (C) Dustin Norlander <dustin@dustismo.com>
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

#include <Y/c++/objects/ytogglebutton.h>
#include <Y/c++/connection.h>

#include <string>

Y::YToggleButton::YToggleButton (Y::Connection *y) : Y::ServerObject::YToggleButton(y)
{
  subscribeSignal ("pressed");
  subscribeSignal ("released");
}

/* Protected constructor
 */
Y::YToggleButton::YToggleButton (Y::Connection *y, std::string t) : Y::ServerObject::YToggleButton(y, t)
{
}

Y::YToggleButton::~YToggleButton ()
{
}

bool
Y::YToggleButton::onEvent (const std::string &name, const Y::Message::Members& params)
{
  if (name == "pressed")
    {
      buttonPressed();
      return true;
    }
  else if (name == "released")
    {
      buttonReleased();
      return true;
    }
  return false;
}

void
Y::YToggleButton::DESTROY (void)
{
  Y::Message::Members v;
  v.push_back("DESTROY");
  invokeMethod (v, false);
}

