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

#include <Y/modules/module_interface.h>
#include <Y/modules/theme_interface.h>

#include <Y/util/yutil.h>
#include <Y/text/font.h>

#include <cairo.h>
#include "font.h"

static struct Font *dustinDefaultFont;

void
default_initialise_font (void)
{
    dustinDefaultFont = NULL;
}

void
default_finalise_font (void)
{
    if (dustinDefaultFont != NULL) {
        fontDestroy (dustinDefaultFont);
    }
}

struct Font *
default_get_default_font (void)
{
  if (dustinDefaultFont == NULL)
    dustinDefaultFont = fontCreate ("Bitstream Vera Sans", "Roman", 12);
  return dustinDefaultFont;
}

void
default_set_default_font (struct Painter *painter)
{
  cairo_t *cr = painter->cairo_context; //get the cairo context
  if (!cr)
    return;
  cairo_select_font_face (cr, "Bitstream Vera Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 12.0);
}
