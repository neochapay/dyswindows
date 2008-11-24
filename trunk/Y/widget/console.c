/************************************************************************
 *   Copyright (C) Mark Thomas <markbt@efaref.net>
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

#include <Y/widget/console.h>
#include <Y/widget/widget_p.h>

#include <Y/util/yutil.h>
#include <Y/util/color.h>
#include <Y/buffer/painter.h>

#include <Y/object/class.h>
#include <Y/object/object_p.h>

#include <Y/text/font.h>
#include <Y/text/utf8.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <cairo.h>

/*
 * This whole file, and the console/terminal system in general, needs
 * a serious overhaul.  It was written a bit hastily.  Volunteers?
 */

enum ConsoleCharFlags
{
  CCF_BOLD       = 1<<0,
  CCF_BLINK      = 1<<1,
  CCF_INVERSE    = 1<<2,
  CCF_UNDERLINE  = 1<<3
};

struct ConsoleChar
{
  wchar_t character;
  int8_t foreground; //foreground color index?
  int8_t background; //backgroud color index?
  uint8_t flags;
  uint8_t charset;
};

static struct ConsoleChar consolecharEmpty = { L'\0', -1, -1, 0, 'A' };

struct Console
{
  struct Widget widget;
  uint32_t cols, rows;
  uint32_t cursorCol, cursorRow;
  struct ConsoleChar *contents;
  YColor colors[16];
  int foreground, background, bold, blink, inverse, underline;
  int defaultForeground, defaultBackground;
  int charWidth, charHeight, charBase;
  char charset; 
};

static void consoleResize (struct Widget *);
static void consoleReconfigure (struct Widget *);
static void consolePaint (struct Widget *, struct Painter *);
static ykbStringHandler consoleYKBString;
static ykbEventHandler consoleYKBEvent;
static void consoleResizeContents (struct Console *self, uint32_t newCols, uint32_t newRows);

DEFINE_CLASS(Console);
#include "Console.yc"

/* SUPER
 * Widget
 */

static struct WidgetTable consoleTable =
{
  resize:        consoleResize,
  reconfigure:   consoleReconfigure,
  paint:         consolePaint,
  ykbString:     consoleYKBString,
  ykbEvent :     consoleYKBEvent,
};

void
CLASS_INIT(struct Console *this, VTable *vtable)
{
  SUPER_INIT(this, vtable);
  this -> cols = 0;
  this -> rows = 0;
  this -> cursorCol = 0;
  this -> cursorRow = 0;
  this -> contents = NULL;
  YColor defaultColors[16];
  
  defaultColors[0] = createColorInt32(0xBB000000);
  defaultColors[1] = createColorInt32(0xFFCC0000);
  defaultColors[2] = createColorInt32(0xFF00CC00);
  defaultColors[3] = createColorInt32(0xFFCCCC00);
  defaultColors[4] = createColorInt32(0xFF4444CC);
  defaultColors[5] = createColorInt32(0xFFCC00CC);
  defaultColors[6] = createColorInt32(0xFF00CCCC);
  defaultColors[7] = createColorInt32(0xFFCCCCCC);
  defaultColors[8] = createColorInt32(0xFF666666);
  defaultColors[9] = createColorInt32(0xFFFF6666);
  defaultColors[10] = createColorInt32(0xFF66FF66);
  defaultColors[11] = createColorInt32(0xFFFFFF66);
  defaultColors[12] = createColorInt32(0xFF6666FF);
  defaultColors[13] = createColorInt32(0xFFFF66FF);
  defaultColors[14] = createColorInt32(0xFF66FFFF);
  defaultColors[15] = createColorInt32(0xFFFFFFFF);
  
  memcpy (this -> colors, defaultColors, sizeof (defaultColors));
  
  this -> defaultForeground = 7;
  this -> defaultBackground = 0;
  this -> foreground = -1;
  this -> background = -1;
  this -> bold = 0;
  this -> blink = 0;
  this -> inverse = 0;
  this -> underline = 0;
  this -> charset = 'A';
  /* FIXME: this really should be derived from the font */
  this -> charWidth = 7;
  this -> charHeight = 15;
  this -> charBase = 11;
  consoleResizeContents (this, 80, 24);
}

static inline struct Console *
castBack (struct Widget *widget)
{
  /* assert ( widget -> c == consoleClass ); */
  return (struct Console *)widget;
}

static inline const struct Console *
castBackConst (const struct Widget *widget)
{
  /* assert ( widget -> c == consoleClass ); */
  return (const struct Console *)widget;
}

static void
consoleResizeContents (struct Console *self, uint32_t newCols, uint32_t newRows)
{
  struct ConsoleChar *newData, *dst, *src;
  if (newCols == self -> cols && newRows == self -> rows)
    return;

  newData = ymalloc (sizeof (struct ConsoleChar) * newCols * newRows);

  dst = newData;
  for (uint32_t j = 0; j<newRows; ++j)
    {
      src = self -> contents + self -> cols * j;
      for (uint32_t i = 0; i<newCols; ++i)
        {
          if (j < self -> rows && i < self -> cols)
            memcpy (dst, src, sizeof (struct ConsoleChar));
          else
            memcpy (dst, &consolecharEmpty, sizeof (struct ConsoleChar));
          ++src;
          ++dst;
        }
    }

  yfree (self -> contents);
  self -> contents = newData;
  self -> rows = newRows;
  self -> cols = newCols;
  widget_repaint (consoleToWidget (self), NULL);
}

static void
consoleRepaintChars (struct Console *self, uint32_t sCol, uint32_t sRow,
                     uint32_t eCol, uint32_t eRow)
{
  struct Rectangle *dirty;
  dirty = rectangleCreate (sCol * self -> charWidth, sRow * self -> charHeight,
                           (eCol - sCol) * self -> charWidth,
                           (eRow - sRow) * self -> charHeight);
  widget_repaint (consoleToWidget (self), dirty);
}

void
consoleDrawText (struct Console *self, uint32_t col, uint32_t row, wchar_t *string, size_t len,
                 uint32_t width)
{
  uint32_t i = col, j = row;
  struct ConsoleChar *cur = self -> contents + self -> cols * j + i;
  uint32_t start = i;

  if (i >= self -> cols || j >= self -> rows)
    return;

  for (size_t ci = 0; ci < len; ci++)
    {
      cur -> character = string[ci];
      cur -> foreground = self -> foreground;
      cur -> background = self -> background;
      cur -> flags = ((self -> bold)      ? CCF_BOLD      : 0)
                   | ((self -> blink)     ? CCF_BLINK     : 0)
                   | ((self -> inverse)   ? CCF_INVERSE   : 0)
                   | ((self -> underline) ? CCF_UNDERLINE : 0);
      cur -> charset = self -> charset;
      ++i;
      ++cur;
      if (i >= self -> cols)
        {
          consoleRepaintChars (self, start, j, self -> cols, j + 1);
          i = 0;
          start = 0;
          ++j;
          cur = self -> contents + self -> rows * j + i;
          if (j >= self -> rows)
            return; /* scroll? */
        }
    }
  consoleRepaintChars (self, start, j, i, j + 1);
}

/* METHOD
 * clearRect :: (uint32, uint32, uint32, uint32) -> ()
 */
void
consoleClearRect (struct Console *self, uint32_t sCol, uint32_t sRow, uint32_t eCol, uint32_t eRow)
{

  if (sCol >= self -> cols)
    return;
  if (sRow >= self -> rows)
    return;
  if (eCol > self -> cols)
    eCol = self -> cols;
  if (eRow > self -> rows)
    eRow = self -> rows;
  for (uint32_t j = sRow; j < eRow; ++j)
    {
      struct ConsoleChar *cur = self -> contents + self -> cols * j + sCol;
      for (uint32_t i = sCol; i < eCol; ++i)
        {
          cur -> character = L'\0';
          cur -> foreground = -1;
          cur -> background = -1;
          cur -> flags = 0;
          cur -> charset = self -> charset; 
          ++cur;
        }
    }
  consoleRepaintChars (self, sCol, sRow, eCol, eRow);
}

/*

 */
static void
consolePaint (struct Widget *self_w, struct Painter *painter)
{
  struct Console *self = castBack (self_w);
  struct ConsoleChar *cur;

  
  //cairo stuff..
  cairo_t *cr = painter->cairo_context;
  cairo_select_font_face (cr, "Mono", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);  
  cairo_set_font_size (cr, 12.0);
  
  struct Rectangle *painterClip = painter_get_clip_rectangle_local (painter);
  
  //number of chars to edge of buffer
  uint32_t left = painterClip -> x / self -> charWidth;

  //number of chars to right edge of clip region
  uint32_t right = (painterClip -> x + painterClip -> w + self -> charWidth - 1)
                 / self -> charWidth;

  //number of chars to top of bufffer
  uint32_t top = painterClip -> y / self -> charHeight; 
  
  //number of chars to bottom of buffer
  uint32_t bottom = (painterClip -> y + painterClip -> h + self -> charHeight - 1)
                  / self -> charHeight;

  rectangleDestroy (painterClip);
  
  if (right > self -> cols)
    right = self -> cols;

  if (bottom > self -> rows)
    bottom = self -> rows;
  

  //j = column position..
  for (uint32_t j = top; j < bottom; ++j)
    {
      cur = self -> contents + self -> cols * j + left;
      
      //i= character position..
      for (uint32_t i = left; i < right; ++i)
        {
          int8_t background = cur -> background;
          int8_t foreground = cur -> foreground;
          if (background < 0)
            background = self -> defaultBackground;
          if (foreground < 0)
             foreground = self -> defaultForeground;  
          if ((cur -> flags & CCF_INVERSE) != 0)
            {
              int8_t tmp = background;
              background = foreground;
              foreground = tmp;
            } 
          if (cur -> flags & CCF_BOLD)
            foreground += 8;
          if (cur -> flags & CCF_BLINK)
            background += 8;


	  YColor curColor = self -> colors[background % 16];
	  cairo_set_source_rgba (cr, 
				 curColor.red,
				 curColor.green,
				 curColor.blue,
				 curColor.alpha);
	
	  int x = self -> charWidth * i;
	  int y = self -> charHeight * j;
	  painter_translate_xy(painter, &x, &y); //need to do this!
	  cairo_rectangle (cr, 
			   x,
			   y,
			   self -> charWidth, 
			   self -> charHeight);
	  
	  cairo_fill(cr); //paint all changes
	  
	  if (cur -> character != L'\0')
            {
	      wchar_t str[2] = { cur -> character, L'\0' };
	       
	      curColor = self -> colors[foreground % 16];
	      cairo_set_source_rgba (cr, 
				 curColor.red,
				 curColor.green,
				 curColor.blue,
				 curColor.alpha);
	      
	  
	      
	      x = self -> charWidth * i;
	      y = self -> charHeight * j + self -> charBase;
	      painter_translate_xy(painter, &x, &y); //need to do this!
	 
	      cairo_move_to(cr, 
			    x,
			    y);

	      cairo_show_text(cr, (const char *)str);
            }
          ++cur;
        }
    } 
  
  //check to make sure that the total num columns does not go off the 
  //space allocated to the widget..
  if (self -> cols * self -> charWidth < (uint32_t)self -> widget.w)
    {
      YColor defBack = self -> colors[self -> defaultBackground];
     

      cairo_set_source_rgba (cr, 
			     defBack.red,
			     defBack.green,
			     defBack.blue,
			     defBack.alpha);

      int x = self -> cols * self -> charWidth;
      int y = 0;
      painter_translate_xy(painter, &x, &y); //need to do this!
	 
      cairo_rectangle (cr, 
		       x, 
		       y,
		       self -> widget.w - self -> cols * self -> charWidth,
		       self -> widget.h);
      cairo_fill(cr); //paint all changes
    }

  //make sure rows are not greater then the num 
  if (self -> rows * self -> charHeight < (uint32_t)self -> widget.h)
    {
      YColor defBack = self -> colors[self -> defaultBackground];
      cairo_set_source_rgba (cr, 
			     defBack.red,
			     defBack.green,
			     defBack.blue,
			     defBack.alpha);

      int x = 0;
      int y = self -> rows * self -> charHeight;
      painter_translate_xy(painter, &x, &y); //need to do this!


      cairo_rectangle (cr, 
		       x, 
		       y,
		       self -> widget.w,
		       self -> widget.h - self -> rows * self -> charHeight);
      cairo_fill(cr); //paint all changes
    }
}

void
consoleYKBString (struct Widget *self_w, const char *str, uint16_t modifiers)
{
  struct Console *self = castBack (self_w);
  objectEmitSignal (console_to_object(self), "YKBString", tb_string(str), tb_uint32(modifiers));
}

void
consoleYKBEvent (struct Widget *self_w, const char *event, uint16_t modifiers)
{
  struct Console *self = castBack (self_w);
  objectEmitSignal (console_to_object(self), "YKBEvent", tb_string(event), tb_uint32(modifiers));
}

static struct Console *
consoleCreate (void)
{
  struct Console *self = ymalloc (sizeof (struct Console));
  objectInitialise (&(self -> widget.o), CLASS(Console));
  CLASS_INIT(self, &consoleTable.vtable);
  return self;
}

/* METHOD
 * DESTROY :: () -> ()
 */
static void
consoleDestroy (struct Console *self)
{
  widgetFinalise (consoleToWidget (self));
  objectFinalise (console_to_object (self));
  yfree (self);
}

struct Widget *
consoleToWidget (struct Console *self)
{
  return &(self -> widget);
}

struct Object *
console_to_object (struct Console *self)
{
  return &(self -> widget.o);
}

/* METHOD
 * Console :: () -> (object)
 */
struct Object *
consoleInstantiate (void)
{
  return console_to_object (consoleCreate ());
}

static void
consoleResize (struct Widget *self_w)
{
  struct Console *self = castBack (self_w);
  uint32_t newCols = self -> widget.w / self -> charWidth;
  uint32_t newRows = self -> widget.h / self -> charHeight;
  if (newCols < 1)
    newCols = 1;
  if (newRows < 1)
    newRows = 1;
  if (newCols != self->cols || newRows != self->rows)
    {
      consoleResizeContents (self, newCols, newRows);
      objectEmitSignal (console_to_object (self), "resize", tb_uint32(self->cols), tb_uint32(self->rows));
    }
}

static void
consoleReconfigure (struct Widget *self_w)
{
  struct Console *self = castBack (self_w);

  self -> widget.reqWidth  = self -> charWidth * 80;
  self -> widget.reqHeight = self -> charHeight * 24;
  self -> widget.minWidth  = self -> charWidth;
  self -> widget.minHeight = self -> charHeight;

  widget_reconfigure (self -> widget.container);
}

/* METHOD
 * drawText :: (uint32, uint32, string, uint32) -> ()
 */
static void
consoleCDrawText (struct Console *self, uint32_t col, 
		  uint32_t row, uint32_t length, 
		  const char *string, uint32_t width)
{
  size_t wlength = 2 * (length + 1);
  wchar_t wstring[wlength];
  size_t count = utf8towc (string, length, wstring, wlength);

  if (count != (size_t)-1)
    consoleDrawText (self, col, row, wstring, count, width);
}

void
consoleSetRendition (struct Console *self, int bold, int blink, int inverse, int underline, int foreground, int background, const char *charset)
{
  self -> bold = bold;
  self -> blink = blink;
  self -> inverse = inverse;
  self -> underline = underline;
  if (foreground >= 1 && foreground <= 8)
    self -> foreground = foreground - 1;
  else
    self -> foreground = -1;
  if (background >= 1 && background <= 8)
    self -> background = background - 1;
  else
  self -> background = -1;
  self -> charset = charset[0];
}

/* METHOD
 * setRendition :: (uint32, uint32, uint32, uint32, uint32, uint32, string) -> ()
 */
static void
consoleCSetRendition (struct Console *self,
                      uint32_t bold, uint32_t blink, uint32_t inverse, uint32_t underline,
                      uint32_t foreground, uint32_t background,
                      uint32_t charset_len, const char *charset)
{
  consoleSetRendition (self, bold, blink, inverse, underline, foreground, background, charset);
}

/* METHOD
 * swapVideo :: () -> ()
 */
void
consoleSwapVideo (struct Console *self)
{
  Y_TRACE ("Swap Video ?");
}

/* METHOD
 * ring :: () -> ()
 */
void
consoleRing (struct Console *self)
{
  Y_TRACE ("***BEEP***");
}

/* METHOD
 * updateCursorPos :: (uint32, uint32) -> ()
 */
void
consoleUpdateCursorPos (struct Console *self, uint32_t col, uint32_t row)
{
  if (col != self -> cursorCol || row != self -> cursorRow)
    {
      consoleRepaintChars (self, self -> cursorCol, self -> cursorRow,
                           self -> cursorCol + 1, self -> cursorRow +1);
      self -> cursorCol = col;
      self -> cursorRow = row;
      consoleRepaintChars (self, self -> cursorCol, self -> cursorRow,
                           self -> cursorCol + 1, self -> cursorRow +1);
    }
}

/* METHOD
 * scrollView :: (uint32, uint32, uint32) -> ()
 */
void
consoleScrollView (struct Console *self, uint32_t destRow, uint32_t srcRow, uint32_t numLines)
{
  if (destRow >= self -> rows)
    return;
  if (srcRow >= self -> rows)
    return;
  if (destRow + numLines > self -> rows)
    numLines = self -> rows - destRow;
  if (srcRow + numLines > self -> rows)
    numLines = self -> rows - srcRow;
  memmove (self -> contents + destRow * self -> cols,
           self -> contents + srcRow * self -> cols,
           numLines * self -> cols * sizeof (struct ConsoleChar)); 
  if (destRow < srcRow)
    consoleClearRect (self, 0, MAX (srcRow, destRow + numLines),
                      self -> cols, srcRow + numLines);
  else if (srcRow < destRow)
    consoleClearRect (self, 0, srcRow, self -> cols,
                      MIN (numLines, destRow - srcRow));
  consoleRepaintChars (self, 0, destRow, self -> cols, destRow + numLines);
}

