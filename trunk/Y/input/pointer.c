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

#include <Y/object/object.h>
#include <Y/widget/widget.h>
#include <Y/screen/screen.h>
#include <Y/input/pointer.h>
#include <Y/buffer/buffer.h>
#include <Y/buffer/bufferio.h>
#include <Y/util/yutil.h>
#include <stdlib.h>
#include <stdio.h>

static int pointerX = 0, pointerY = 0;
static struct Widget *pointerWidget = NULL;

void
pointerGrab (struct Widget *w)
{
  pointerWidget = w;
}

void
pointerRelease ()
{
  pointerWidget = NULL;
}

void
pointerGetPosition (int *xp, int *yp)
{
  if (xp != NULL)
    *xp = pointerX;
  if (yp != NULL)
    *yp = pointerY;
}

void
pointerMovePosition (int dx, int dy)
{
  pointerSetPosition (pointerX + dx, pointerY + dy);
}

void
pointerSetPosition (int x, int y)
{
  struct Widget *w;
  int px = x;
  int py = y;
  int dx;
  int dy;
  screenConstrainPoint (&px, &py);
  dx = px - pointerX;
  dy = py - pointerY;
  if (px == pointerX && py == pointerY)
    return;
  screenInvalidateRectangle (rectangleCreate (pointerX, pointerY, 32, 32));
  pointerX = px;
  pointerY = py;
  if (pointerWidget != NULL)
    {
      w = pointerWidget;
      widget_global_to_local (w, &px, &py);
    }
  else
    w = screenGetRootWidget ();

  screenInvalidateRectangle (rectangleCreate (pointerX, pointerY, 32, 32));

  widget_pointer_motion (w, px, py, dx, dy);
}

void
pointerButtonChange (int button, int pressed)
{
  struct Widget *w;
  int x = pointerX;
  int y = pointerY;

  if (pointerWidget != NULL)
    {
      w = pointerWidget;
      widget_global_to_local (w, &x, &y);
    }
  else
    w = screenGetRootWidget ();

  widget_pointer_button (w, x, y, button, pressed);

}

static Buffer *pointerImageDefault = NULL;

static void
pointerLoadImages (void)
{
  const char filename[] = "default.png";
  size_t l = strlen (yPointerImageDir);
  size_t path_len = l + 1 + sizeof(filename) + 1;
  char path[path_len];
  snprintf(path, path_len, "%s/%s", yPointerImageDir, filename);
  pointerImageDefault = bufferLoadFromFile (path, -1, -1);
}

Buffer *
pointerGetCurrentImage (void)
{
  if (pointerImageDefault == NULL)
    pointerLoadImages ();
  return pointerImageDefault;
}

void
pointerRender (Renderer *renderer)
{
  if (pointerImageDefault == NULL)
    pointerLoadImages ();

  struct Rectangle pointer;
  pointerGetPosition (&pointer.x, &pointer.y);
  buffer_get_size (pointerImageDefault, &pointer.w, &pointer.h);
  if (renderer_enter (renderer, &pointer, 0, 0))
    {
      renderer_render_buffer (renderer, pointerImageDefault, pointerX, pointerY);
    }
}
