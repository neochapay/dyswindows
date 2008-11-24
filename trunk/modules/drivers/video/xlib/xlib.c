/************************************************************************
 *   Copyright (C) Simon Persson <simpster@users.sourceforge.net>
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <Y/modules/videodriver_interface.h>
#include <Y/modules/module_interface.h>
#include <Y/main/control.h>
#include <Y/buffer/bufferclass.h>
#include <Y/screen/viewport.h>
#include <Y/screen/screen.h>
#include <Y/screen/cairorenderer.h>
#include <Y/input/pointer.h>
#include <Y/input/ykb.h>
#include <Y/util/yutil.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo-xlib.h>
#include <cairo-xlib-xrender.h>

#define XLIB_EVENT_POLL_INTERVAL 20

typedef struct
{
  Display *dpy;
  Window win;
  Atom WM_DELETE_WINDOW;
  Pixmap buffer;
  struct VideoResolution cur_res;
  int scr;
  int polling_ID;
  int width, height;
  cairo_surface_t *surface;
  struct Viewport *viewport;
} XlibVideoDriverData;

typedef struct
{
  struct Buffer_t buffer;
  Pixmap pixmap;
  Display *dpy;
  XRenderPictFormat *xrender_format;
  int depth;
  int hblocksize;
  int vblocksize;
  bool resizing;
} XlibBuffer;

/******* XlibBuffer implementation follows **********/

static void
xlib_buffer_destroy (Buffer *self)
{
  XlibBuffer *buf = (XlibBuffer *)self;
  if (self)
  {
    buffer_finalise (self);
    cairo_surface_destroy (buf->buffer.surface);
    XFreePixmap (buf->dpy, buf->pixmap);
    yfree (buf);
  }
}

static XRenderPictFormat *
xlib_render_format (Display *dpy, cairo_format_t format)
{
  int pict_format;
  switch (format)
    {
      case CAIRO_FORMAT_A1:
        pict_format = PictStandardA1; break;
      case CAIRO_FORMAT_A8:
        pict_format = PictStandardA8; break;
      case CAIRO_FORMAT_RGB24:
        pict_format = PictStandardRGB24; break;
      case CAIRO_FORMAT_ARGB32:
      default:
        pict_format = PictStandardARGB32; break;
    }
  return XRenderFindStandardFormat (dpy, pict_format);
}

static void
xlib_buffer_set_size (Buffer *self, int w, int h)
{
  if (self->width == w && self->height == h)
    return; //no change

  XlibBuffer *buf = (XlibBuffer *)self;

  buffer_destroy_all_painters (self);
  cairo_surface_destroy (self->surface);

  if (buf->resizing)
    {
      int new_hblocks = 1 + w / buf->hblocksize;
      int new_vblocks = 1 + h / buf->vblocksize;
      int hblocks = 1 + self->width / buf->hblocksize;
      int vblocks = 1 + self->height / buf->vblocksize;
      if (hblocks != new_hblocks || vblocks != new_vblocks)
        {
          XFreePixmap (buf->dpy, buf->pixmap);
          buf->pixmap = XCreatePixmap (buf->dpy, DefaultRootWindow (buf->dpy),
                                       new_hblocks * buf->hblocksize,
                                       new_vblocks * buf->vblocksize,
                                       buf->depth);
        }
    }
  else
    {
      XFreePixmap (buf->dpy, buf->pixmap);
      buf->pixmap = XCreatePixmap (buf->dpy, DefaultRootWindow (buf->dpy),
                                   w, h, buf->depth);
    }

  self->surface = cairo_xlib_surface_create_with_xrender_format (buf->dpy, buf->pixmap,
    DefaultScreenOfDisplay (buf->dpy), buf->xrender_format, w, h);
  
  self->width = w;
  self->height = h;
}

static void
xlib_buffer_reallocate (Buffer *self, int new_w, int new_h)
{
  XlibBuffer *buf = (XlibBuffer *)self;
  Pixmap p = XCreatePixmap (buf->dpy, DefaultRootWindow (buf->dpy), new_w, new_h, buf->depth);
  cairo_surface_t *s = cairo_xlib_surface_create_with_xrender_format (buf->dpy, p,
    DefaultScreenOfDisplay (buf->dpy), buf->xrender_format, new_w, new_h);
  cairo_t *cr = cairo_create (s);
  cairo_set_source_surface (cr, self->surface, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_rectangle (cr, 0, 0, self->width, self->height);
  cairo_fill (cr);
  cairo_destroy (cr);
  
  buffer_destroy_all_painters (self);
  cairo_surface_destroy (self->surface);
  XFreePixmap (buf->dpy, buf->pixmap);
  buf->pixmap = p;
  self->surface = s;
}

static void
xlib_buffer_begin_resize (Buffer *self, int w, int h)
{
  XlibBuffer *buf = (XlibBuffer *)self;
  xlib_buffer_reallocate (self, w, h);
  buf->resizing = true;
  buf->hblocksize = w;
  buf->vblocksize = h;
}

static void
xlib_buffer_end_resize (Buffer *self)
{
  XlibBuffer *buf = (XlibBuffer *)self;
  xlib_buffer_reallocate (self, self->width, self->height);
  buf->resizing = false;
}

static BufferClass xlib_buffer_class =
{
    name:             "XlibBuffer",
    destroy:          xlib_buffer_destroy,
    set_size:         xlib_buffer_set_size,
    begin_resize:     xlib_buffer_begin_resize,
    end_resize:       xlib_buffer_end_resize
};

static XlibBuffer *
xlib_buffer_create (cairo_format_t buffer_format, uint w, uint h, Display *dpy)
{
  XlibBuffer *buf = ymalloc (sizeof (XlibBuffer));

  if (!buf)
    return NULL;

  /* Initialize the buffer variables */
  buffer_init (&buf->buffer, &xlib_buffer_class, buffer_format);
  buf->dpy = dpy;
  buf->hblocksize = 0;
  buf->vblocksize = 0;
  buf->xrender_format = xlib_render_format (dpy, buffer_format);
  buf->depth = buffer_get_depth (buffer_format);
  buf->resizing = false;

  buf->pixmap = XCreatePixmap (buf->dpy, DefaultRootWindow (buf->dpy),
                               1, 1, buf->depth);
  buf->buffer.surface = cairo_xlib_surface_create_with_xrender_format (buf->dpy, buf->pixmap,
                          DefaultScreenOfDisplay (buf->dpy), buf->xrender_format, 1, 1);
  
  buffer_set_size (&buf->buffer, w, h);
  return buf;
}


/******* Xlib video driver implementation follows **********/

/* Check to see if this is a repeated key.
   (idea shamelessly lifted from GII -- thanks guys! :)
  ...this is stolen from libSDL... thanks guys! ;)
 */
static int
xlib_key_repeat (Display *display, XEvent *event)
{
  XEvent peekevent;
  int repeated;
  
  repeated = 0;
  if (XPending (display) )
    {
      XPeekEvent (display, &peekevent);
      if (peekevent.type == KeyPress &&
         peekevent.xkey.keycode == event->xkey.keycode &&
         peekevent.xkey.time - event->xkey.time < 2)
        {
          repeated = 1;
          XNextEvent(display, &peekevent);
        }
    }
  return repeated;
}

static void
xlib_event_probe (void *data_v)
{
  XlibVideoDriverData *driver = data_v;
  XEvent xev;

  while (XPending (driver->dpy))
    {
      XNextEvent(driver->dpy, &xev);
      switch(xev.type)
        {
          case KeyPress:
            {
              XKeyEvent *kev = &xev.xkey;
              ykbKeyDown(kev->keycode);
            }
          break;
          case KeyRelease:
            {
              XKeyEvent *kev = &xev.xkey;
              if (!xlib_key_repeat (driver->dpy, &xev))
                ykbKeyUp(kev->keycode);
            }
          break;
          case ButtonPress:
            {
              XButtonEvent *bev = &xev.xbutton;
              pointerButtonChange (bev->button - 1, 1);
            }
          break;
          case ButtonRelease:
            {
              XButtonEvent *bev = &xev.xbutton;
              pointerButtonChange (bev->button - 1, 0);
            }
          break;
          case MotionNotify:
            {
              XMotionEvent *mev = &xev.xmotion;
              pointerSetPosition (mev->x, mev->y);
            }
          break;
          case ConfigureNotify:
            {
              XConfigureEvent *cev = &xev.xconfigure;
      
              if (cev->width != driver->width || cev->height != driver->height)
                {
                  driver->width = cev->width;
                  driver->height = cev->height;
                  XFreePixmap (driver->dpy, driver->buffer);
                  driver->buffer = XCreatePixmap (driver->dpy, driver->win, driver->width, driver->height,
                                                  DefaultDepth (driver->dpy,  driver->scr));

                  cairo_xlib_surface_set_drawable (driver->surface, driver->buffer, cev->width, cev->height);
                  viewportSetSize (driver->viewport, cev->width, cev->height);
                }
            }
          break;
          case Expose:
            {
              XExposeEvent *eev = &xev.xexpose;
              XCopyArea (driver->dpy, driver->buffer, driver->win, DefaultGC (driver->dpy, driver->scr),
                         eev->x, eev->y, eev->width, eev->height, eev->x, eev->y);
              XFlush (driver->dpy);
            }
          break;
          case ClientMessage:
            {
              XClientMessageEvent *cev = &xev.xclient;
              if (cev->format == 32 && (Atom)cev->data.l[0] == driver->WM_DELETE_WINDOW)
                {
                  Y_TRACE("requested to quit!");
                  controlShutdownY ();
                }
            }
          break;
        }
    }
  driver -> polling_ID = controlTimerDelay (0, XLIB_EVENT_POLL_INTERVAL, data_v, xlib_event_probe);
}

static void
xlib_get_pixel_dimensions (struct VideoDriver *self, int *x, int *y)
{
  XlibVideoDriverData *data = (XlibVideoDriverData *)self->d;
  *x = data->width;
  *y = data->height;
}

static const char *
xlib_get_name (struct VideoDriver *self)
{
  return self->module->name;
}

static struct llist *
xlib_get_resolutions (struct VideoDriver *self)
{
  XlibVideoDriverData *data = (XlibVideoDriverData *)self->d;
  struct llist *resolutions = new_llist ();
  yfree (data->cur_res.name);
  data->cur_res.name = ymalloc (30);
  sprintf (data -> cur_res.name, "%dx%d", data->width, data->height);
  llist_add_tail (resolutions, &(data -> cur_res));
  return resolutions;
}

static void
xlib_blit (struct VideoDriver *self, uint32_t *data,
         int x, int y, int w, int h, int stepping)
{
}

static struct Tuple *
xlib_special (struct VideoDriver *self, const struct Tuple *args)
{
  return NULL;
}

static void
xlib_begin_updates (struct VideoDriver *self)
{
}

static void
xlib_end_updates (struct VideoDriver *self)
{
  XlibVideoDriverData *driver = (XlibVideoDriverData *)self->d;
  XCopyArea (driver->dpy, driver->buffer, driver->win, DefaultGC (driver->dpy, driver->scr),
             0, 0, driver->width, driver->height, 0, 0);
  XFlush (driver->dpy);
}

static Renderer *
xlib_get_renderer (struct VideoDriver *self, const struct Rectangle *rect)
{
  XlibVideoDriverData *driver = (XlibVideoDriverData *)self->d;
  Renderer *renderer = cairo_renderer_get_renderer (cairo_renderer_create (rect, driver->surface));
  renderer_set_option (renderer, "hardware pointer", "yes");
  return renderer;
}

static Buffer *
xlib_get_buffer (struct VideoDriver *self, cairo_format_t buffer_format,
                 uint w, uint h)
{
  XlibVideoDriverData *driver = (XlibVideoDriverData *)self->d;
  return (Buffer *)xlib_buffer_create (buffer_format, w, h, driver->dpy);
}

int
initialise (struct Module *module, const struct Tuple *args)
{
  struct VideoDriver *videodriver;
  Display *dpy;
  XlibVideoDriverData *driver;

  if ( NULL == (dpy = XOpenDisplay (0)) )
    {
        Y_ERROR ("xlib: Unable to open display");
        return 1;
    }

  driver = ymalloc (sizeof (XlibVideoDriverData));
  videodriver = ymalloc (sizeof (struct VideoDriver));
  videodriver->d = driver;
  videodriver->module = module;

  driver->dpy = dpy;
  driver->width = 800;
  driver->height = 600;
  driver->scr = DefaultScreen (dpy);
  driver->win = XCreateSimpleWindow (dpy, DefaultRootWindow (dpy), 0, 0,
                                     driver->width, driver->height, 0,
                                     BlackPixel(dpy, driver->scr),
                                     BlackPixel(dpy, driver->scr));

  XSelectInput (dpy, driver->win,KeyPressMask | KeyReleaseMask | ButtonPressMask
                | ButtonReleaseMask | PointerMotionMask
                | StructureNotifyMask | ExposureMask);

  XTextProperty window_title_property;
  char *window_title = ystrdup("DsY on Xlib");

  if (0 != XStringListToTextProperty (&window_title, 1, &window_title_property))
    XSetWMName (dpy, driver->win, &window_title_property);
  yfree(window_title);

  driver->WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(dpy, driver->win, &driver->WM_DELETE_WINDOW, 1);

  XMapWindow (dpy, driver->win);
  
  driver->buffer = XCreatePixmap (driver->dpy, driver->win, driver->width, driver->height,
                                  DefaultDepth (driver->dpy,  driver->scr));
  
  driver->surface = cairo_xlib_surface_create (dpy, driver->buffer,
                                               DefaultVisual (driver->dpy, driver->scr),
                                               driver->width, driver->height);
  
  videodriver->getPixelDimensions = xlib_get_pixel_dimensions;
  videodriver->getName = xlib_get_name;
  videodriver->getResolutions = xlib_get_resolutions;
  videodriver->setResolution = NULL;
  videodriver->setPointer = NULL;
  videodriver->special = xlib_special;
  videodriver->beginUpdates = xlib_begin_updates;
  videodriver->endUpdates = xlib_end_updates;
  videodriver->blit = xlib_blit;
  videodriver->getRenderer = xlib_get_renderer;
  videodriver->get_buffer = xlib_get_buffer;

  static char moduleName[] = "Xlib Video Driver";
  module->name = moduleName;
  module->data = videodriver;

  driver->cur_res.name = NULL;
  driver->viewport = viewportCreate (videodriver);
  screenRegisterViewport (driver->viewport);

  driver->polling_ID =
    controlTimerDelay (0, XLIB_EVENT_POLL_INTERVAL, driver, xlib_event_probe);

  return 0;
}

int
finalise (struct Module *module)
{
  struct VideoDriver *videodriver = module->data;
  XlibVideoDriverData *driver = videodriver->d;

  screenUnregisterViewport (driver->viewport);
  viewportDestroy (driver->viewport);
  controlCancelTimerDelay (driver->polling_ID);

  cairo_surface_destroy (driver->surface);
  XFreePixmap (driver->dpy, driver->buffer);
  XDestroyWindow (driver->dpy, driver->win);
  XCloseDisplay (driver->dpy);
  yfree (driver);
  yfree (videodriver);
  return 0;
}

