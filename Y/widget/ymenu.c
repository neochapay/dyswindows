#include <Y/widget/ymenu.h>
#include <Y/widget/widget_p.h>

#include <Y/util/yutil.h>
#include <Y/util/index.h>
#include <Y/buffer/painter.h>

#include <Y/object/class_p.h>

#include <Y/modules/theme.h>
#include <Y/object/object_p.h>
#include <Y/text/font.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>


struct YMenuItem
{
  uint32_t id;
  char *text;
  int32_t yPosition;
  int32_t height;
  struct YMenu *submenu;
};

struct YMenu
{
  struct Widget widget;
  struct Index *items;  
  struct MenuItem *current;
};

static int
ymenuitemKeyFunction (const void *key_v, const void *item_v)
{
  const uint32_t *key = key_v;
  const struct YMenuItem *item = item_v;
  if (*key < item->id)
    return -1;
  else
    return (*key > item->id);
}

static int
ymenuitemComparisonFunction (const void *item1_v, const void *item2_v)
{
  const struct YMenuItem *item1 = item1_v, *item2 = item2_v;
  if (item1->id < item2->id)
    return -1;
  else
    return (item1->id > item2->id);
}

static void
ymenuitemDestructorFunction (void *item_v)
{
  struct MenuItem *item = item_v;
  yfree (item -> text);
  yfree (item);
}

static void ymenuReconfigure (struct Widget *);
static void ymenuPaint (struct Widget *, struct Painter *);
static int ymenuPointerMotion (struct Widget *, int32_t, int32_t, 
int32_t, int32_t);
static int ymenuPointerButton (struct Widget *, int32_t, int32_t, 
uint32_t, bool);
static void ymenuPointerEnter (struct Widget *, int32_t, int32_t);
static void ymenuPointerLeave (struct Widget *);

DEFINE_CLASS(YMenu);
#include "YMenu.yc"

static struct WidgetTable ymenuTable =
{
  reconfigure:   ymenuReconfigure,
  paint:         ymenuPaint,
  pointerMotion: ymenuPointerMotion,
  pointerButton: ymenuPointerButton,
  pointerEnter:  ymenuPointerEnter,
  pointerLeave:  ymenuPointerLeave
};

static inline struct YMenu *
castBack (struct Widget *widget)
{
  /* assert ( widget -> c == YmenuClass ); */
  return (struct YMenu *)widget;
}

static inline const struct YMenu *
castBackConst (const struct Widget *widget)
{
  /* assert ( widget -> c == YmenuClass ); */
  return (const struct YMenu *)widget;
}

static void
ymenuPaint (struct Widget *self_w, struct Painter *painter)
{
  struct YMenu *self = castBack (self_w);
  struct Font *font;
  int yp;
  int ascender, offset;
  struct IndexIterator *iter;

  font = themeGetDefaultFont ();
  fontGetMetrics (font, &ascender, NULL, NULL);
  iter = indexGetStartIterator (self -> items);
  while (indexiteratorHasValue (iter))
    {
      struct YMenuItem *item = indexiteratorGet (iter);
      yp = item -> yPosition + 2;
      if (strcmp (item -> text, "-") == 0)
        {
          painterSetPenColour (painter, 0x80000000);
          painterDrawHLine (painter, 0, yp - 1, self -> widget.w);
          painterSetPenColour (painter, 0x80FFFFFF);
          painterDrawHLine (painter, 0, yp, self -> widget.w);
        }
      else
        {
          painterSaveState (painter);
          if (self -> current == item)
            themeDrawButtonPane (painter, 0, item -> yPosition,
                                 self -> widget.w, item -> height,
                                 self -> widget.state);
          painterSetPenColour (painter, 0xFF000000);
          fontMeasureString (font, item -> text, &offset, NULL, NULL);
          fontRenderString (font, painter, item -> text, 4 - offset, yp 
+ ascender);
          painterRestoreState (painter);
        }
      indexiteratorNext (iter);
    }
  indexiteratorDestroy (iter);
}

static struct YMenu *
ymenuCreate (void)
{
  struct YMenu *self = ymalloc (sizeof (struct YMenu));
  objectInitialise (&(self -> widget.o), CLASS(YMenu));
  widgetInitialise (&(self -> widget), &ymenuTable);
  self -> items = indexCreate (ymenuitemKeyFunction, 
ymenuitemComparisonFunction);
  self -> current = NULL;
  return self;
}


static void
ymenuDestroy (struct YMenu *self)
{
  indexDestroy (self -> items, ymenuitemDestructorFunction);
  widgetFinalise (ymenuToWidget (self));
  objectFinalise (ymenuToObject (self));
  yfree (self);
}

struct Widget *
ymenuToWidget (struct YMenu *self)
{
  return &(self -> widget);
}

struct Object *
ymenuToObject (struct YMenu *self)
{
  return &(self -> widget.o);
}


static struct Object *
ymenuInstantiate (void)
{
  return ymenuToObject (ymenuCreate ());
}

static void ymenuReconfigure (struct Widget *self_w)
{
  struct YMenu *self = castBack (self_w);
  struct Font *font;
  int32_t maxWidth = 0;
  int32_t height = 0;
  int ascender, descender, linegap, width, offset;
  struct IndexIterator *iter;
  font = themeGetDefaultFont ();
  fontGetMetrics (font, &ascender, &descender, &linegap);
  iter = indexGetStartIterator (self -> items);
  while (indexiteratorHasValue (iter))
    {
      struct YMenuItem *item = indexiteratorGet (iter);
      if (strcmp (item -> text, "-") == 0)
        {
          item -> yPosition = height;
          item -> height = 4;
          height += 4;
        }
      else
        {
          fontMeasureString (font, item -> text, &offset, &width, NULL);
          if ((width - offset) > maxWidth)
            maxWidth = width - offset;
          item -> yPosition = height;
          item -> height = ascender + descender + 4;
          height += ascender + descender + 4;
        }
      indexiteratorNext (iter);
    }
  indexiteratorDestroy (iter);

  self -> widget.minWidth  = maxWidth + 8;
  self -> widget.minHeight = height;
  self -> widget.reqWidth  = maxWidth + 8;
  self -> widget.reqHeight = height;
  self -> widget.maxHeight = height;

  widgetReconfigure (self -> widget.container);
}

static struct YMenuItem *
ymenuItemAt (struct YMenu *self, int32_t x, int32_t y)
{
  struct IndexIterator *iter = indexGetStartIterator (self -> items);
  while (indexiteratorHasValue (iter))
    {
      struct YMenuItem *item = indexiteratorGet (iter);
      if (item -> yPosition <= y && y < item -> yPosition + item -> 
height)
        {
          indexiteratorDestroy (iter);
          return item;
        }

      indexiteratorNext (iter);
    }
  indexiteratorDestroy (iter);
  return NULL; 
}

int
ymenuPointerButton (struct Widget *self_w, int32_t x, int32_t y, uint32_t 
b, bool pressed)
{
  struct YMenu *self = castBack (self_w);
  if (b==0)
    {
      if (pressed)
        {
          pointerGrab (ymenuToWidget (self));
          self -> widget.state = WIDGET_STATE_PRESSED;
          self -> current = ymenuItemAt (self, x, y);
          if (self -> current != NULL)
            widgetRepaint (ymenuToWidget (self),
                rectangleCreate (0, self -> current -> yPosition,
                    self -> widget.w, self -> current -> height));
        }
      else
        {
          pointerRelease ();
          if (self -> current != NULL)
            {
              objectEmitSignal (ymenuToObject (self), "clicked", 
tb_uint32(self->current->id));
              widgetRepaint (ymenuToWidget (self),
                  rectangleCreate (0, self -> current -> yPosition,
                      self -> widget.w, self -> current -> height));
            }
          if (widgetContainsPoint (ymenuToWidget (self), x, y))
            self -> widget.state = WIDGET_STATE_HOVER;
          else
            {
              self -> widget.state = WIDGET_STATE_NORMAL;
              self -> current = NULL;
            }
        }
    }
  return 1; 
}

int
ymenuPointerMotion (struct Widget *self_w, int32_t x, int32_t y, int32_t 
dx, int32_t dy)
{
  struct YMenu *self = castBack (self_w);
  if (self -> widget.state == WIDGET_STATE_PRESSED &&
      !widgetContainsPoint (ymenuToWidget (self), x, y))
    {
      self -> widget.state = WIDGET_STATE_CANCELLING;
      if (self -> current != NULL)
        widgetRepaint (ymenuToWidget (self),
            rectangleCreate (0, self -> current -> yPosition,
                self -> widget.w, self -> current -> height));
      self -> current = NULL;
    }
  else if (widgetContainsPoint (ymenuToWidget (self), x, y))
    {
      struct YMenuItem *item = ymenuItemAt (self, x, y);
      if (item != NULL && strcmp (item -> text, "-") == 0)
        item = NULL;

      if (item != NULL && self -> widget.state == 
WIDGET_STATE_CANCELLING)
        self -> widget.state = WIDGET_STATE_PRESSED;

      if (item != self -> current)
        {
          if (self -> current != NULL)
            {
              widgetRepaint (ymenuToWidget (self),
                  rectangleCreate (0, self -> current -> yPosition,
                      self -> widget.w, self -> current -> height));
            }
          if (item != NULL)
            {
              widgetRepaint (ymenuToWidget (self),
                  rectangleCreate (0, item -> yPosition,
                      self -> widget.w, item -> height));
            }
          self -> current = item;
        }
      
    }
  return 1;
}

void
ymenuPointerEnter (struct Widget *self_w, int32_t x, int32_t y)
{
  struct YMenu *self = castBack (self_w);
  self -> widget.state = WIDGET_STATE_HOVER;
  self -> current = ymenuItemAt (self, x, y);
  if (self -> current != NULL)
    widgetRepaint (ymenuToWidget (self),
        rectangleCreate (0, self -> current -> yPosition,
            self -> widget.w, self -> current -> height));
}

void
ymenuPointerLeave (struct Widget *self_w)
{
  struct YMenu *self = castBack (self_w);
  self -> widget.state = WIDGET_STATE_NORMAL;
  if (self -> current != NULL)
    widgetRepaint (ymenuToWidget (self),
        rectangleCreate (0, self -> current -> yPosition,
            self -> widget.w, self -> current -> height));
  self -> current = NULL; 
}  

void
ymenuAddItem (struct YMenu *self, uint32_t id, const char *text, uint32_t 
submenuId)
{
  struct Object *submenu;
  struct YMenuItem *item;
  
  if (indexFind (self -> items, &id) != NULL)
    return;

  submenu = objectFind (submenuId);
  if (submenu != NULL && !classInherits (objectClass (submenu), 
CLASS(YMenu)))
    return; 

  item = ymalloc (sizeof (struct YMenuItem));
  item -> id = id;
  item -> text = ystrdup (text);
  item -> height = 0;
  item -> submenu = (struct YMenu *) submenu; 
  indexAdd (self -> items, item);
  widgetReconfigure (ymenuToWidget (self));
}


static void
ymenuCAddItem (struct YMenu *self, uint32_t id, uint32_t text_len, const 
char *text, uint32_t submenu)
{
  ymenuAddItem (self, id, text, submenu);
}
