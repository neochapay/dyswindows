#ifndef WINDOW_DECO_EAU_H
#define WINDOW_DECO_EAU_H

#include <Y/widget/window.h>
#include <Y/buffer/painter.h>

void default_window_init (struct Window *);
void default_window_paint (struct Window *, struct Painter *);
int  default_window_get_region (struct Window *, int32_t, int32_t);
int  default_window_pointer_motion (struct Window *, int32_t, int32_t, int32_t, int32_t);
int  default_window_pointer_button (struct Window *, int32_t, int32_t, uint32_t, bool);
void default_window_reconfigure (struct Window *, int32_t *, int32_t *, int32_t *, int32_t *,
                             int32_t *, int32_t *);
void default_window_resize (struct Window *);

void default_draw_background_pane (struct Painter *, int32_t, int32_t, int32_t, int32_t);

#endif
