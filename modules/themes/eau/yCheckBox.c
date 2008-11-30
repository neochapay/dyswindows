#include <string.h>

#include <Y/modules/module_interface.h>
#include <Y/modules/theme_interface.h>
#include <Y/buffer/bufferclass.h>
#include <Y/widget/ycheckbox.h>

#include "yCheckBox.h"
#include "yButton.h"

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
