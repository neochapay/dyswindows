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

#pragma interface

#ifndef Y_CPP_YBIN_H
#define Y_CPP_YBIN_H

#include <Y/c++/connection.h>
#include <Y/c++/widget.h>

#include <Y/c++/YBin.yh>

namespace Y
{
  /** \brief Push button
   * \ingroup remote
   */
  class YBin : public Y::ServerObject::YBin
  {
  public:
    YBin (Y::Connection *y);
    virtual ~YBin ();
        
    void DESTROY (void);
  protected:
    YBin (Y::Connection *y, std::string className);
    virtual bool onEvent (const std::string &, const Y::Message::Members&);
  };
}

#endif