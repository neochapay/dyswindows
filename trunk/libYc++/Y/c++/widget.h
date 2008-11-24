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

#pragma interface

#ifndef Y_CPP_WIDGET_H
#define Y_CPP_WIDGET_H

#include <Y/c++/connection.h>
#include <Y/c++/object.h>

#include <string>

namespace Y
{
  /** \brief %Widget
   * \ingroup remote
   */
  class Widget : public Object
  {
  public:
   ~Widget() {}
   bool getEnabled (void);
   void setEnabled (bool);
   
  private:
   Object::Property<ybool> enabled;
   
  protected:
    Widget (Y::Connection *y, std::string className) : Object(y, className), 
       enabled(this, "enabled") {}
  };
}

#endif

/* arch-tag: 22131ec8-8f16-48ac-a938-92ed15cc6f6f
 */
