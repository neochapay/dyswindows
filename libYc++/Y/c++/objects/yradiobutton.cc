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

#include <Y/c++/objects/yradiobutton.h>
#include <Y/c++/connection.h>

#include <string>

Y::YRadioButton::YRadioButton (Y::Connection *y) : Y::ServerObject::YRadioButton(y)
{
  subscribeSignal ("pressed");
  subscribeSignal ("released");
}

/* Protected constructor
 */
Y::YRadioButton::YRadioButton (Y::Connection *y, std::string t) : Y::ServerObject::YRadioButton(y, t)
{
}

Y::YRadioButton::~YRadioButton ()
{
}

bool
Y::YRadioButton::onEvent (const std::string &name, const Y::Message::Members& params)
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
Y::YRadioButton::DESTROY (void)
{
  Y::Message::Members v;
  v.push_back("DESTROY");
  invokeMethod (v, false);
}

