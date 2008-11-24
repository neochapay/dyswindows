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

#pragma interface

#ifndef Y_CPP_VALUE_H
#define Y_CPP_VALUE_H

#define ybool uint32_t

#include <Y/c++/reply.h>

#include <utility>

namespace Y
{
  template <typename T> T getValue(const Y::Message::Members& m, int index);
  template <typename T1, typename T2> std::pair<T1, T2> getValue(const Y::Message::Members& m, int index)
    {
      return std::pair<T1, T2>(Y::getValue<T1>(m, index), Y::getValue<T2>(m, index + 1));
    }

  /** \brief Remote property value
   * \ingroup remote
   *
   * A template thunk around a Y::Reply object; it behaves like
   * Y::Reply, but converts the reply tuple into the relevant type
   */
  template <typename T> class Value
    {
    public:
      virtual ~Value ();

      Value(Reply *rep_) : rep(rep_) {}

      bool hasValue() const {return rep->hasTuple();}
      T value()
      {
        if (rep)
          {
            v = getValue<T>(rep->tuple(), 0);
            delete rep;
            rep = NULL;
          }
        return v;
      }
    private:
      Reply* rep;
      T v;
    };

  template <typename T1, typename T2> class Value<std::pair<T1, T2> >
    {
    public:
      virtual ~Value ();

      Value(Reply *rep_) : rep(rep_) {}

      bool hasValue() const {return rep->hasTuple();}
      std::pair<T1, T2> value()
      {
        if (rep)
          {
            v = getValue<T1, T2>(rep->tuple(), 0);
            delete rep;
            rep = NULL;
          }
        return v;
      }
    private:
      Reply* rep;
      std::pair<T1, T2> v;
    };
}

template <typename T>
Y::Value<T>::~Value()
{
  delete rep;
}

template <typename T1, typename T2>
  Y::Value<std::pair<T1, T2> >::~Value()
{
  delete rep;
}

#endif

/* arch-tag: f3af3c79-065a-47d8-ba02-a5d0764a0cff
 */
