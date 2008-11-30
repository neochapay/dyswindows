#ifndef Y_BUTTON_EAU_H
#define Y_BUTTON_EAU_H

#include <Y/buffer/painter.h>
#include <Y/widget/widget.h>

void default_draw_ybutton_pane (struct Painter *, int32_t, int32_t, int32_t, int32_t,
                     	enum WidgetState, enum ButtonState);
void default_draw_ybutton (struct Painter *, struct YButton *);

#endif
