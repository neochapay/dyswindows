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
 * MT safe
 */
#include <stdlib.h>
#include <Y/util/yprimes.h>
#include <Y/util/yhash.h>
#include <Y/util/yutil.h>

#define HASH_TABLE_MIN_SIZE 11
#define HASH_TABLE_MAX_SIZE 13845163


typedef struct _YHashNode      YHashNode;

struct _YHashNode
{
  ypointer   key;
  ypointer   value;
  YHashNode *next;
};

struct _YHashTable
{
  uint32_t             size;
  uint32_t             nnodes;
  YHashNode      **nodes;
  YHashFunc        hash_func;
  YEqualFunc       key_equal_func;
  YDestroyNotify   key_destroy_func;
  YDestroyNotify   value_destroy_func;
};

#define Y_HASH_TABLE_RESIZE(hash_table)				\
   {								\
     if ((hash_table->size >= 3 * hash_table->nnodes &&	        \
	  hash_table->size > HASH_TABLE_MIN_SIZE) ||		\
	 (3 * hash_table->size <= hash_table->nnodes &&	        \
	  hash_table->size < HASH_TABLE_MAX_SIZE))		\
	   y_hash_table_resize (hash_table);			\
   } 

static void		y_hash_table_resize	  (YHashTable	  *hash_table);
static YHashNode**	y_hash_table_lookup_node  (YHashTable     *hash_table,
                                                   yconstpointer   key);
static YHashNode*	y_hash_node_new		  (ypointer	   key,
                                                   ypointer        value);
static void		y_hash_node_destroy	  (YHashNode	  *hash_node,
                                                   YDestroyNotify  key_destroy_func,
                                                   YDestroyNotify  value_destroy_func);
static void		y_hash_nodes_destroy	  (YHashNode	  *hash_node,
						  YDestroyNotify   key_destroy_func,
						  YDestroyNotify   value_destroy_func);
static uint32_t y_hash_table_foreach_remove_or_steal (YHashTable     *hash_table,
                                                   YHRFunc	   func,
                                                   ypointer	   user_data,
                                                   bool        notify);


/**
 * y_hash_table_new:
 * @hash_func: a function to create a hash value from a key.
 *   Hash values are used to determine where keys are stored within the
 *   #YHashTable data structure. The g_direct_hash(), g_int_hash() and 
 *   g_str_hash() functions are provided for some common types of keys. 
 *   If hash_func is %NULL, g_direct_hash() is used.
 * @key_equal_func: a function to check two keys for equality.  This is
 *   used when looking up keys in the #YHashTable.  The g_direct_equal(),
 *   g_int_equal() and g_str_equal() functions are provided for the most
 *   common types of keys. If @key_equal_func is %NULL, keys are compared
 *   directly in a similar fashion to g_direct_equal(), but without the
 *   overhead of a function call.
 *
 * Creates a new #YHashTable.
 * 
 * Return value: a new #YHashTable.
 **/
YHashTable*
y_hash_table_new (YHashFunc    hash_func,
		  YEqualFunc   key_equal_func)
{
  return y_hash_table_new_full (hash_func, key_equal_func, NULL, NULL);
}


/**
 * y_hash_table_new_full:
 * @hash_func: a function to create a hash value from a key.
 * @key_equal_func: a function to check two keys for equality.
 * @key_destroy_func: a function to free the memory allocated for the key 
 *   used when removing the entry from the #YHashTable or %NULL if you 
 *   don't want to supply such a function.
 * @value_destroy_func: a function to free the memory allocated for the 
 *   value used when removing the entry from the #YHashTable or %NULL if 
 *   you don't want to supply such a function.
 * 
 * Creates a new #YHashTable like y_hash_table_new() and allows to specify
 * functions to free the memory allocated for the key and value that get 
 * called when removing the entry from the #YHashTable.
 * 
 * Return value: a new #YHashTable.
 **/
YHashTable*
y_hash_table_new_full (YHashFunc       hash_func,
		       YEqualFunc      key_equal_func,
		       YDestroyNotify  key_destroy_func,
		       YDestroyNotify  value_destroy_func)
{
  YHashTable *hash_table;
  uint32_t i;
  
  hash_table = ymalloc (sizeof (YHashTable));
  hash_table->size               = HASH_TABLE_MIN_SIZE;
  hash_table->nnodes             = 0;
  hash_table->hash_func          = hash_func ? hash_func : y_direct_hash;
  hash_table->key_equal_func     = key_equal_func;
  hash_table->key_destroy_func   = key_destroy_func;
  hash_table->value_destroy_func = value_destroy_func;
  hash_table->nodes              = ymalloc (sizeof(YHashNode) * hash_table->size);
  
  for (i = 0; i < hash_table->size; i++)
    hash_table->nodes[i] = NULL;
  
  return hash_table;
}

/**
 * y_hash_table_destroy:
 * @hash_table: a #YHashTable.
 * 
 * Destroys the #YHashTable. If keys and/or values are dynamically 
 * allocated, you should either free them first or create the #YHashTable
 * using y_hash_table_new_full(). In the latter case the destroy functions 
 * you supplied will be called on all keys and values before destroying 
 * the #YHashTable.
 **/
void
y_hash_table_destroy (YHashTable *hash_table)
{
  uint32_t i;
  
  if (hash_table == NULL)
  	return;
  
  for (i = 0; i < hash_table->size; i++)
    y_hash_nodes_destroy (hash_table->nodes[i], 
			  hash_table->key_destroy_func,
			  hash_table->value_destroy_func);
  
  yfree (hash_table->nodes);
  yfree (hash_table);
}

static inline YHashNode**
y_hash_table_lookup_node (YHashTable	*hash_table,
			  yconstpointer	 key)
{
  YHashNode **node;
  
  node = &hash_table->nodes
    [(* hash_table->hash_func) (key) % hash_table->size];
  
  /* Hash table lookup needs to be fast.
   *  We therefore remove the extra conditional of testing
   *  whether to call the key_equal_func or not from
   *  the inner loop.
   */
  if (hash_table->key_equal_func)
    while (*node && !(*hash_table->key_equal_func) ((*node)->key, key))
      node = &(*node)->next;
  else
    while (*node && (*node)->key != key)
      node = &(*node)->next;
  
  return node;
}

/**
 * y_hash_table_lookup:
 * @hash_table: a #YHashTable.
 * @key: the key to look up.
 * 
 * Looks up a key in a #YHashTable. Note that this function cannot
 * distinguish between a key that is not present and one which is present
 * and has the value %NULL. If you need this distinction, use
 * y_hash_table_lookup_extended().
 * 
 * Return value: the associated value, or %NULL if the key is not found.
 **/
ypointer
y_hash_table_lookup (YHashTable	  *hash_table,
		     yconstpointer key)
{
  YHashNode *node;
  
  if (hash_table == NULL)
  	return NULL;
  
  node = *y_hash_table_lookup_node (hash_table, key);
  
  return node ? node->value : NULL;
}

/**
 * y_hash_table_lookup_extended:
 * @hash_table: a #YHashTable.
 * @lookup_key: the key to look up.
 * @orig_key: returns the original key.
 * @value: returns the value associated with the key.
 * 
 * Looks up a key in the #YHashTable, returning the original key and the
 * associated value and a #bool which is %TRUE if the key was found. This 
 * is useful if you need to free the memory allocated for the original key, 
 * for example before calling y_hash_table_remove().
 * 
 * Return value: %TRUE if the key was found in the #YHashTable.
 **/
bool
y_hash_table_lookup_extended (YHashTable    *hash_table,
			      yconstpointer  lookup_key,
			      ypointer	    *orig_key,
			      ypointer	    *value)
{
  YHashNode *node;
  
  if (hash_table == NULL)
  	return FALSE;
  
  node = *y_hash_table_lookup_node (hash_table, lookup_key);
  
  if (node)
    {
      if (orig_key)
	*orig_key = node->key;
      if (value)
	*value = node->value;
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * y_hash_table_insert:
 * @hash_table: a #YHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #YHashTable.
 * 
 * If the key already exists in the #YHashTable its current value is replaced
 * with the new value. If you supplied a @value_destroy_func when creating the 
 * #YHashTable, the old value is freed using that function. If you supplied
 * a @key_destroy_func when creating the #YHashTable, the passed key is freed 
 * using that function.
 **/
void
y_hash_table_insert (YHashTable *hash_table,
		     ypointer	 key,
		     ypointer	 value)
{
  YHashNode **node;
  
  if (hash_table == NULL)
  	return;
  
  node = y_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      /* do not reset node->key in this place, keeping
       * the old key is the intended behaviour. 
       * y_hash_table_replace() can be used instead.
       */

      /* free the passed key */
      if (hash_table->key_destroy_func)
	hash_table->key_destroy_func (key);
      
      if (hash_table->value_destroy_func)
	hash_table->value_destroy_func ((*node)->value);

      (*node)->value = value;
    }
  else
    {
      *node = y_hash_node_new (key, value);
      hash_table->nnodes++;
      Y_HASH_TABLE_RESIZE (hash_table);
    }
}

/**
 * y_hash_table_replace:
 * @hash_table: a #YHashTable.
 * @key: a key to insert.
 * @value: the value to associate with the key.
 * 
 * Inserts a new key and value into a #YHashTable similar to 
 * y_hash_table_insert(). The difference is that if the key already exists 
 * in the #YHashTable, it gets replaced by the new key. If you supplied a 
 * @value_destroy_func when creating the #YHashTable, the old value is freed 
 * using that function. If you supplied a @key_destroy_func when creating the 
 * #YHashTable, the old key is freed using that function. 
 **/
void
y_hash_table_replace (YHashTable *hash_table,
		      ypointer	  key,
		      ypointer	  value)
{
  YHashNode **node;
  
  if (hash_table == NULL)
  	return;
  
  node = y_hash_table_lookup_node (hash_table, key);
  
  if (*node)
    {
      if (hash_table->key_destroy_func)
	hash_table->key_destroy_func ((*node)->key);
      
      if (hash_table->value_destroy_func)
	hash_table->value_destroy_func ((*node)->value);

      (*node)->key   = key;
      (*node)->value = value;
    }
  else
    {
      *node = y_hash_node_new (key, value);
      hash_table->nnodes++;
      Y_HASH_TABLE_RESIZE (hash_table);
    }
}

/**
 * y_hash_table_remove:
 * @hash_table: a #YHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #YHashTable.
 *
 * If the #YHashTable was created using y_hash_table_new_full(), the
 * key and value are freed using the supplied destroy functions, otherwise
 * you have to make sure that any dynamically allocated values are freed 
 * yourself.
 * 
 * Return value: %TRUE if the key was found and removed from the #YHashTable.
 **/
bool
y_hash_table_remove (YHashTable	   *hash_table,
		     yconstpointer  key)
{
  YHashNode **node, *dest;
  
  if (hash_table == NULL)
  	return FALSE;
  
  node = y_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      y_hash_node_destroy (dest, 
			   hash_table->key_destroy_func,
			   hash_table->value_destroy_func);
      hash_table->nnodes--;
  
      Y_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}

/**
 * y_hash_table_steal:
 * @hash_table: a #YHashTable.
 * @key: the key to remove.
 * 
 * Removes a key and its associated value from a #YHashTable without
 * calling the key and value destroy functions.
 *
 * Return value: %TRUE if the key was found and removed from the #YHashTable.
 **/
bool
y_hash_table_steal (YHashTable    *hash_table,
                    yconstpointer  key)
{
  YHashNode **node, *dest;
  
  if (hash_table == NULL)
  	return FALSE;
  
  node = y_hash_table_lookup_node (hash_table, key);
  if (*node)
    {
      dest = *node;
      (*node) = dest->next;
      y_hash_node_destroy (dest, NULL, NULL);
      hash_table->nnodes--;
  
      Y_HASH_TABLE_RESIZE (hash_table);

      return TRUE;
    }

  return FALSE;
}

/**
 * y_hash_table_foreach_remove:
 * @hash_table: a #YHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #YHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #YHashTable. If you supplied key or value destroy functions when creating
 * the #YHashTable, they are used to free the memory allocated for the removed
 * keys and values.
 * 
 * Return value: the number of key/value pairs removed.
 **/
uint32_t
y_hash_table_foreach_remove (YHashTable	*hash_table,
			     YHRFunc	 func,
			     ypointer	 user_data)
{
  if (hash_table == NULL)
  	return 0;
  if (func == NULL)
  	return 0;
  
  return y_hash_table_foreach_remove_or_steal (hash_table, func, user_data, TRUE);
}

/**
 * y_hash_table_foreach_steal:
 * @hash_table: a #YHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each key/value pair in the #YHashTable.
 * If the function returns %TRUE, then the key/value pair is removed from the
 * #YHashTable, but no key or value destroy functions are called.
 * 
 * Return value: the number of key/value pairs removed.
 **/
uint32_t
y_hash_table_foreach_steal (YHashTable *hash_table,
                            YHRFunc	func,
                            ypointer	user_data)
{
  if (hash_table == NULL)
  	return 0;
  if (func == NULL)
  	return 0;
  
  return y_hash_table_foreach_remove_or_steal (hash_table, func, user_data, FALSE);
}

static uint32_t
y_hash_table_foreach_remove_or_steal (YHashTable *hash_table,
                                      YHRFunc	  func,
                                      ypointer	  user_data,
                                      bool    notify)
{
  YHashNode *node, *prev;
  uint32_t i;
  uint32_t deleted = 0;
  
  for (i = 0; i < hash_table->size; i++)
    {
    restart:
      
      prev = NULL;
      
      for (node = hash_table->nodes[i]; node; prev = node, node = node->next)
	{
	  if ((* func) (node->key, node->value, user_data))
	    {
	      deleted += 1;
	      
	      hash_table->nnodes -= 1;
	      
	      if (prev)
		{
		  prev->next = node->next;
		  y_hash_node_destroy (node,
				       notify ? hash_table->key_destroy_func : NULL,
				       notify ? hash_table->value_destroy_func : NULL);
		  node = prev;
		}
	      else
		{
		  hash_table->nodes[i] = node->next;
		  y_hash_node_destroy (node,
				       notify ? hash_table->key_destroy_func : NULL,
				       notify ? hash_table->value_destroy_func : NULL);
		  goto restart;
		}
	    }
	}
    }
  
  Y_HASH_TABLE_RESIZE (hash_table);
  
  return deleted;
}

/**
 * y_hash_table_foreach:
 * @hash_table: a #YHashTable.
 * @func: the function to call for each key/value pair.
 * @user_data: user data to pass to the function.
 * 
 * Calls the given function for each of the key/value pairs in the
 * #YHashTable.  The function is passed the key and value of each
 * pair, and the given @user_data parameter.  The hash table may not
 * be modified while iterating over it (you can't add/remove
 * items). To remove all items matching a predicate, use
 * y_hash_table_remove().
 **/
void
y_hash_table_foreach (YHashTable *hash_table,
		      YHFunc	  func,
		      ypointer	  user_data)
{
  YHashNode *node;
  uint32_t i;
  
  if (hash_table == NULL)
  	return;
  if (func == NULL)
  	return;
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      (* func) (node->key, node->value, user_data);
}

/**
 * y_hash_table_find:
 * @hash_table: a #YHashTable.
 * @predicate:  function to test the key/value pairs for a certain property.
 * @user_data:  user data to pass to the function.
 * 
 * Calls the given function for key/value pairs in the #YHashTable until 
 * @predicate returns %TRUE.  The function is passed the key and value of 
 * each pair, and the given @user_data parameter. The hash table may not
 * be modified while iterating over it (you can't add/remove items). 
 *
 * Return value: The value of the first key/value pair is returned, for which 
 * func evaluates to %TRUE. If no pair with the requested property is found, 
 * %NULL is returned.
 *
 * Since: 2.4
 **/
ypointer
y_hash_table_find (YHashTable	   *hash_table,
                   YHRFunc	    predicate,
                   ypointer	    user_data)
{
  YHashNode *node;
  uint32_t i;
  
  if (hash_table == NULL)
  	return NULL;
  if (predicate == NULL)
  	return NULL;
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = node->next)
      if (predicate (node->key, node->value, user_data))
        return node->value;       
  return NULL;
}

/**
 * y_hash_table_size:
 * @hash_table: a #YHashTable.
 * 
 * Returns the number of elements contained in the #YHashTable.
 * 
 * Return value: the number of key/value pairs in the #YHashTable.
 **/
uint32_t
y_hash_table_size (YHashTable *hash_table)
{
  if (hash_table == NULL)
  	return 0;
  
  return hash_table->nnodes;
}

static void
y_hash_table_resize (YHashTable *hash_table)
{
  YHashNode **new_nodes;
  YHashNode *node;
  YHashNode *next;
  uint32_t hash_val;
  int32_t new_size;
  uint32_t i;

  new_size = y_spaced_primes_closest (hash_table->nnodes);
  new_size = CLAMP (new_size, HASH_TABLE_MIN_SIZE, HASH_TABLE_MAX_SIZE);
 
  new_nodes = ycalloc (new_size, sizeof(YHashNode));
  
  for (i = 0; i < hash_table->size; i++)
    for (node = hash_table->nodes[i]; node; node = next)
      {
	next = node->next;

	hash_val = (* hash_table->hash_func) (node->key) % new_size;

	node->next = new_nodes[hash_val];
	new_nodes[hash_val] = node;
      }
  
  yfree (hash_table->nodes);
  hash_table->nodes = new_nodes;
  hash_table->size = new_size;
}

static YHashNode*
y_hash_node_new (ypointer key,
		 ypointer value)
{
  YHashNode *hash_node;

  hash_node = ymalloc (sizeof (YHashNode));
 
  hash_node->key = key;
  hash_node->value = value;
  hash_node->next = NULL;
  
  return hash_node;
}

static void
y_hash_node_destroy (YHashNode      *hash_node,
		     YDestroyNotify  key_destroy_func,
		     YDestroyNotify  value_destroy_func)
{
  if (key_destroy_func)
    key_destroy_func (hash_node->key);
  if (value_destroy_func)
    value_destroy_func (hash_node->value);
  yfree (hash_node);
}

static void
y_hash_nodes_destroy (YHashNode *hash_node,
		      YFreeFunc  key_destroy_func,
		      YFreeFunc  value_destroy_func)
{
  while (hash_node)
    {
      YHashNode *next = hash_node->next;

      if (key_destroy_func)
	key_destroy_func (hash_node->key);
      if (value_destroy_func)
	value_destroy_func (hash_node->value);

      yfree (hash_node);
      hash_node = next;
    }  
}

/*
 * Default hash function
 * simply uses the uint value of the pointer
 */
uint32_t
y_direct_hash (yconstpointer v)
{
  return *(const int32_t*) v;
}

bool
y_direct_equal (yconstpointer v1,
		yconstpointer v2)
{
  return v1 == v2;
}

bool
y_int_equal (yconstpointer v1,
	     yconstpointer v2)
{
  return *((const int32_t*) v1) == *((const int32_t*) v2);
}

uint32_t
y_int_hash (yconstpointer v)
{
  return *(const int32_t*) v;
}
