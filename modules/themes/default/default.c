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

#include "window.h"
#include "draw.h"
#include "font.h"

static struct Theme default_theme =
{
  name:           "Default Theme",

  drawBackgroundPane:  default_draw_background_pane,
  drawLabel:           default_draw_label,

  drawYButton:         default_draw_ybutton,
  drawYCheckbox:       default_draw_ycheckbox,
  drawYRadioButton:    default_draw_yradiobutton,
  
  getDefaultFont:      default_get_default_font,
  set_default_font:    default_set_default_font,

  windowInit:          default_window_init,
  windowPaint:         default_window_paint,
  windowGetRegion:     default_window_get_region,
  windowPointerMotion: default_window_pointer_motion,
  windowPointerButton: default_window_pointer_button,
  windowReconfigure:   default_window_reconfigure,
  windowResize:        default_window_resize

};

int
initialise (struct Module *module, const struct Tuple *args)
{
  static char moduleName[] = "Default Theme";
  module -> name = moduleName;
  module -> data = &default_theme;

  default_initialise_font ();

  themeAdd (&default_theme);
  return 0;
}

int
finalise (struct Module *module)
{
  default_finalise_font ();
  themeRemove (&default_theme);
  return 0;
}

