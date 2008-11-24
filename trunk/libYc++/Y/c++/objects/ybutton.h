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

#ifndef Y_CPP_YBUTTON_H
#define Y_CPP_YBUTTON_H

#include <Y/c++/connection.h>
#include <Y/c++/widget.h>

#include <Y/c++/YButton.yh>

namespace Y
{
  /** \brief Push button
   * \ingroup remote
   */
  class YButton : public Y::ServerObject::YButton
  {
  public:
    YButton (Y::Connection *y);
    virtual ~YButton ();

    /** Signalled when the button is fully clicked (depressed and
     * released)
     */
     //use Signal+<number parameters><void, param1, param2> functionname
    sigc::signal<void> clicked;
    sigc::signal<void> mousePressed;
    sigc::signal<void> mouseReleased;

    void DESTROY (void);
  protected:
    YButton (Y::Connection *y, std::string t);
    virtual bool onEvent (const std::string &, const Y::Message::Members&);
  };
}

#endif
