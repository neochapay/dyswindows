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

#include <Y/modules/module_interface.h>
#include <Y/modules/windowmanager_interface.h>
#include <Y/object/object.h>
#include <Y/widget/desktop.h>
#include <Y/screen/screen.h>
#include <Y/util/zorder.h>
#include <Y/util/yutil.h>
#include <stdio.h>

static void defaultSelectWindow (struct WindowManager *wm, struct Window *w);

static struct Desktop *desktop;
static struct Window *currentWindow;

static void
defaultUnload (struct WindowManager *wm)
{
  moduleUnload ("wm/default");
}

static void
defaultRegisterWindow (struct WindowManager *wm, struct Window *w)
{
  desktopAddWindow (desktop, w);
}

static void
defaultUnregisterWindow (struct WindowManager *wm, struct Window *w)
{
  if (currentWindow == w)
    currentWindow = NULL;
  desktopRemoveWindow (desktop, w);
}

static void
defaultYKBEvent (struct WindowManager *wm, const char *event, uint16_t modifiers)
{
  if (strcmp(event, "wm:cycle windows forward") == 0)
    desktopCycleWindows (desktop, 1);
  else if (strcmp(event, "wm:cycle windows backward") == 0)
    desktopCycleWindows (desktop, -1);
  else if (strcmp(event, "wm:close window") == 0)
    {
      if (currentWindow != NULL)
        windowRequestClose (currentWindow);
    }
}

static void
defaultWindowPointerButton (struct WindowManager *wm, struct Window *w,
                            int x, int y, int button, int pressed)
{
  if (pressed == 1)
    {
      defaultSelectWindow (wm, w);
    }
}

static void
defaultWindowPointerMotion (struct WindowManager *wm, struct Window *w,
                            int x, int y, int dx, int dy)
{
}

static void
defaultWindowPointerEnter (struct WindowManager *wm, struct Window *w,
                           int x, int y)
{
}

static void
defaultWindowPointerLeave (struct WindowManager *wm, struct Window *w)
{
}

static struct Window *
defaultSelectedWindow (struct WindowManager *wm)
{
  return currentWindow;
}

static void
defaultSelectWindow (struct WindowManager *wm, struct Window *w)
{
  if (currentWindow != w)
    {
      if (currentWindow != NULL)
        {
          widget_repaint (windowToWidget (currentWindow), NULL);
          ykbUnsetFocus (windowGetFocussedWidget (currentWindow));
        }
      currentWindow = w;
      if (currentWindow != NULL)
        {
          ykbSetFocus (windowGetFocussedWidget (currentWindow));
          widget_repaint (windowToWidget (currentWindow), NULL);
          desktopRaiseWindow (desktop, currentWindow);
        }
    }
}

static void
defaultMaximiseWindow (struct WindowManager *wm, struct Window *win)
{
  int x, y, w, h;
  defaultSelectWindow (wm, win);
  widget_get_position_local (desktopToWidget (desktop), &x, &y);
  widget_get_size (desktopToWidget (desktop), &w, &h);
  windowSaveGeometry (win);
  windowSetSizeState (win, WINDOW_SIZE_MAXIMISE);
  widget_move (windowToWidget (win), x, y+20);
  widget_resize (windowToWidget (win), w, h-20);
  widget_move (windowToWidget (win), x-10, y);
  widget_resize (windowToWidget (win), w, h-10);
}

static void
defaultRestoreWindow (struct WindowManager *wm, struct Window *win)
{
  defaultSelectWindow (wm, win);
  windowSetSizeState (win, WINDOW_SIZE_NORMAL);
  windowRestoreGeometry (win);
}

int
initialise (struct Module *m, const struct Tuple *args)
{
  struct WindowManager *self;
  self = ymalloc (sizeof (struct WindowManager));
  self -> unload              = defaultUnload;
  self -> registerWindow      = defaultRegisterWindow;
  self -> unregisterWindow    = defaultUnregisterWindow;
  self -> ykbEvent            = defaultYKBEvent;
  self -> windowPointerButton = defaultWindowPointerButton;
  self -> windowPointerMotion = defaultWindowPointerMotion;
  self -> windowPointerEnter  = defaultWindowPointerEnter;
  self -> windowPointerLeave  = defaultWindowPointerLeave;
  self -> selectedWindow      = defaultSelectedWindow;
  self -> selectWindow        = defaultSelectWindow;
  self -> maximiseWindow      = defaultMaximiseWindow;
  self -> restoreWindow       = defaultRestoreWindow;
  self -> module = m;
  static char moduleName[] = "Default Window Manager";
  m -> name = moduleName;
  m -> data = self;

  windowmanagerRegister (self);

  desktop = desktopCreate ();
  screenSetRootWidget (desktopToWidget (desktop));
  return 0;
}

int
finalise (struct Module *m)
{
  objectDestroy (desktop_to_object(desktop));
  yfree (m -> data);
  return 0;
}

/* arch-tag: bea4f81c-1f2d-41ba-be84-362a400a0cb0
 */
