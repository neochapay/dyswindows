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

#include <string.h>

#include <Y/modules/module_interface.h>
#include <Y/modules/theme_interface.h>

#include <Y/text/font.h>
#include <Y/util/yutil.h>
#include <Y/util/log.h>
#include <Y/util/rectangle.h>
#include <Y/buffer/painter.h>
#include <Y/object/class.h>
#include <Y/object/object.h>
#include <Y/widget/widget.h>
#include <Y/widget/ybutton.h>
#include <Y/widget/label.h>
#include <Y/modules/theme.h>

#include <Y/buffer/bufferclass.h>

#include "draw.h"
#include "font.h"

void
default_draw_background_pane (struct Painter *painter, int32_t x, int32_t y, int32_t w, int32_t h)
{
}

/*
 * This draws a YButton pane.
 */
void
default_draw_ybutton_pane (struct Painter *painter, 
		       int32_t x, int32_t y, int32_t w, int32_t h,
		       enum WidgetState w_state, enum ButtonState b_state)
{

  cairo_t *cr = painter->cairo_context; //get the cairo context


  if (w_state == WIDGET_STATE_HOVER
      || w_state == WIDGET_STATE_CANCELLING)
    cairo_set_source_rgb (cr, 0.0, 0.3, 1.0); //bluish for hover or cancelling
  else if (b_state == BUTTON_STATE_PRESSED)
    cairo_set_source_rgb (cr, .5, .5, .5); //grey for pressed
  else if (w_state == WIDGET_STATE_DISABLED)
    cairo_set_source_rgb (cr, .75, .75, .75);
  else
    cairo_set_source_rgb (cr, 1.0, 1.0, 1.0); //white otherwise

  cairo_set_line_width (cr, 2);
  int x_p = x;
  int y_p = y;
  painter_translate_xy(painter, &x_p, &y_p);
  cairo_rectangle (cr, x_p, y_p, w, h);
  cairo_fill_preserve(cr);

  if (w_state == WIDGET_STATE_DISABLED)
    cairo_set_source_rgb (cr, .25, .25, .25);
  else
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0); 
  cairo_stroke(cr);
}

/*
 * Drawing a YButton
 * -DN
 */

void
default_draw_ybutton(struct Painter *painter, struct YButton *button)
{
  int offset, width, ascender, descender;
  const struct Value *textValue = objectGetProperty (ybutton_to_object (button), "label");
  const char *text = textValue ? textValue->string.data : "";
  struct Rectangle *rect = widget_get_rectangle (ybuttonToWidget (button));
  enum WidgetState state = widget_get_state (ybuttonToWidget (button));
  cairo_t *cr = painter->cairo_context; //get the cairo context

  painter_save_state (painter);
  
  default_draw_ybutton_pane (painter, 0, 0, rect->w, rect->h, state, button->state);
  if (state == WIDGET_STATE_DISABLED)
    cairo_set_source_rgb (cr, .5, .5, .5); //grey for disabled
  else
    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0); //black text
  
  cairo_select_font_face (cr, "Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 12.0);
  cairo_text_extents_t extents;
  cairo_text_extents (cr, text, &extents);
  int x = (rect->w - extents.width) / 2;
  int y = (rect->h + extents.height) / 2;
   
  painter_translate_xy(painter, &x, &y);  
  cairo_move_to (cr, x, y);
  cairo_show_text (cr, text);
  painter_restore_state (painter);
  rectangleDestroy(rect);
}

/*
 * Drawing a YCheckbox
 */
void
default_draw_ycheckbox(struct Painter *painter, struct YCheckbox *checkbox)
{
  
  int offset, ascender, descender;
  const struct Value *textValue = objectGetProperty (ycheckbox_to_object (checkbox), "label");
  const char *text = textValue ? textValue->string.data : "";
  struct Rectangle *rect = widget_get_rectangle (ycheckboxToWidget (checkbox));
  enum WidgetState state = widget_get_state (ycheckboxToWidget (checkbox));

  cairo_t *cr = painter->cairo_context; //get the cairo context

  painter_save_state (painter);

  cairo_select_font_face (cr, "Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 12.0);
  cairo_text_extents_t extents;
  cairo_text_extents (cr, text, &extents);

  int x = 20;
  int y = (rect -> h + extents.height) / 2;

  painter_translate_xy (painter, &x, &y);
  cairo_move_to (cr, x, y);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0); //black text
  cairo_show_text (cr, text);

  default_draw_ybutton_pane (painter, 0, (rect->h - 16) / 2,
                                16, 16, state, BUTTON_STATE_NORMAL);
  const struct Value *checkedValue = objectGetProperty (ycheckbox_to_object (checkbox), "pressed");
  uint32_t checked = checkedValue ? checkedValue->uint32 : 0;
  
  if (checked)
    {
      int yp = (rect->h - 16) / 2;
      int xp = 0;
      painter_translate_xy(painter, &xp, &yp);
      cairo_set_source_rgb(cr, 0,0,0);
      cairo_move_to(cr,xp,yp);
      cairo_line_to(cr,xp+16,yp+16);
      cairo_stroke(cr);
      cairo_move_to(cr,xp+16,yp);
      cairo_line_to(cr,xp,yp+16);
      cairo_stroke(cr);
    }
  
  painter_restore_state (painter);

  rectangleDestroy(rect);
 
}


/*
 * Drawing a YRadioButton
 *
 * -DN
 */
void
default_draw_yradiobutton(struct Painter *painter, struct YRadioButton *radiobutton)
{
  const struct Value *textValue = objectGetProperty (yradiobutton_to_object (radiobutton), "label");
  const char *text = textValue ? textValue->string.data : "";
  struct Rectangle *rect = widget_get_rectangle (yradiobuttonToWidget (radiobutton));
  enum WidgetState state = widget_get_state (yradiobuttonToWidget (radiobutton));
  cairo_t *cr = painter->cairo_context; //get the cairo context

  painter_save_state (painter);

  int xp, yp;

  default_draw_ybutton_pane (painter, 0, 0, rect->w, rect->h, state, 
		  radiobutton->togglebutton.button.state);

  cairo_select_font_face (cr, "Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 12.0);

  cairo_text_extents_t extents;
  cairo_text_extents (cr, text, &extents);

  xp = 20;
  yp = (rect->h + extents.height) / 2;

  painter_translate_xy (painter, &xp, &yp);
  cairo_move_to (cr, xp, yp);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0); //black text
  cairo_show_text (cr, text);


  painter_restore_state (painter);

  rectangleDestroy(rect);
}

void
default_draw_label(struct Painter *painter, struct Label *label)
{
  const struct Value *textValue = objectGetProperty (label_to_object (label), "text");
  const char *text = textValue ? textValue->string.data : "";
  const struct Value *alignmentValue = objectGetProperty (label_to_object (label), "alignment");
  const char *alignment = alignmentValue ? alignmentValue->string.data : NULL;
  struct Rectangle *rect = widget_get_rectangle (labelToWidget (label));
  enum WidgetState state = widget_get_state (labelToWidget (label));
  cairo_t *cr = painter->cairo_context; //get the cairo context

  int xp, yp;

  painter_save_state (painter);

  cairo_select_font_face (cr, "Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 12.0);
  
  cairo_text_extents_t extents;
  cairo_text_extents (cr, text, &extents);

  yp = (rect->h + extents.height ) / 2;
  if (alignment && strcasecmp (alignment, "right") == 0)
    xp = (rect->w - extents.width);
  else if (alignment && strcasecmp (alignment, "center") == 0)
    xp = (rect->w - extents.width) / 2;
  else
    xp = 5;
  
  painter_translate_xy (painter, &xp, &yp);
  cairo_move_to (cr, xp, yp);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0); //black text
  cairo_show_text (cr, text);

  painter_restore_state (painter);

  rectangleDestroy(rect);
  
}


