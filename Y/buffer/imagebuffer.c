/************************************************************************
 *   Copyright (C) Dustin Norlander <dustinn@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#include <Y/buffer/imagebuffer.h>
#include <Y/util/yutil.h>

struct ImageBuffer_t
{
  Buffer buffer;
  uint8_t *pixel_data;  //we need this because cairo doesn't have a conveneient way to get the actual pixeldata
  uint32_t stride_bytes;
  int hblocksize;
  int vblocksize;
  bool resizing;
};

static uint32_t
image_buffer_stride_bytes (cairo_format_t format, int width)
{
  uint32_t stride_bytes;
  switch (format)
    {
      case CAIRO_FORMAT_A1: //packed in 32 bit words
        stride_bytes = 4 * ( (width & 0x1F ? 1 : 0) + width / 32 ); break;
      case CAIRO_FORMAT_A8:
        stride_bytes = width; break;
      case CAIRO_FORMAT_RGB16_565:
        stride_bytes = 2 * width; break;
      case CAIRO_FORMAT_RGB24:
        stride_bytes = 3 * width; break;
      case CAIRO_FORMAT_ARGB32:
      default:
        stride_bytes = 4 * width; break;
    }
  return stride_bytes;
}

/*
 * Sets the size of an already created buffer
 * If there is any pixel data stored in the buffer
 * it will be destroyed.
 *
 * A lot of the initialization of the buffer is
 * contingent on it's size, so much of that
 * information is set here
 *
 */
static void
image_buffer_set_size (Buffer *self, int w, int h)
{
  if (self->width == w && self->height == h)
    return; //no change
  
  ImageBuffer *buf = (ImageBuffer *)self;
  
  // destroy the cairo surface if there is one..
  // This might be a problem if the cairo context is currently trying to
  // draw to this surface.
  //
  // XXX: Possible implement a mutex on the Painter struct.
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
      yfree (buf->pixel_data);
      buf->stride_bytes = image_buffer_stride_bytes(self->format, new_hblocks * buf->hblocksize);
      buf->pixel_data = ymalloc (buf->stride_bytes * new_vblocks * buf->vblocksize);
    }
  }
  else
  {
    yfree (buf->pixel_data);
    buf->stride_bytes = image_buffer_stride_bytes(self->format, w);
    buf->pixel_data = ymalloc (buf->stride_bytes * h);
  }

  //memset (buf->pixel_data, 0, memblock);	// Set all channels to black/transparant
  
  self->surface = cairo_image_surface_create_for_data (buf->pixel_data,
                    self->format, w, h, buf->stride_bytes);
  self->width = w;
  self->height = h;
}


/**
 * Destroys the buffer
 */
static void
image_buffer_destroy (Buffer *self)
{
  ImageBuffer *buf = (ImageBuffer *)self;
  if (self)
    {
      buffer_finalise (self);
      cairo_surface_destroy (self->surface);
      if (buf->pixel_data)
        yfree(buf->pixel_data);
      yfree (buf);
    }
}

static void
image_buffer_reallocate (Buffer *self, int new_w, int new_h)
{
  ImageBuffer *buf = (ImageBuffer *)self;
  uint32_t stride_bytes = image_buffer_stride_bytes(self->format, new_w);
  uint8_t *p = ymalloc (stride_bytes * new_h);
  cairo_surface_t *s = cairo_image_surface_create_for_data (p, self->format, new_w, new_h, stride_bytes);
  cairo_t *cr = cairo_create (s);
  cairo_set_source_surface (cr, self->surface, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_rectangle (cr, 0, 0, self->width, self->height);
  cairo_fill (cr);
  cairo_destroy (cr);

  buffer_destroy_all_painters (self);
  cairo_surface_destroy (self->surface);
  yfree (buf->pixel_data);

  buf->pixel_data = p;
  buf->stride_bytes = stride_bytes;
  self->surface = s;
}

static void
image_buffer_begin_resize (Buffer *self, int w, int h)
{
  ImageBuffer *buf = (ImageBuffer *)self;
  image_buffer_reallocate (self, w, h);
  buf->resizing = true;
  buf->hblocksize = w;
  buf->vblocksize = h;
}

static void
image_buffer_end_resize (Buffer *self)
{
  ImageBuffer *buf = (ImageBuffer *)self;
  image_buffer_reallocate (self, self->width, self->height);
  buf->resizing = false;
}

/* defines the table of necessary Buffer functions for the ImageBuffer */
static BufferClass image_buffer_class =
{
  name:             "ImageBuffer",
  destroy:          image_buffer_destroy,
  set_size:         image_buffer_set_size,
  begin_resize:     image_buffer_begin_resize,
  end_resize:       image_buffer_end_resize,
};


uint8_t *
image_buffer_get_pixel_data (ImageBuffer *self)
{
  return self->pixel_data;
}

uint32_t
image_buffer_get_stride_bytes (ImageBuffer *self)
{
  return self->stride_bytes;
}


/**
 * Create buffer
 *
 * Creates a new Buffer, including allocating memory.
 */
ImageBuffer *
image_buffer_create (cairo_format_t format, uint w, uint h)
{
  ImageBuffer *buf = ymalloc (sizeof (ImageBuffer));
  
  if (!buf)
    return NULL;
  
/* Initialize the buffer variables */
  buffer_init (&buf->buffer, &image_buffer_class, format);
  buf->hblocksize = 0;
  buf->vblocksize = 0;
  buf->resizing = false;

/* Initialize something small so we can get rid of special cases in set_size...*/
  buf->stride_bytes = image_buffer_stride_bytes(format, 1);
  buf->pixel_data = ymalloc (buf->stride_bytes * 1);
  buf->buffer.surface = cairo_image_surface_create_for_data (buf->pixel_data,
                    format, 1, 1, buf->stride_bytes);
  buffer_set_size (&buf->buffer, w, h);
  return buf;
}


/**
 * Creates a new buffer from the data passed in.
 *
 * -DN
 */
ImageBuffer *
image_buffer_create_from_data (cairo_format_t format,
                               uint w, uint h, uint stride_bytes, uint8_t *data)
{
  ImageBuffer *buf = image_buffer_create (format, 1, 1);
  cairo_surface_destroy (buf->buffer.surface);
  yfree (buf->pixel_data);

  buf->stride_bytes = stride_bytes;
  buf->pixel_data = data;
  buf->buffer.surface = cairo_image_surface_create_for_data (buf->pixel_data,
                    format, w, h, buf->stride_bytes);
  buf->buffer.width = w;
  buf->buffer.height = h;
  
  return buf;
}

