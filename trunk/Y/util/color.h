/************************************************************************
 *   Copyright (C) Dustin Norlander <dustinn@gmail.com>
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

#ifndef Y_COLOR_H
#define Y_COLOR_H

#include <stdint.h>

typedef struct _ycolor {
  
  /*
   * These are the rgba values stored as doubles.
   * each value should be in the range 0.0 - 1.0
   * cairo uses colors defined in doubles.  doubles also
   * afford greater accuracy for >32 bit color spaces.
   */
  double red;
  double green;
  double blue;
  double alpha;
} YColor;


YColor createColorInt32(uint32_t value);

YColor createColor(double red, double green, double blue, double alpha);

void initColorInt32(YColor *color, uint32_t value);

void initColorDouble(YColor *color, double r, double g, double b, double a);

void initColorInt8(YColor *color, uint8_t r, uint8_t g, uint8_t b, uint8_t a);


extern YColor Y_BLACK;

extern YColor Y_WHITE;

extern YColor Y_OPAQUE;


#endif
