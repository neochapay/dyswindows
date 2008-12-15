#ifndef Y_WIDGET_MENU_H
#define Y_WIDGET_MENU_H

struct YMenu;

#include <Y/y.h>
#include <Y/widget/widget.h>

struct Widget * menuToWidget  (struct YMenu *);
struct Object * menuToObject  (struct YMenu *);

void ymenuAddItem (struct YMenu *, uint32_t, const char *, uint32_t);

#endif 
