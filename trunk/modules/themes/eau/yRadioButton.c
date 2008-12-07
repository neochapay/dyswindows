#include <string.h>

#include <Y/modules/module_interface.h>
#include <Y/modules/theme_interface.h>
#include <Y/buffer/bufferclass.h>
#include <Y/widget/yradiobutton.h>

#include "yRadioButton.h"
#include "yButton.h"


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

  cairo_select_font_face (cr, "Sans Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 10.0);

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
