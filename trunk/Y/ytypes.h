/*
 *   Copyright (C) 2005 Dustin Norlander <dustin@dustismo.com>
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
 
#ifndef Y_YTYPES
#define Y_YTYPES

#include <inttypes.h>
#include <stdbool.h>

//only use these for object property hooks
//todo: make datatypes universal

#define ybool uint32
#define t_ybool t_uint32

#define TRUE 1
#define FALSE 0

/*
 * These make conversions from gtk and glib simplier
 */

typedef void* ypointer;
typedef const void *yconstpointer;

typedef uint32_t        (*YHashFunc)            (yconstpointer  key);
typedef void            (*YHFunc)               (ypointer       key,
                                                 ypointer       value,
                                                 ypointer       user_data);
typedef bool	        (*YEqualFunc)           (yconstpointer  a,
                                                 yconstpointer  b);                                             
typedef void            (*YDestroyNotify)       (ypointer       data);

#endif
