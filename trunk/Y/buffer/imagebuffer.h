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

#ifndef Y_CAIROBUFFER_H
#define Y_CAIROBUFFER_H

#include <Y/y.h>
#include <Y/screen/renderer.h>
#include <Y/buffer/bufferclass.h>

#include <stdint.h>

struct ImageBuffer_t;
typedef struct ImageBuffer_t ImageBuffer;

/** \brief Create a new buffer of the given size
 */
ImageBuffer *image_buffer_create (cairo_format_t format, uint w, uint h);

ImageBuffer *image_buffer_create_from_data (cairo_format_t format,
						uint w, uint h, 
						uint stride_bytes,
						uint8_t *data);
uint8_t *image_buffer_get_pixel_data (ImageBuffer *self);
uint32_t image_buffer_get_stride_bytes (ImageBuffer *self);

#endif
