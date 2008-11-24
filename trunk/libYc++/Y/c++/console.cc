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

#include <Y/c++/console.h>
#include <Y/c++/connection.h>

#include <string>

Y::Console::Console (Y::Connection *y) : Y::ServerObject::Console(y)
{
  subscribeSignal ("YKBString");
  subscribeSignal ("YKBEvent");
  subscribeSignal ("resize");
}

Y::Console::~Console ()
{
}

void
Y::Console::setRendition (uint32_t bold, uint32_t blink, uint32_t inverse, uint32_t underline,
                          uint32_t foreground, uint32_t background, char charset)
{
  char charsetstring[] = { charset, '\0' };
  Y::ServerObject::Console::setRendition(bold, blink, inverse, underline, foreground, background, charsetstring);
}

bool
Y::Console::onEvent (const std::string &name, const Y::Message::Members& params)
{
  if (name == "YKBString" && params.size () == 2)
    {
      ykbString (params[0].string(), params[1].uint32());
      return true;
    }
  if (name == "YKBEvent" && params.size () == 2)
    {
      ykbEvent (params[0].string(), params[1].uint32());
      return true;
    }
  else if (name == "resize" && params.size () == 2)
    {
      int cols = params[0].uint32();
      int rows = params[1].uint32();
      resize (cols, rows);
      return true;
    }
  return false;
}

/* arch-tag: 3c4bea9a-db16-4a53-b40c-317e2ab8a93a
 */
