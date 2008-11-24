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


#include <Y/buffer/bufferio.h>
#include <Y/util/llist.h>
#include <Y/util/yutil.h>
#include <cairo.h>
#include <Y/buffer/imagebuffer.h>

#include <png.h>

#include <string.h>
#include <stdlib.h>

struct FileHandlerInfo
{
  BufferFileHandler callback;
  const char **extensions;
};

static int maximumExtensionLength = 0;
static struct llist *handlers = NULL;
static void bufferioInitialise (void);

/**
 * Because cairo has no current way to get the pixel data
 * out of an image buffer we have to load a cairo_surface with the
 * image then copy it onto another surface which we have created the
 * pixel_data for.
 * The Cairo list suggests later cairo releases will contain the funcitons
 * that we need to do this more cleanly.
 * -DN
 */

Buffer *
buffer_load_from_png (const char *filename) 
{
  cairo_surface_t *cairo_surface = cairo_image_surface_create_from_png (filename);
  int width = cairo_image_surface_get_width(cairo_surface);
  int height = cairo_image_surface_get_height(cairo_surface);
  
  //we draw the image onto a ARGB32 buffer
  int btpp = 4;
  int memblock = width*height*btpp;
  Y_TRACE ("MEMBLOCK %d w %d h %d", memblock, width, height);
  uint8_t *pixel_data = ymalloc(width*height*btpp);
  memset (pixel_data, 0, memblock);	// Set all channels to black/transparant
  ImageBuffer *cairoBuffer = image_buffer_create_from_data (CAIRO_FORMAT_ARGB32,
							       width,
							       height,
							       btpp*width,
							       pixel_data);
  cairo_t *cr = buffer_get_cairo_context ((Buffer *)cairoBuffer);
  cairo_set_source_surface (cr, cairo_surface, 0, 0);
  cairo_paint(cr);

  cairo_destroy(cr);
  cairo_surface_destroy(cairo_surface);
  return (Buffer *)cairoBuffer;
}


Buffer *
bufferLoadFromFile (const char *filename, int w, int h)
{
  if (handlers == NULL)
    bufferioInitialise ();

  FILE *file;
  file = fopen (filename, "r");

  if (file == NULL)
    return NULL;

  Buffer *buffer = NULL;

  /* 1. Split filename and try the extension
   */

  int i = 0;
  int l = strlen (filename);
  const char *ext = filename + l - 1;

  while (*ext != '.' && i < l && i < maximumExtensionLength)
    {
      ext--;
      i++;
    }

  if (i != 0 && i < l && i < maximumExtensionLength)
    {
      /* ext currently points at the dot, we want the extension */
      ext++;

      for (struct llist_node *node = llist_head (handlers);
           node;
           node = llist_node_next (node)) 
        {
          struct FileHandlerInfo *info = llist_node_data (node);

          for (int e=0; info->extensions[e] != NULL; ++e)
            {
              if (strcmp (ext, info->extensions[e]) == 0)
                {
                  /* try this handler */
                  buffer = info->callback (file, w, h);

                  if (buffer != NULL)
                    return buffer;

                  break;
                }
            }
        }
    }

  /* 2. Try all the loaders in sequence
   */

  for (struct llist_node *node = llist_head (handlers);
       node;
       node = llist_node_next (node)) 
    {
      struct FileHandlerInfo *info = llist_node_data (node);

      /* try this handler */
      buffer = info->callback (file, w, h);

      if (buffer != NULL)
        return buffer;
    }

  /* 3. fail
   */

  return NULL;

}

void
bufferRegisterFileHandler (BufferFileHandler callback, const char *extensions[])
{
  if (handlers == NULL)
    bufferioInitialise ();

  struct FileHandlerInfo *info = ymalloc (sizeof (struct FileHandlerInfo));
  info->callback = callback;
  info->extensions = extensions;
  llist_add_tail (handlers, info);

  for (int i=0; extensions[i] != NULL; ++i)
    {
      int l = strlen (extensions [i]);
      if (l > maximumExtensionLength)
        maximumExtensionLength = l;
    }
}

void
bufferUnregisterFileHandler (BufferFileHandler callback)
{
  if (handlers == NULL)
    return;

  for (struct llist_node *node = llist_head (handlers);
       node;
       node = llist_node_next (node)) 
    {
      struct FileHandlerInfo *info = llist_node_data (node);
      if (info->callback == callback)
        {
          llist_node_delete (node);
          yfree (info);
          return;
        }
    }
}

#define PNG_HEADER_SIZE 8

static Buffer *
bufferioLoadPNG (FILE *file, int w, int h)
{
  /*
  Y_TRACE ( "Loading PNG" );
 
  fpos_t start;
  if (fgetpos (file, &start))
    return NULL;
  
  // Check if the supplied file is in the correct format...

  png_bytep header;

  header = (png_bytep) malloc(PNG_HEADER_SIZE*sizeof(png_byte));

  if (fread (header, 1, PNG_HEADER_SIZE, file) != PNG_HEADER_SIZE)
    return NULL;

  fsetpos (file, &start);

  if (png_sig_cmp (header, (png_size_t) 0, PNG_HEADER_SIZE) != 0)
    {
      return NULL;
    }

  png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
    (png_voidp) NULL, NULL, NULL);

  if (!png_ptr)
    return NULL;

  png_infop info_ptr = png_create_info_struct (png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct (&png_ptr,
                               (png_infopp) NULL, (png_infopp) NULL);
      return NULL;
    }

  png_infop end_info = png_create_info_struct (png_ptr);
  if (!end_info)
    {
      png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
      return NULL;
    }

  if (setjmp (png_jmpbuf (png_ptr)))
    {
      png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
      fsetpos (file, &start);
      return NULL;
    }

  png_init_io (png_ptr, file);
  png_read_info (png_ptr, info_ptr);

  unsigned int width = png_get_image_width (png_ptr, info_ptr);
  unsigned int height = png_get_image_height (png_ptr, info_ptr);
  unsigned char colortype = png_get_color_type (png_ptr, info_ptr);

  struct PixelBuffer *pixbuf = NULL;

  if (colortype == PNG_COLOR_TYPE_RGB)
    {
      // build an RGBBuffer *TODO*
      abort ();
    }
  else if (colortype == PNG_COLOR_TYPE_RGB_ALPHA)
    {
      // build an RGBABuffer
       
      
      // we use machine endian ARGB not big endian RGBA
       
  
#if __BYTE_ORDER == __LITTLE_ENDIAN
      png_set_bgr(png_ptr);
#else
      png_set_swap_alpha(png_ptr); 
#endif

      unsigned int dw = 16;
      unsigned int dh = 16;

      while (dw < width)
        dw *= 2;
      while (dh < height)
        dh *= 2;

      uint32_t *data = ymalloc (dw * dh * sizeof (uint32_t));
      uint32_t *row = data;

      for (unsigned int i=0; i < height; i++)
        {
          png_read_row (png_ptr, (png_bytep)row , NULL);
          row += dw;
        }

        pixbuf = pixelbufferCreateFromData (&Y_COLORSPACE_RGBA, width, height, dw, dw*dh, (uint8_t *)data);
    }

  png_read_end (png_ptr, end_info);
  png_destroy_read_struct (&png_ptr, &info_ptr, &end_info);
 Y_TRACE ( "Done Loading PNG" );
  return &pixbuf->buffer;
  */
  return NULL;
}

static const char *bufferioLoadPNGExtensions[] = { "png", NULL };

void
bufferioInitialise (void)
{
  handlers = new_llist ();
  bufferRegisterFileHandler (bufferioLoadPNG, bufferioLoadPNGExtensions);
}

