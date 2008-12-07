#include <Y/widget/window.h>
#include <Y/widget/widget.h>
#include <Y/object/object.h>
#include <Y/screen/screen.h>
#include <Y/text/font.h>
#include <Y/modules/windowmanager.h>
#include <Y/modules/theme.h>
#include <Y/buffer/bufferclass.h>
#include <Y/buffer/painter.h>
#include <Y/util/color.h>

#include "windowDeco.h"

enum WindowRegion
{
  WINDOW_REGION_NOTHING,
  WINDOW_REGION_MOVE,
  WINDOW_REGION_MAXIMISE, WINDOW_REGION_RESTORE, WINDOW_REGION_CLOSE,
  WINDOW_REGION_RESIZE_NW, WINDOW_REGION_RESIZE_N, WINDOW_REGION_RESIZE_NE,
  WINDOW_REGION_RESIZE_W,                          WINDOW_REGION_RESIZE_E,
  WINDOW_REGION_RESIZE_SW, WINDOW_REGION_RESIZE_S, WINDOW_REGION_RESIZE_SE,
  WINDOW_REGION_CHILD
};

//some static variables,
//these should be configurable,


static int title_height = 20;
static double window_radius = 0.0;
static double window_edge_width = 1.0;
static double edge_offset = 0.0;
static double edge_button_offset = 10.0;

static int button_size = 10; //buttons are square
static int button_right_edge = 20; //buttons end window_width - 20


void
default_window_init (struct Window *window)
{
}

static void
clear_title_bar(cairo_t *cr, int x, int y, int w, int h)
{
    cairo_save (cr);

    cairo_rectangle (cr, x, y, w, h);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_fill (cr);

    cairo_restore (cr);
}

static void
clear_border(cairo_t *cr, int x, int y, int w, int h)
{
    cairo_save (cr);

    cairo_rectangle (cr, x, y, w, h);
    cairo_set_line_width (cr, window_edge_width*2);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_stroke (cr);

    cairo_restore (cr);
}

/*
 * Adapted from cairo samples,
 * http://cairographics.org/samples/curve_rectangle.html
 */
static void
paint_curved_rectangle (cairo_t *cr,
		     double x0,
		     double y0,
		     double rect_width,
		     double rect_height,
		     double radius)
{
  double x1, y1;
  x1=x0+rect_width;
  y1=y0+rect_height;
  //  if (!rect_width || !rect_height)
  //  return;
  if (rect_width/2<radius) {
    if (rect_height/2<radius) {
      cairo_move_to  (cr, x0, (y0 + y1)/2);
      cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
      cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
    } else {
      cairo_move_to  (cr, x0, y0 + radius);
      cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1)/2, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
      cairo_line_to (cr, x1 , y1 - radius);
      cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0)/2, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
    }
  } else {
    if (rect_height/2<radius) {
      cairo_move_to  (cr, x0, (y0 + y1)/2);
      cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
      cairo_line_to (cr, x1 - radius, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1)/2);
      cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
      cairo_line_to (cr, x0 + radius, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1)/2);
    } else {
      cairo_move_to  (cr, x0, y0 + radius);
      cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
      cairo_line_to (cr, x1 - radius, y0);
      cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
      cairo_line_to (cr, x1 , y1 - radius);
      cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
      cairo_line_to (cr, x0 + radius, y1);
      cairo_curve_to (cr, x0, y1, x0, y1, x0, y1- radius);
    }
  }
  cairo_close_path (cr);
}



static void
paint_title_bar (struct Window *window, struct Painter *painter,
               int x, int y, int w)
{
  /* title bar */
  const struct Value *titleValue = objectGetProperty (window_to_object (window), "title");
  const char *title = titleValue ? titleValue->string.data : "";

  bool selected = (wmSelectedWindow () == window);

  cairo_t *cr = painter->cairo_context;

// background title color
  cairo_set_source_rgb(cr, 0.8,0.8,0.8);
// background title patch
  cairo_move_to (cr,x,y);
  cairo_line_to (cr,x,y+title_height);
  cairo_line_to (cr,x+w,y+title_height);
  cairo_line_to (cr,x+w,y);
  cairo_close_path (cr);
  cairo_fill_preserve (cr);
//line color
  cairo_set_source_rgb (cr, 0.6, 0.6, 0.6);
//line width
  cairo_set_line_width(cr,3);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
  
  cairo_stroke (cr);

  cairo_select_font_face (cr, "Sans Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, 10.0);
  cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
  cairo_move_to (cr, 20, title_height-6);
  cairo_show_text (cr, title);

  /* Paint the expand, close buttons */
  int buttonPosX = x + w - button_right_edge;
  int buttonPosY = (int)(y + (title_height/2)-(button_size/2));

// Create close button
    cairo_move_to(cr,buttonPosX,buttonPosY);
    cairo_line_to(cr,buttonPosX+10,buttonPosY+10);
    cairo_move_to(cr,buttonPosX,buttonPosY+10);
    cairo_line_to(cr,buttonPosX+10,buttonPosY);
    cairo_set_line_width (cr, 2.0);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_stroke (cr);

  buttonPosX -= button_size+edge_button_offset;
  //  Y_TRACE ("drawing expand button at: %d,%d", buttonPosX, buttonPosY);

//Maximize Button
    cairo_move_to(cr,buttonPosX+15,buttonPosY+5);
    cairo_line_to(cr,buttonPosX+10,buttonPosY);
    cairo_line_to(cr,buttonPosX+5,buttonPosY+5);
    cairo_set_line_width (cr, 2.0);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
    cairo_stroke (cr);
}

void
default_window_paint (struct Window *window, struct Painter *painter)
{

  cairo_t *cr = painter->cairo_context;//cairo context

  struct Rectangle *rect = widget_get_rectangle (windowToWidget (window));
  struct Widget *child = windowGetChild (window);
  bool selected = (wmSelectedWindow () == window);

  YColor bgcolor = createColor(0.8,0.8,0.8,1);
  const struct Value *bgcolourProperty = objectGetProperty (window_to_object (window), "background");
  if (bgcolourProperty)
  bgcolor = createColorInt32 (bgcolourProperty->uint32);
  clear_title_bar (cr, 0, 0, rect->w, title_height);
  clear_border (cr, 0, title_height, rect->w, rect->h - title_height);

  cairo_move_to (cr,0,0-title_height);
  cairo_line_to (cr,rect->w,0-title_height);
  cairo_line_to (cr,rect->w,rect->h);
  cairo_line_to (cr,0,rect->h);
  cairo_close_path (cr);
  cairo_set_source_rgb(cr,0.8,0.8,0.8);
  cairo_fill_preserve (cr);
  cairo_set_source_rgb (cr,0.6,0.6,0.6);
  cairo_set_line_width (cr, 2.0);
  
  cairo_stroke (cr);

  paint_title_bar (window, painter, 0, 0, rect -> w);

  rectangleDestroy (rect);

  /* paint child */
  if (child != NULL)
    {
      struct Rectangle *childRect = widget_get_rectangle (child);
      if (!painter_is_fully_clipped (painter))
        {
          widget_paint (child, painter);
        }
      rectangleDestroy (childRect);
    }
}

int
default_window_get_region (struct Window *window, int32_t x_pos, int32_t y_pos)
{
  struct Rectangle *rect = widget_get_rectangle (windowToWidget (window));
  int region = WINDOW_REGION_NOTHING;
  int32_t wx = 0, wy = 0, ww = rect -> w, wh = rect -> h;

  //translate the x and y coords so that they are based on window coords 0,0 origin
  int32_t x = x_pos - rect -> x;
  int32_t y = y_pos - rect -> y;

  //left edge of the closer button
  int closeX = rect -> w - button_right_edge;
  int expandX = closeX - edge_offset - button_size;

  //top edge of buttons
  int buttonPosY = (int)(y + (title_height/2)-(button_size/2));

  switch (windowGetSizeState (window))
    {
      case WINDOW_SIZE_NORMAL:
        wx = 5;
        wy = 5;
        ww = rect -> w - 10;
        wh = rect -> h - 10;
        break;
      case WINDOW_SIZE_MAXIMISE:
        wx = 0;
        wy = 0;
        ww = rect -> w;
        wh = rect -> h;
    }

  if (y > wy && y <= wy + title_height)
    {
      if (x > wx + wx + title_height && x < expandX)
        region = WINDOW_REGION_MOVE;
      if (x >= expandX && x <= expandX + button_size)
        {
          if (windowGetSizeState (window) == WINDOW_SIZE_NORMAL)
            region = WINDOW_REGION_MAXIMISE;
          else
            region = WINDOW_REGION_RESTORE;
        }
      if (x >= closeX && x <= closeX+button_size)
        region = WINDOW_REGION_CLOSE;
    }
  if (x > wx && x < wx + ww && y > wy + title_height && y < wy + wh)
    region = WINDOW_REGION_CHILD;

  if (windowGetSizeState (window) == WINDOW_SIZE_NORMAL)
    {
      if (y <= 5)
        {
          if (x <= 24)
            region = WINDOW_REGION_RESIZE_NW;
          if (x > 24 && x <= rect -> w - 24)
            region = WINDOW_REGION_RESIZE_N;
          if (x > rect -> w - 24 && x <= rect -> w)
            region = WINDOW_REGION_RESIZE_NE;
        }
      if (x <= 5)
        {
          if (y <= 24)
            region = WINDOW_REGION_RESIZE_NW;
          if (y > 24 && y <= rect -> h - 24)
            region = WINDOW_REGION_RESIZE_W;
          if (y > rect -> h - 24 && y <= rect -> h)
            region = WINDOW_REGION_RESIZE_SW;
        }
      if (y >= rect -> h - 5 && y <= rect -> h)
        {
          if (x <= 24)
            region = WINDOW_REGION_RESIZE_SW;
          if (x > 24 && x <= rect -> w - 24)
            region = WINDOW_REGION_RESIZE_S;
          if (x > rect -> w - 24 && x <= rect -> w)
            region = WINDOW_REGION_RESIZE_SE;
        }
      if (x >= rect -> w - 5 && x <= rect -> w)
        {
          if (y <= 24)
            region = WINDOW_REGION_RESIZE_NE;
          if (y > 24 && y <= rect -> h - 24)
            region = WINDOW_REGION_RESIZE_E;
          if (y > rect -> h - 24 && y <= rect -> h)
            region = WINDOW_REGION_RESIZE_SE;
        }
    }
  rectangleDestroy (rect);
  return region;
}

int
default_window_pointer_motion (struct Window *w, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
  int r = default_window_get_region (w, x, y);
  widget_global_to_local (windowToWidget(w), &x, &y);
  switch (r)
    {
      case WINDOW_REGION_CHILD:
        return widget_pointer_motion (windowGetChild (w), x, y, dx, dy);
    }
  return 0;
}

int
default_window_pointer_button (struct Window *w, int32_t x_pos, int32_t y_pos, uint32_t b, bool pressed)
{
  int r = default_window_get_region (w, x_pos, y_pos);

  //translate the x and y coords so that they are based on window coords 0,0 origin
  int32_t x;
  int32_t y;

  widget_get_position_local(windowToWidget (w), &x, &y);
  x = x_pos - x;
  y = y_pos - y;

  if (b == 0 && pressed == 1)
    {
      switch (r)
        {
          case WINDOW_REGION_MOVE:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_MOVE); break;
          case WINDOW_REGION_MAXIMISE:
            wmMaximiseWindow (w);   break;
          case WINDOW_REGION_RESTORE:
            wmRestoreWindow (w);   break;
          case WINDOW_REGION_CLOSE:
            windowRequestClose (w);   break;
          case WINDOW_REGION_RESIZE_NW:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_NW); break;
          case WINDOW_REGION_RESIZE_N:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_N); break;
          case WINDOW_REGION_RESIZE_NE:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_NE); break;
          case WINDOW_REGION_RESIZE_W:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_W); break;
          case WINDOW_REGION_RESIZE_E:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_E); break;
          case WINDOW_REGION_RESIZE_SW:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_SW); break;
          case WINDOW_REGION_RESIZE_S:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_S); break;
          case WINDOW_REGION_RESIZE_SE:
            windowStartReshape (w, x, y, WINDOW_RESHAPE_RESIZE_SE); break;
        }
    }
  if (b == 0 && pressed == 0)
    {
      windowStopReshape (w);
      return 1;
    }
  if (r == WINDOW_REGION_CHILD)
    return widget_pointer_button (windowGetChild (w), x, y, b, pressed);
  return 1;
}

static struct Rectangle *
default_window_child_rectangle (struct Window *window)
{
  struct Rectangle *wr = widget_get_rectangle (windowToWidget (window));

  int x = 2*edge_offset;
  int y = title_height + (2*edge_offset);
  int w = wr -> w - x - (2*edge_offset);
  int h = wr -> h - y - (2*edge_offset);

  struct Rectangle *cr = rectangleCreate (x, y, w, h);
  rectangleDestroy (wr);
  return cr;
}

static void
default_window_pointer_move (struct Window *window, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
}

void
default_window_reconfigure (struct Window *window,
                        int32_t *minWidth_p, int32_t *minHeight_p,
                        int32_t *reqWidth_p, int32_t *reqHeight_p,
                        int32_t *maxWidth_p, int32_t *maxHeight_p)
{
  struct Widget *child = windowGetChild (window);
  *minWidth_p  = 64;
  *minHeight_p = 48;
  *maxWidth_p  = -1;
  *maxHeight_p = -1;

  if (child != NULL)
    {
      int32_t childMinWidth, childMinHeight;
      int32_t childReqWidth, childReqHeight;
      int32_t childMaxWidth, childMaxHeight;
      widget_get_constraints (child, &childMinWidth, &childMinHeight,
                            &childReqWidth, &childReqHeight,
                            &childMaxWidth, &childMaxHeight);
      if (childMinWidth > 54)
        *minWidth_p = childMinWidth + (4*edge_offset);
      if (childMinHeight > 19)
        *minHeight_p = childMinHeight + (title_height + 4*edge_offset);
      if (childMaxWidth > 0)
        *maxWidth_p = childMaxWidth + (4*edge_offset);
      if (childMaxHeight > 0)
        *maxHeight_p = childMaxHeight + (title_height + 4*edge_offset);

      if (*reqWidth_p < 0 && childReqWidth > 54)
        *reqWidth_p = childReqWidth + (4*edge_offset);
      if (*reqHeight_p < 0 && childReqHeight > 19)
        *reqHeight_p = childReqHeight + (title_height + 4*edge_offset);
    }

  if (*maxWidth_p > 0)
    {
      if (*maxWidth_p < *minWidth_p)
        *maxWidth_p = *minWidth_p;
      if (*reqWidth_p > *maxWidth_p)
        *reqWidth_p = *maxWidth_p;
    }
  if (*maxHeight_p > 0)
    {
      if (*maxHeight_p < *minHeight_p)
        *maxHeight_p = *minHeight_p;
      if (*reqHeight_p > *maxHeight_p)
        *reqHeight_p = *maxHeight_p;
    }
  if (*reqWidth_p < *minWidth_p)
    *reqWidth_p = *minWidth_p;
  if (*reqHeight_p < *minHeight_p)
    *reqHeight_p = *minHeight_p;
}

void
default_window_resize (struct Window *window)
{
  struct Widget *child = windowGetChild (window);
  if (child != NULL)
    {
      struct Rectangle *childRect = default_window_child_rectangle(window);
      switch (windowGetSizeState (window))
        {
          case WINDOW_SIZE_NORMAL:
            widget_move (child, childRect->x, childRect->y);
            widget_resize (child, childRect->w, childRect->h);
            break;
          case WINDOW_SIZE_MAXIMISE:
            widget_move (child, childRect->x, childRect->y);
            widget_resize (child, childRect->w, childRect->h);

        }
      rectangleDestroy (childRect);
    }
}

void
default_draw_background_pane (struct Painter *painter, int32_t x, int32_t y, int32_t w, int32_t h)
{
}
