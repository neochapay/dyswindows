/************************************************************************
 *   Copyright (C) Andrew Suffield <asuffield@debian.org>
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

#pragma implementation

#include <Y/c++/value.h>

#include <utility>

namespace Y
{
  template <>
  std::string
  getValue<std::string>(const Message::Members& m, int index)
  {
    return m.at(index).string();
  }

  template <>
  uint32_t
  getValue<uint32_t>(const Message::Members& m, int index)
  {
    return m.at(index).uint32();
  }

  template <>
  int32_t
  getValue<int32_t>(const Message::Members& m, int index)
  {
    return m.at(index).int32();
  }
}
/* arch-tag: 69c01399-c14c-42c9-8c2d-f1304993f033
 */
