#include <string.h>

#include <Y/modules/module_interface.h>
#include <Y/modules/theme_interface.h>
#include <Y/buffer/bufferclass.h>
#include <Y/widget/ybutton.h>

#include "yButton.h"

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
