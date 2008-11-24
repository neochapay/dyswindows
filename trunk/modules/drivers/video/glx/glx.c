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
#include <Y/buffer/bufferclass.h>
#include <Y/buffer/imagebuffer.h>
#include <Y/modules/videodriver_interface.h>
#include <Y/modules/module_interface.h>
#include <Y/main/control.h>
#include <Y/screen/viewport.h>
#include <Y/screen/screen.h>
#include <Y/screen/cairorenderer.h>
#include <Y/input/pointer.h>
#include <Y/input/ykb.h>
#include <Y/util/yutil.h>
#include <glitz-glx.h>
#include <cairo-glitz.h>

#define GLX_EVENT_POLL_INTERVAL 20

typedef struct
{
  Display *dpy;
  Window win, root;
  Atom WM_DELETE_WINDOW;
  glitz_format_t *format;
  glitz_drawable_t *drawable;
  struct VideoResolution cur_res;
  int scr;
  int polling_ID;
  int width, height;
  cairo_surface_t *surface;
  struct Viewport *viewport;
} GlxVideoDriverData;

typedef struct
{
  struct Buffer_t buffer;
  int hblocksize;
  int vblocksize;
  bool resizing;
  glitz_surface_t *gsurface;
  glitz_drawable_t *drawable;
  glitz_format_t *format;
  glitz_drawable_format_t *dformat;
} GlxBuffer;

static cairo_surface_t *
resize_glitz_drawable (glitz_drawable_t *drawable,
                       glitz_format_t *format,
                       glitz_drawable_buffer_t buffer,
                       int width,
                       int height)
{
  glitz_surface_t *surface;
  cairo_surface_t *crsurface;
  
  glitz_drawable_update_size (drawable, width, height);
  surface = glitz_surface_create (drawable, format, width, height, 0, NULL);
  if (!surface)
    {
      Y_ERROR ("glx: Couldn't create glitz surface.");
      return NULL;
    }
  glitz_surface_attach (surface, drawable, buffer);
  crsurface = cairo_glitz_surface_create (surface);
  glitz_surface_destroy (surface);
  return crsurface;
}

/******* GlxBuffer implementation follows **********/

static void
glx_buffer_destroy (Buffer *self)
{
  GlxBuffer *buf = (GlxBuffer *)self;
  if (self)
    {
      buffer_finalise (self);
      cairo_surface_destroy (buf->buffer.surface);
      glitz_surface_destroy (buf->gsurface);
      glitz_drawable_destroy (buf->drawable);
      yfree (buf);
    }
}

static void
glx_buffer_set_size (Buffer *self, int w, int h)
{
  if (self->width == w && self->height == h)
    return; //no change
  
  GlxBuffer *buf = (GlxBuffer *)self;

  buffer_destroy_all_painters (self);
  cairo_surface_destroy (self->surface);

//   if (buf->resizing)
//     {
//       int new_hblocks = 1 + w / buf->hblocksize;
//       int new_vblocks = 1 + h / buf->vblocksize;
//       int hblocks = 1 + self->width / buf->hblocksize;
//       int vblocks = 1 + self->height / buf->vblocksize;
//       if (hblocks != new_hblocks || vblocks != new_vblocks)
//         {
//           glitz_surface_destroy (buf->gsurface);
//           buf->gsurface = glitz_surface_create (buf->drawable, buf->format,
//                                                 new_hblocks * buf->hblocksize,
//                                                 new_vblocks * buf->vblocksize,
//                                                 0, NULL);
//           glitz_surface_attach (buf->gsurface, buf->drawable, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);
//         }
//     }
//   else
//     {
      glitz_surface_destroy (buf->gsurface);
      buf->gsurface = glitz_surface_create (buf->drawable, buf->format,
                                            w, h, 0, NULL);
      glitz_surface_attach (buf->gsurface, buf->drawable, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);
//     }
  self->surface = cairo_glitz_surface_create (buf->gsurface);
  self->width = w;
  self->height = h;
  glitz_drawable_update_size (buf->drawable, w, h);
}

static void
glx_buffer_reallocate (Buffer *self, int w, int h)
{
//   GlxBuffer *buf = (GlxBuffer *)self;
// 
//   glitz_surface_t *gs = glitz_surface_create (buf->drawable, buf->format, w, h, 0, NULL);
//   cairo_surface_t *s = cairo_glitz_surface_create (gs);
//   cairo_t *cr = cairo_create (s);
//   cairo_set_source_surface (cr, self->surface, 0, 0);
//   cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
//   cairo_rectangle (cr, 0, 0, self->width, self->height);
//   cairo_fill (cr);
//   cairo_destroy (cr);
//   
//   buffer_destroy_all_painters (self);
//   cairo_surface_destroy (self->surface);
//   glitz_surface_destroy (buf->gsurface);
//   
//   self->surface = s;
//   buf->gsurface = gs;
//   glitz_drawable_update_size (buf->drawable, w, h);
//   glitz_surface_attach (buf->gsurface, buf->drawable, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);
}

static void
glx_buffer_begin_resize (Buffer *self, int w, int h)
{
  GlxBuffer *buf = (GlxBuffer *)self;
  glx_buffer_reallocate (self, w, h);
  buf->resizing = true;
  buf->hblocksize = w;
  buf->vblocksize = h;
}

static void
glx_buffer_end_resize (Buffer *self)
{
  GlxBuffer *buf = (GlxBuffer *)self;
  glx_buffer_reallocate (self, self->width, self->height);
  buf->resizing = false;
}

static BufferClass glx_buffer_class =
{
  name:             "GlxBuffer",
  destroy:          glx_buffer_destroy,
  set_size:         glx_buffer_set_size,
  begin_resize:     glx_buffer_begin_resize,
  end_resize:       glx_buffer_end_resize
};

static GlxBuffer *
glx_buffer_create (cairo_format_t buffer_format, uint w, uint h, GlxVideoDriverData *driver)
{
  glitz_drawable_format_t templ;
  unsigned long mask;

  GlxBuffer *buf = ymalloc (sizeof (GlxBuffer));
  if (!buf)
    return NULL;
  
  /* Initialize the buffer variables */
  buffer_init (&buf->buffer, &glx_buffer_class, buffer_format);
  buf->buffer.width = w;
  buf->buffer.height = h;
  buf->hblocksize = 0;
  buf->vblocksize = 0;
  buf->resizing = false;

  templ.samples          = 1;
  templ.color.fourcc     = GLITZ_FOURCC_RGB;
  templ.color.alpha_size = 8;
  templ.color.red_size   = 8;
  templ.color.green_size = 8;
  templ.color.blue_size  = 8;
  
  mask = GLITZ_FORMAT_SAMPLES_MASK |
         GLITZ_FORMAT_RED_SIZE_MASK |
         GLITZ_FORMAT_GREEN_SIZE_MASK |
         GLITZ_FORMAT_BLUE_SIZE_MASK |
         GLITZ_FORMAT_ALPHA_SIZE_MASK |
         GLITZ_FORMAT_FOURCC_MASK;

  buf->dformat = glitz_find_drawable_format (driver->drawable, mask, &templ, 0);
  if (!buf->dformat)
    {
      buf->dformat = glitz_find_pbuffer_format (driver->drawable, mask, &templ, 0);
      if (!buf->dformat)
        {
          Y_ERROR ("glx: Couldn't find drawable format.");
          return NULL;
        }
      else
        buf->drawable = glitz_create_pbuffer_drawable (driver->drawable, buf->dformat, w, h);
    }
  else
    buf->drawable = glitz_create_drawable (driver->drawable, buf->dformat, w, h);
  
  if (!buf->drawable)
    {
      Y_ERROR ("glx: Failed to create glitz drawable for buffer.");
      return NULL;
    }
  
  buf->format = glitz_find_standard_format (driver->drawable, GLITZ_STANDARD_ARGB32);
  buf->gsurface = glitz_surface_create (buf->drawable, buf->format, w, h, 0, NULL);
  glitz_surface_attach (buf->gsurface, buf->drawable, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);
  buf->buffer.surface = cairo_glitz_surface_create (buf->gsurface);
  
  return buf;
}



/******* Glx video driver implementation follows **********/

/* Check to see if this is a repeated key.
   (idea shamelessly lifted from GII -- thanks guys! :)
  ...this is stolen from libSDL... thanks guys! ;)
 */
static int
glx_key_repeat (Display *display, XEvent *event)
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
glx_event_probe (void *data_v)
{
  GlxVideoDriverData *driver = data_v;
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
              if (!glx_key_repeat (driver->dpy, &xev))
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
                  cairo_surface_destroy (driver->surface);
                  driver->surface = resize_glitz_drawable (driver->drawable, driver->format,
                      GLITZ_DRAWABLE_BUFFER_BACK_COLOR, driver->width, driver->height);
                  viewportSetSize (driver->viewport, cev->width, cev->height);
                }
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
  driver -> polling_ID = controlTimerDelay (0, GLX_EVENT_POLL_INTERVAL, data_v, glx_event_probe);
}

static void
glx_get_pixel_dimensions (struct VideoDriver *self, int *x, int *y)
{
  GlxVideoDriverData *data = (GlxVideoDriverData *)self->d;
  *x = data->width;
  *y = data->height;
}

static const char *
glx_get_name (struct VideoDriver *self)
{
  return self->module->name;
}

static struct llist *
glx_get_resolutions (struct VideoDriver *self)
{
  GlxVideoDriverData *data = (GlxVideoDriverData *)self->d;
  struct llist *resolutions = new_llist ();
  yfree (data->cur_res.name);
  data->cur_res.name = ymalloc (30);
  sprintf (data -> cur_res.name, "%dx%d", data->width, data->height);
  llist_add_tail (resolutions, &(data -> cur_res));
  return resolutions;
}

static void
glx_blit (struct VideoDriver *self, uint32_t *data,
         int x, int y, int w, int h, int stepping)
{
}

static struct Tuple *
glx_special (struct VideoDriver *self, const struct Tuple *args)
{
  return NULL;
}

static void
glx_begin_updates (struct VideoDriver *self)
{
}

static void
glx_end_updates (struct VideoDriver *self)
{
  GlxVideoDriverData *driver = (GlxVideoDriverData *)self->d;
  glitz_drawable_swap_buffers (driver->drawable);
  XFlush (driver->dpy);
}

static Renderer *
glx_get_renderer (struct VideoDriver *self, const struct Rectangle *rect)
{
  GlxVideoDriverData *driver = (GlxVideoDriverData *)self->d;
  Renderer *renderer = cairo_renderer_get_renderer (cairo_renderer_create (rect, driver->surface));
  renderer_set_option (renderer, "hardware pointer", "yes");
  return renderer;
}

static Buffer *
glx_get_buffer (struct VideoDriver *self, cairo_format_t buffer_format,
                 uint w, uint h)
{
  GlxVideoDriverData *driver = (GlxVideoDriverData *)self->d;
  Buffer *b = (Buffer *)glx_buffer_create (buffer_format, w, h, driver);
  if (!b)
    {
      b = (Buffer *)image_buffer_create (buffer_format, w, h);
      Y_WARN ("glx: falling back to image buffer.");
    }
  return b;
}

static void
print_features (unsigned long features)
{
  Y_INFO ("texture rectangle: %s\n",
          (features & GLITZ_FEATURE_TEXTURE_RECTANGLE_MASK)? "Yes": "No");
  Y_INFO ("texture non power of two: %s\n",
      (features & GLITZ_FEATURE_TEXTURE_NON_POWER_OF_TWO_MASK)? "Yes":
          "No");
  Y_INFO ("texture mirrored repeat: %s\n",
      (features & GLITZ_FEATURE_TEXTURE_MIRRORED_REPEAT_MASK)? "Yes":
          "No");
  Y_INFO ("texture border clamp: %s\n",
          (features & GLITZ_FEATURE_TEXTURE_BORDER_CLAMP_MASK)? "Yes": "No");
  Y_INFO ("multitexture: %s\n",
          (features & GLITZ_FEATURE_MULTITEXTURE_MASK)? "Yes": "No");
  Y_INFO ("multi draw arrays: %s\n",
          (features & GLITZ_FEATURE_MULTI_DRAW_ARRAYS_MASK)? "Yes": "No");
  Y_INFO ("texture environment combine: %s\n",
      (features & GLITZ_FEATURE_TEXTURE_ENV_COMBINE_MASK)? "Yes":
          "No");
  Y_INFO ("texture environment dot3: %s\n",
          (features & GLITZ_FEATURE_TEXTURE_ENV_DOT3_MASK)? "Yes": "No");
  Y_INFO ("blend color: %s\n",
          (features & GLITZ_FEATURE_BLEND_COLOR_MASK)? "Yes": "No");
  Y_INFO ("packed pixels: %s\n",
          (features & GLITZ_FEATURE_PACKED_PIXELS_MASK)? "Yes": "No");
  Y_INFO ("fragment program: %s\n",
          (features & GLITZ_FEATURE_FRAGMENT_PROGRAM_MASK)? "Yes": "No");
  Y_INFO ("vertex buffer object: %s\n",
          (features & GLITZ_FEATURE_VERTEX_BUFFER_OBJECT_MASK)? "Yes": "No");
  Y_INFO ("pixel buffer object: %s\n",
          (features & GLITZ_FEATURE_PIXEL_BUFFER_OBJECT_MASK)? "Yes": "No");
  Y_INFO ("per component rendering: %s\n",
      (features & GLITZ_FEATURE_PER_COMPONENT_RENDERING_MASK)? "Yes":
          "No");
  Y_INFO ("full-scene anti-aliasing: %s\n",
          (features & GLITZ_FEATURE_MULTISAMPLE_MASK)? "Yes": "No");
  Y_INFO ("full-scene anti-aliasing filter hint: %s\n",
      (features & GLITZ_FEATURE_MULTISAMPLE_FILTER_HINT_MASK)? "Yes":
          "No");
  Y_INFO ("framebuffer object: %s\n",
      (features & GLITZ_FEATURE_FRAMEBUFFER_OBJECT_MASK)? "Yes":
          "No");
}

static int
print_format (glitz_drawable_format_t *format)
{
  if (format)
  {
    Y_INFO ("0x%x\t%d/%d/%d/%d    \t%d\t%d\t%c\t%d\n",
            (int) format->id,
            format->color.red_size,
            format->color.green_size,
            format->color.blue_size,
            format->color.alpha_size,
            format->depth_size,
            format->stencil_size,
            (format->doublebuffer) ? 'y' : '.',
            format->samples);
    return 1;
  }
  return 0;
}



int
initialise (struct Module *module, const struct Tuple *args)
{
  struct VideoDriver *videodriver;
  Display *dpy;
  int screen;
  unsigned int i;
  char dsy_name[] = "dsy";
  XVisualInfo *vinfo;
  XSetWindowAttributes xswa;
  XWMHints *wmHints;
  XSizeHints *normalHints;
  XClassHint *classHint;
  GlxVideoDriverData *driver;
  glitz_drawable_format_t *format, templ;
  glitz_format_t templ2;
  unsigned long mask;

  dpy = XOpenDisplay (NULL);
  if (dpy == NULL)
    {
      Y_ERROR ("glx: Unable to open display");
      return 1;
    }

  glitz_glx_init (NULL);

  screen = DefaultScreen (dpy);

  templ.samples          = 1;
  templ.doublebuffer     = 1;
  templ.color.fourcc     = GLITZ_FOURCC_RGB;
  
  mask = GLITZ_FORMAT_SAMPLES_MASK | GLITZ_FORMAT_FOURCC_MASK | GLITZ_FORMAT_DOUBLEBUFFER_MASK;

  format = glitz_glx_find_window_format (dpy, screen, mask, &templ, 0);
  if (!format)
    {
      Y_ERROR ("glx: Couldn't find window format.");
      return 1;
    }

  vinfo = glitz_glx_get_visual_info_from_format (dpy, screen, format);
  if (!vinfo)
    {
      Y_ERROR ("glx: Couldn't find visual info.");
      return 1;
    }

  driver = ymalloc (sizeof (GlxVideoDriverData));
  videodriver = ymalloc (sizeof (struct VideoDriver));
  videodriver->d = driver;
  videodriver->module = module;
  
  driver->dpy = dpy;
  driver->width = 800;
  driver->height = 600;
  driver->scr = screen;
  driver->root = RootWindow (dpy, screen);

  xswa.colormap = XCreateColormap (dpy, driver->root, vinfo->visual, AllocNone);
  driver->win = XCreateWindow (dpy, driver->root,
                               0, 0, driver->width, driver->height,
                               0, vinfo->depth, InputOutput,
                               vinfo->visual, CWColormap, &xswa);

  XFree (vinfo);

  normalHints = XAllocSizeHints ();
  normalHints->flags      = PMinSize | PMaxSize | PSize;
  normalHints->min_width  = driver->width;
  normalHints->min_height = driver->height;
  normalHints->max_width  = driver->width;
  normalHints->max_height = driver->height;

  classHint = XAllocClassHint ();
  classHint->res_name = dsy_name;
  classHint->res_class = dsy_name;

  wmHints = XAllocWMHints ();
  wmHints->flags = InputHint;
  wmHints->input = TRUE;
  Xutf8SetWMProperties (dpy, driver->win, "DsY on Glx! Woohoo!", dsy_name, 0, 0,
                        normalHints, wmHints, classHint);
  XFree (wmHints);
  XFree (classHint);
  XFree (normalHints);

  XSelectInput (dpy, driver->win,KeyPressMask | KeyReleaseMask | ButtonPressMask
                | ButtonReleaseMask | PointerMotionMask
                | StructureNotifyMask | ExposureMask);

  driver->drawable = glitz_glx_create_drawable_for_window (dpy, driver->scr, format,
                                                   driver->win, driver->width,
                                                   driver->height);
  if (!driver->drawable)
    {
      Y_ERROR ("glx: Failed to create glitz drawable.");
      return 1;
    }
  
  print_features (glitz_drawable_get_features (driver->drawable));

  i = 0;
  while (print_format (glitz_find_drawable_format (driver->drawable, 0, 0, i)))
    i++;

    
  //driver->format = glitz_find_standard_format (driver->drawable, GLITZ_STANDARD_ARGB32);
  templ2.color.fourcc     = GLITZ_FOURCC_RGB;
  templ2.color.red_size   = 8;
  templ2.color.green_size = 8;
  templ2.color.blue_size  = 8;
  templ2.color.alpha_size = 0;
  mask = GLITZ_FORMAT_RED_SIZE_MASK | GLITZ_FORMAT_GREEN_SIZE_MASK | GLITZ_FORMAT_BLUE_SIZE_MASK
      | GLITZ_FORMAT_ALPHA_SIZE_MASK | GLITZ_FORMAT_FOURCC_MASK;
  driver->format = glitz_find_format (driver->drawable,  mask, &templ2, 0);
  if (!driver->format)
    {
      Y_ERROR ("glx: Couldn't find RGB24 surface format.");
      return 1;
    }
  
  driver->surface = resize_glitz_drawable (driver->drawable, driver->format,
                                             GLITZ_DRAWABLE_BUFFER_BACK_COLOR, driver->width,
                                             driver->height);

  driver->WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(dpy, driver->win, &driver->WM_DELETE_WINDOW, 1);
  
  XMapWindow (dpy, driver->win);
  
  videodriver->getPixelDimensions = glx_get_pixel_dimensions;
  videodriver->getName = glx_get_name;
  videodriver->getResolutions = glx_get_resolutions;
  videodriver->setResolution = NULL;
  videodriver->setPointer = NULL;
  videodriver->special = glx_special;
  videodriver->beginUpdates = glx_begin_updates;
  videodriver->endUpdates = glx_end_updates;
  videodriver->blit = glx_blit;
  videodriver->getRenderer = glx_get_renderer;
  videodriver->get_buffer = glx_get_buffer;

  static char moduleName[] = "Glx Video Driver";
  module->name = moduleName;
  module->data = videodriver;

  driver->cur_res.name = NULL;
  driver->viewport = viewportCreate (videodriver);
  screenRegisterViewport (driver->viewport);

  driver->polling_ID =
    controlTimerDelay (0, GLX_EVENT_POLL_INTERVAL, driver, glx_event_probe);

  return 0;
}

int
finalise (struct Module *module)
{
  struct VideoDriver *videodriver = module->data;
  GlxVideoDriverData *driver = videodriver->d;

  screenUnregisterViewport (driver->viewport);
  viewportDestroy (driver->viewport);
  controlCancelTimerDelay (driver->polling_ID);

  cairo_surface_destroy (driver->surface);
  glitz_drawable_destroy (driver->drawable);
  XDestroyWindow (driver->dpy, driver->win);
  XCloseDisplay (driver->dpy);
  yfree (driver);
  yfree (videodriver);
  return 0;
}

