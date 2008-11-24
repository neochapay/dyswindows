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

#include <Y/c++/objects/ycontainer.h>
#include <Y/c++/connection.h>

#include <string>

Y::YContainer::YContainer (Y::Connection *y) : Y::ServerObject::YContainer(y)
{
}

Y::YContainer::YContainer (Y::Connection *y, std::string className) : Y::ServerObject::YContainer(y, className)
{
}

Y::YContainer::~YContainer ()
{
}

bool
Y::YContainer::onEvent (const std::string &name, const Y::Message::Members& params)
{
  return false;
}

void
Y::YContainer::DESTROY (void)
{
  Y::Message::Members v;
  v.push_back("DESTROY");
  invokeMethod (v, false);
}

