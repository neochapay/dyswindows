/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */
 
/*
 * Modified for Y-Windows system by Dustin Norlander 2005
 */

#ifndef Y_YHASHTABLE
#define Y_YHASHTABLE

#include <Y/ytypes.h>

typedef struct _YHashTable  YHashTable;

typedef bool  (*YHRFunc)  (ypointer  key,
                               ypointer  value,
                               ypointer  user_data);

/* Hash tables
 */
YHashTable* y_hash_table_new		   (YHashFunc	    hash_func,
					    YEqualFunc	    key_equal_func);
YHashTable* y_hash_table_new_full      	   (YHashFunc	    hash_func,
					    YEqualFunc	    key_equal_func,
					    YDestroyNotify  key_destroy_func,
					    YDestroyNotify  value_destroy_func);
void	    y_hash_table_destroy	   (YHashTable	   *hash_table);
void	    y_hash_table_insert		   (YHashTable	   *hash_table,
					    ypointer	    key,
					    ypointer	    value);
void        y_hash_table_replace           (YHashTable     *hash_table,
					    ypointer	    key,
					    ypointer	    value);
bool    y_hash_table_remove		   (YHashTable	   *hash_table,
					    yconstpointer   key);
bool    y_hash_table_steal             (YHashTable     *hash_table,
					    yconstpointer   key);
ypointer    y_hash_table_lookup		   (YHashTable	   *hash_table,
					    yconstpointer   key);
bool    y_hash_table_lookup_extended   (YHashTable	   *hash_table,
					    yconstpointer   lookup_key,
					    ypointer	   *orig_key,
					    ypointer	   *value);
void	    y_hash_table_foreach	   (YHashTable	   *hash_table,
					    YHFunc	    func,
					    ypointer	    user_data);
ypointer    y_hash_table_find	   (YHashTable	   *hash_table,
					    YHRFunc	    predicate,
					    ypointer	    user_data);
uint32_t	    y_hash_table_foreach_remove	   (YHashTable	   *hash_table,
					    YHRFunc	    func,
					    ypointer	    user_data);
uint32_t	    y_hash_table_foreach_steal	   (YHashTable	   *hash_table,
					    YHRFunc	    func,
					    ypointer	    user_data);
uint32_t	    y_hash_table_size		   (YHashTable	   *hash_table);

/* Hash Functions
 */
bool y_str_equal (yconstpointer  v,
                      yconstpointer  v2);
uint32_t    y_str_hash  (yconstpointer  v);

bool y_int_equal (yconstpointer  v,
                      yconstpointer  v2);
uint32_t    y_int_hash  (yconstpointer  v);

/* This "hash" function will just return the key's address as an
 * unsigned integer. Useful for hashing on plain addresses or
 * simple integer values.
 * Passing NULL into y_hash_table_new() as YHRFunc has the
 * same effect as passing y_direct_hash().
 */
uint32_t    y_direct_hash  (yconstpointer  v) /*G_GNUC_CONST*/;
bool y_direct_equal (yconstpointer  v,
                         yconstpointer  v2) /*G_GNUC_CONST*/;

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
typedef void            (*YFreeFunc)            (ypointer       data);

#endif
