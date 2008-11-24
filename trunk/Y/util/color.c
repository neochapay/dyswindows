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

#include <math.h>
#include <Y/util/color.h>

/*
 * Global predefined colors
 */
YColor Y_WHITE = {
  1.0, //red
  1.0, //green
  1.0, //blue
  1.0  //alpha
};

YColor Y_BLACK = {
  0.0, //red
  0.0, //green
  0.0, //blue
  1.0  //alpha
};

YColor Y_OPAQUE = {
  0.0, //red
  0.0, //green
  0.0, //blue
  0.0  //alpha
};

YColor
createColor(double red, double green, double blue, double alpha)
{
  YColor color;
  initColorDouble(&color, red, green, blue, alpha);
  return color;
}

YColor
createColorInt32(uint32_t value)
{
  YColor color;
  initColorInt32(&color, value);
  return color;
}
/*
 * Loads a color based on a single 32bit int.
 * This is really just here for compatability with old
 * style colors..  Should be removed..
 * assumed to be rgba with 8 bits per color.
 */
void 
initColorInt32(YColor *color, uint32_t value) 
{
  uint8_t a = (value & 0xFF000000) >> 24;
  uint8_t r = (value & 0x00FF0000) >> 16;
  uint8_t g = (value & 0x0000FF00) >> 8;
  uint8_t b = (value & 0x000000FF);
  initColorInt8(color, r, g, b, a);
}

/*
 * loads color based on double values..
 */
void initColorDouble(YColor *color, double red, 
		     double green, double blue, 
		     double alpha) 
{
  if (!color)
    {
      //      Y_TRACE ("Color is NULL!");
      return;
    }
  
  //we set the double values..  Making sure that
  //values are not > 1.0 and not < 0.0.
  color->red = red < 0.0 ? 0.0 : (red > 1.0 ? 1.0 : red);
  color->green = green < 0.0 ? 0.0 : (green > 1.0 ? 1.0 : green);
  color->blue = blue < 0.0 ? 0.0 : (blue > 1.0 ? 1.0 : blue);
  color->alpha = alpha < 0.0 ? 0.0 : (alpha > 1.0 ? 1.0 : alpha);
}

void initColorInt8(YColor *color, uint8_t red, uint8_t green, 
		   uint8_t blue, uint8_t alpha)
{

  /*
   * Generate double values
   */
  double r = red/255.0;
  double g = green/255.0;
  double b = blue/255.0;
  double a = alpha/255.0;
  initColorDouble(color, r, g, b, a);
}
