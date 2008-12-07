#include <string.h>

#include <Y/modules/module_interface.h>
#include <Y/modules/theme_interface.h>
#include <Y/buffer/bufferclass.h>
#include <Y/widget/label.h>

#include "yLabel.h"


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

  cairo_select_font_face (cr, "Sans Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 10.0);
  
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
