/************************************************************************
 *   Copyright (C) Andrew Suffield <asuffield@debian.org>
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

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <Y/y.h>
#include <Y/input/ykm_p.h>
#include <Y/input/ykbmap_p.h>
#include <Y/input/ykbmap.h>
#include <Y/util/yutil.h>
#include <Y/util/dbuffer.h>
#include <Y/util/rbtree.h>

static int
ykbMapEdgeCmp (const void *n1_v, const void *n2_v)
{
  const struct ykbMapEdge *n1 = n1_v;
  const struct ykbMapEdge *n2 = n2_v;
  if (n1->keycode == n2->keycode)
    {
      if (n1->modifiers == n2->modifiers)
        {
          if (n1->direction == n2->direction)
            return 0;
          else if (n1->direction < n2->direction)
            return -1;
          else
            return 1;
        }
      else if (n1->modifiers < n2->modifiers)
        return -1;
      else
        return 1;
    }
  else if (n1->keycode < n2->keycode)
    return -1;
  else
    return 1; 
}

static int
ykbMapMaskEdgeListCmp (const void *n1_v, const void *n2_v)
{
  const struct ykbMapMaskEdgeList *n1 = n1_v;
  const struct ykbMapMaskEdgeList *n2 = n2_v;
  if (n1->keycode == n2->keycode)
    {
      if (n1->direction == n2->direction)
        return 0;
      else if (n1->direction < n2->direction)
        return -1;
      else
        return 1;
    }
  else if (n1->keycode < n2->keycode)
    return -1;
  else
    return 1; 
}

static int
ykbKeyNameComparisonFunction (const void *n1_v, const void *n2_v)
{
  const struct ykbKeyName *n1 = n1_v;
  const struct ykbKeyName *n2 = n2_v;
  if (n1->keycode == n2->keycode)
    return 0;
  else if (n1->keycode < n2->keycode)
    return -1;
  else
    return 1; 
}

static struct ykbAction *
ykbActionCreate(void)
{
  struct ykbAction *action = ymalloc(sizeof(*action));
  action->str1 = NULL;
  action->str2 = NULL;
  action->modifiers = 0;
  return action;
}

static void
ykbActionDestroy(struct ykbAction *action)
{
  if (!action)
    return;
  yfree(action->str1);
  yfree(action->str2);
  yfree(action);
}

static struct ykbMapNode *
ykbMapNodeAdd(struct ykbMap *map)
{
  struct ykbMapNode *node = ymalloc(sizeof(*node));
  node->edges = new_rbtree(&ykbMapEdgeCmp, &ykbMapEdgeCmp);
  node->mask_edge_lists = new_rbtree(&ykbMapMaskEdgeListCmp, &ykbMapMaskEdgeListCmp);
  llist_add_tail (map->nodes, node);
  return node;
}

static struct ykbMapEdge *
ykbMapEdgeAdd(struct ykbMapNode *from, bool direction, uint16_t keycode, uint16_t modifiers)
{
  struct ykbMapEdge *edge = ymalloc(sizeof(*edge));
  edge->direction = direction;
  edge->keycode = keycode;
  edge->modifiers = modifiers;
  edge->destination = NULL;
  edge->actions = new_llist ();
  rbtree_insert(from->edges, edge);
  return edge;
}

static struct ykbMapMaskEdgeList *
ykbMapMaskEdgeListAdd(struct ykbMapNode *from, bool direction, uint16_t keycode)
{
  struct ykbMapMaskEdgeList *list = ymalloc(sizeof(*list));
  list->direction = direction;
  list->keycode = keycode;
  list->mask_edges = new_llist ();
  rbtree_insert(from->mask_edge_lists, list);
  return list;
}

static struct ykbMapMaskEdge *
ykbMapMaskEdgeAdd(struct ykbMapNode *from, bool direction, uint16_t keycode, uint16_t mask, uint16_t modifiers)
{
  struct ykbMapMaskEdge *edge = ymalloc(sizeof(*edge));
  edge->mask = mask;
  edge->edge = ymalloc(sizeof(*edge->edge));
  edge->edge->direction = direction;
  edge->edge->keycode = keycode;
  edge->edge->modifiers = modifiers;
  edge->edge->destination = NULL;
  edge->edge->actions = new_llist ();
  struct ykbMapMaskEdgeList key = {.direction = direction, .keycode = keycode, .mask_edges = NULL};
  struct rbtree_node *node = rbtree_find(from->mask_edge_lists, &key);
  struct ykbMapMaskEdgeList *list;
  if (node)
    list = rbtree_node_data(node);
  else
    list = ykbMapMaskEdgeListAdd(from, direction, keycode);
  llist_add_tail (list->mask_edges, edge);
  return edge;
}

/* Note that we don't destroy the destination node; there is diamond
 * structure here, so we store all the nodes in a list in the map and
 * destroy those at the top level
 */
static void
ykbMapEdgeDestroy(struct ykbMapEdge *edge)
{
  if (!edge)
    return;
  llist_destroy (edge->actions, ykbActionDestroy);
  yfree(edge);
}

static void
ykbMapMaskEdgeDestroy(struct ykbMapMaskEdge *edge)
{
  if (!edge)
    return;
  ykbMapEdgeDestroy(edge->edge);
  yfree(edge);
}

static void
ykbMapMaskEdgeListDestroy(struct ykbMapMaskEdgeList *list)
{
  if (!list)
    return;
  llist_destroy (list->mask_edges, ykbMapMaskEdgeDestroy);
  yfree(list);
}

static void
ykbMapNodeDestroy(struct ykbMapNode *node)
{
  if (!node)
    return;
  rbtree_destroy(node->edges, ykbMapEdgeDestroy);
  rbtree_destroy(node->mask_edge_lists, ykbMapMaskEdgeListDestroy);
  yfree(node);
}

static struct ykbMapEdge *
ykbMapFindEdge(struct ykbMapNode *node, bool direction, uint16_t keycode, uint16_t modifiers)
{
  struct ykbMapEdge key = {.direction = direction, .keycode = keycode, .modifiers = modifiers};
  struct rbtree_node *n = rbtree_find(node->edges, &key);
  return rbtree_node_data(n);
}

/* This function finds an *exact* match, not a mask match */
static struct ykbMapEdge *
ykbMapFindMaskEdge(struct ykbMapNode *node, bool direction, uint16_t keycode, uint16_t mask, uint16_t modifiers)
{
  struct ykbMapMaskEdgeList mask_key = {.direction = direction, .keycode = keycode};
  struct rbtree_node *n = rbtree_find(node->mask_edge_lists, &mask_key);;
  if (!n)
    return NULL;
  struct ykbMapMaskEdgeList *list = rbtree_node_data(n);

  struct ListIterator *i;
  for (struct llist_node *n = llist_head (list->mask_edges);
       n != NULL;
       n = llist_node_next (n))
    {
      struct ykbMapMaskEdge *mask_edge = llist_node_data (n);
      if (mask == mask_edge->mask && modifiers == mask_edge->edge->modifiers)
        return mask_edge->edge;
    }

  return NULL;
}

/* This function finds either an exact match, or a mask match */
struct ykbMapEdge *
ykbMapLookup(struct ykbMapNode *node, bool direction, uint16_t keycode, uint16_t modifiers)
{
  /* First we try for an exact match */
  struct ykbMapEdge *edge = ykbMapFindEdge(node, direction, keycode, modifiers);
  if (edge)
    return edge;

  /* Then we try for a mask match */
  struct ykbMapMaskEdgeList mask_key = {.direction = direction, .keycode = keycode};
  struct rbtree_node *n = rbtree_find(node->mask_edge_lists, &mask_key);
  if (!n)
    return NULL;
  struct ykbMapMaskEdgeList *list = rbtree_node_data(n);

  /* We have a list; try them each in turn */
  for (struct llist_node *n = llist_head (list->mask_edges); 
       n != NULL;
       n = llist_node_next (n))
    {
      struct ykbMapMaskEdge *mask_edge = llist_node_data (n);
      edge = mask_edge->edge;
      /* This is the definition of a mask match */
      if ((modifiers & mask_edge->mask) == edge->modifiers)
        {
          assert(direction == edge->direction);
          assert(keycode == edge->keycode);
          return edge;
        }
    }

  return NULL;
}

static struct ykbKeycode *
ykbKeycodeCreate(const char *name, uint16_t value, uint16_t modifiers_value, uint16_t modifiers_mask)
{
  struct ykbKeycode *keycode = ymalloc(sizeof(*keycode));
  keycode->name = ystrdup(name);
  keycode->value = value;
  keycode->modifiers_value = modifiers_value;
  keycode->modifiers_mask = modifiers_mask;
  return keycode;
}

static void
ykbKeycodeDestroy(struct ykbKeycode *keycode)
{
  if (!keycode)
    return;
  yfree(keycode->name);
  yfree(keycode);
}

static int
ykbKeycodeKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykbKeycode *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykbKeycodeCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykbKeycode *obj1 = obj1_v;
  const struct ykbKeycode *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykbModifier *
ykbModifierCreate(const char *name, uint16_t value)
{
  struct ykbModifier *modifier = ymalloc(sizeof(*modifier));
  modifier->name = ystrdup(name);
  modifier->value = value;
  return modifier;
}

static void
ykbModifierDestroy(struct ykbModifier *modifier)
{
  if (!modifier)
    return;
  yfree(modifier->name);
  yfree(modifier);
}

static int
ykbModifierKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykbModifier *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykbModifierCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykbModifier *obj1 = obj1_v;
  const struct ykbModifier *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykbKeyName *
ykbKeyNameCreate(uint16_t keycode, uint16_t modifiers_value, uint16_t modifiers_mask, const char *str)
{
  struct ykbKeyName *kn = ymalloc(sizeof(*kn));
  kn->keycode = keycode;
  kn->str = ystrdup(str);
  kn->modifiers_value = modifiers_value;
  kn->modifiers_mask = modifiers_mask;
  return kn;
}

static void
ykbKeyNameDestroy(struct ykbKeyName *kn)
{
  if (!kn)
    return;
  yfree(kn->str);
  yfree(kn);
}

static struct ykbKeyNameList *
ykbKeyNameListCreate(uint16_t keycode)
{
  struct ykbKeyNameList *knl = ymalloc(sizeof(*knl));
  knl->keycode = keycode;
  knl->list = new_llist();
  return knl;
}

static void
ykbKeyNameListDestroy(struct ykbKeyNameList *knl)
{
  if (!knl)
    return;
  llist_destroy(knl->list, ykbKeyNameDestroy);
  yfree(knl);
}

static int
ykbKeyNameListKey (const void *key_v, const void *obj_v)
{
  const uint16_t *keycode = key_v;
  const struct ykbKeyNameList *obj = obj_v;
  return *keycode - obj->keycode;
}

static int
ykbKeyNameListCmp (const void *obj1_v, const void *obj2_v)
{
  const struct ykbKeyNameList *obj1 = obj1_v;
  const struct ykbKeyNameList *obj2 = obj2_v;
  return obj1->keycode - obj2->keycode;
}

static struct ykbModifierName *
ykbModifierNameCreate(uint16_t modifiers, uint16_t mask, char *str)
{
  struct ykbModifierName *desc = ymalloc(sizeof(*desc));
  desc->modifiers = modifiers;
  desc->mask = mask;
  desc->str = ystrdup(str);
  return desc;
}

static void
ykbModifierNameDestroy(struct ykbModifierName *desc)
{
  if (!desc)
    return;
  yfree(desc->str);
  yfree(desc);
}

static struct ykbMap *
ykbMapCreate(void)
{
  struct ykbMap *map = ymalloc(sizeof(*map));
  map->nodes = new_llist();
  map->keycodes = new_rbtree(ykbKeycodeKey, ykbKeycodeCmp);
  map->modifiers = new_rbtree(ykbModifierKey, ykbModifierCmp);
  map->keycode_names = new_rbtree(ykbKeyNameListKey, ykbKeyNameListCmp);
  map->modifier_names = new_llist();
  map->start = ykbMapNodeAdd(map);
  return map;
}

static void
ykbMapDestroy(struct ykbMap *map)
{
  if (!map)
    return;
  llist_destroy(map->nodes, ykbMapNodeDestroy);
  llist_destroy(map->modifier_names, ykbModifierNameDestroy);
  rbtree_destroy(map->keycodes, ykbKeycodeDestroy);
  rbtree_destroy(map->modifiers, ykbModifierDestroy);
  rbtree_destroy(map->keycode_names, ykbKeyNameListDestroy);
  yfree(map);
}

static struct ykbmapActiveKeymap *
ykbmapActiveKeymapCreate(struct ykbmapKeymap *keymap)
{
  struct ykbmapActiveKeymap *ek = ymalloc(sizeof(*ek));
  ek->keymap = keymap;
  ek->option = NULL;
  return ek;
}

static struct ykbmapActiveKeymap *
ykbmapActiveKeymapCreateOption(struct ykbmapOption *option)
{
  struct ykbmapActiveKeymap *ek = ymalloc(sizeof(*ek));
  ek->keymap = NULL;
  ek->option = option;
  return ek;
}

static void
ykbmapActiveKeymapDestroy(struct ykbmapActiveKeymap *ek)
{
  if (!ek)
    return;
  yfree(ek);
}

static struct ykbmapKeymap *
ykbmapKeymapCreate(const char *name)
{
  struct ykbmapKeymap *keymap = ymalloc(sizeof(*keymap));
  keymap->name = ystrdup(name);
  keymap->maps_entry = NULL;
  return keymap;
}

static void
ykbmapKeymapDestroy(struct ykbmapKeymap *keymap)
{
  if (!keymap)
    return;
  yfree(keymap->name);
  yfree(keymap);
}

static int
ykbmapKeymapKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykbmapKeymap *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykbmapKeymapCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykbmapKeymap *obj1 = obj1_v;
  const struct ykbmapKeymap *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykbmapOption *
ykbmapOptionCreate(const char *name, const char *parent)
{
  struct ykbmapOption *option = ymalloc(sizeof(*option));
  option->name = ystrdup(name);
  option->parent = ystrdup(parent);
  option->keymap = NULL;
  option->maps_entry = NULL;
  return option;
}

static void
ykbmapOptionDestroy(struct ykbmapOption *option)
{
  if (!option)
    return;
  yfree(option->name);
  yfree(option->keymap);
  yfree(option->parent);
  yfree(option);
}

static int
ykbmapOptionKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykbmapOption *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykbmapOptionCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykbmapOption *obj1 = obj1_v;
  const struct ykbmapKeymap *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykbMapSet *
ykbMapSetCreate(void)
{
  struct ykbMapSet *mapset = ymalloc(sizeof(*mapset));
  mapset->map = ykbMapCreate();
  mapset->inhibit_rebuild = false;
  mapset->rebuild_pending = false;
  mapset->ykm = NULL;
  mapset->active_maps = new_llist();
  mapset->selected_maps = new_rbtree(ykbmapKeymapKey, ykbmapKeymapCmp);
  mapset->options = new_rbtree(ykbmapOptionKey, ykbmapOptionCmp);
  return mapset;
}

void
ykbMapSetDestroy(struct ykbMapSet *mapset)
{
  if (!mapset)
    return;
  ykbMapDestroy(mapset->map);
  rbtree_destroy(mapset->selected_maps, ykbmapKeymapDestroy);
  rbtree_destroy(mapset->options, ykbmapOptionDestroy);
  llist_destroy(mapset->active_maps, ykbmapActiveKeymapDestroy);
  yfree(mapset);
}

struct ykbKeyNameList *
ykbKeyLookup(struct ykbMap *map, uint16_t keycode)
{
  struct rbtree_node *node = rbtree_find(map->keycode_names, &keycode);
  struct ykbKeyNameList *knl = rbtree_node_data(node);
  return knl;
}

static void
ykbmapInsertKeycode(struct ykbMap *map, const char *name, uint16_t value, uint16_t modifiers_value, uint16_t modifiers_mask)
{
  struct rbtree_node *node = rbtree_find(map->keycodes, name);
  if (node)
    {
      struct ykbKeycode *keycode = rbtree_node_data(node);
      keycode->value = value;
      keycode->modifiers_value = modifiers_value;
      keycode->modifiers_mask = modifiers_mask;
    }
  else
    rbtree_insert(map->keycodes, ykbKeycodeCreate(name, value, modifiers_value, modifiers_mask));
}

static void
ykbmapInsertModifier(struct ykbMap *map, const char *name, uint16_t value)
{
  struct rbtree_node *node = rbtree_find(map->modifiers, name);
  if (node)
    {
      struct ykbModifier *modifier = rbtree_node_data(node);
      modifier->value = value;
    }
  else
    rbtree_insert(map->modifiers, ykbModifierCreate(name, value));
}

static void
ykbmapInsertKeyName(struct ykbMap *map, struct ykbKeycode *keycode, const char *name, uint16_t modifiers_value, uint16_t modifiers_mask)
{
  struct rbtree_node *node = rbtree_find(map->keycode_names, &keycode->value);
  struct ykbKeyNameList *knl;  
  if (node)
    knl = rbtree_node_data(node);
  else
    {
      knl = ykbKeyNameListCreate(keycode->value);
      rbtree_insert(map->keycode_names, knl);
    }

  modifiers_value |= keycode->modifiers_value;
  modifiers_mask &= keycode->modifiers_mask;

  for (struct llist_node *node = llist_head(knl->list); node; node = llist_node_next(node))
    {
      struct ykbKeyName *kn = llist_node_data(node);
      if (kn->keycode != keycode->value)
        continue;
      if (kn->modifiers_value != modifiers_value)
        continue;
      if (kn->modifiers_mask != modifiers_mask)
        continue;

      /* Found it */
      yfree(kn->str);
      kn->str = ystrdup(name);
      return;
    }

  /* Not there. Add it. */
  llist_add_tail(knl->list, ykbKeyNameCreate(keycode->value, modifiers_value, modifiers_mask, name));
}

static bool
ykbmapResolveKeycode(struct ykbMap *map, const char *name, struct ykbKeycode **pkeycode)
{
  struct rbtree_node *node = rbtree_find(map->keycodes, name);
  if (!node)
    return false;
  struct ykbKeycode *keycode = rbtree_node_data(node);
  if (pkeycode)
    *pkeycode = keycode;
  return true;
}

static bool
ykbmapResolveModifier(struct ykbMap *map, struct llist *modifier_values, uint16_t *pvalue, bool warn)
{
  uint16_t value = 0;
  for (struct llist_node *node = llist_head(modifier_values); node; node = llist_node_next(node))
    {
      struct ykmModifierValue *v = llist_node_data(node);
      if (!v->name)
        {
          value |= v->value;
          continue;
        }
              
      struct rbtree_node *node2 = rbtree_find(map->modifiers, v->name);
      if (!node2)
        {
          if (warn)
            Y_WARN("Failed to resolve modifier %s", v->name);
          return false;
        }

      struct ykbModifier *modifier = rbtree_node_data(node2);
      value |= modifier->value;
    }

  if (pvalue)
    *pvalue = value;
  return true;
}

static void
ykbmapInsertKeycodes(struct ykbMap *map, struct ykmKeymap *source)
{
  uint16_t no_mask = 0;
  no_mask = ~no_mask;

  /* First pass: note the aliases, insert the explicit values */
  struct llist *pending_keycodes = new_llist();
  for (struct rbtree_node *node = rbtree_head(source->keycodes); node; node = rbtree_node_next(node))
    {
      struct ykmKeycode *keycode = rbtree_node_data(node);
      if (keycode->alias)
        llist_add_tail(pending_keycodes, keycode);
      else
        ykbmapInsertKeycode(map, keycode->name, keycode->value, 0, no_mask);
    }

  /* All other passes: try to insert some values */
  while (llist_length(pending_keycodes) > 0)
    {
      uint32_t old_length = llist_length(pending_keycodes);
      struct llist_node *next;
      for (struct llist_node *node = llist_head(pending_keycodes); node; node = next)
        {
          next = llist_node_next(node);
          struct ykmKeycode *keycode = llist_node_data(node);

          struct ykbKeycode *alias_keycode;
          if (!ykbmapResolveKeycode(map, keycode->alias, &alias_keycode))
            continue;

          uint16_t modifiers_value;
          uint16_t modifiers_mask;

          if (!ykbmapResolveModifier(map, keycode->modifiers_value, &modifiers_value, false))
            continue;

          if (!ykbmapResolveModifier(map, keycode->modifiers_mask, &modifiers_mask, false))
            continue;

          ykbmapInsertKeycode(map, keycode->name, alias_keycode->value, modifiers_value, modifiers_mask);
          llist_node_delete(node);
        }

      /* Stop when we no longer have any matches */
      if (llist_length(pending_keycodes) == old_length)
        break;
    }

  for (struct llist_node *node = llist_head(pending_keycodes); node; node = llist_node_next(node))
    {
      struct ykmKeycode *keycode = llist_node_data(node);
      Y_WARN("Failed to resolve keycode %s (aliased to %s)", keycode->name, keycode->alias);

      if (!ykbmapResolveModifier(map, keycode->modifiers_value, NULL, true))
        Y_WARN(" (failed to resolve modifier value)");

      if (!ykbmapResolveModifier(map, keycode->modifiers_mask, NULL, true))
        Y_WARN(" (failed to resolve modifier mask)");
    }

  free_llist(pending_keycodes);
}

static void
ykbmapInsertModifiers(struct ykbMap *map, struct ykmKeymap *source)
{
  /* First pass: note the aliases, insert the explicit values */
  struct llist *pending_modifiers = new_llist();
  for (struct rbtree_node *node = rbtree_head(source->modifiers); node; node = rbtree_node_next(node))
    {
      struct ykmModifier *modifier = rbtree_node_data(node);
      uint16_t value = 0;
      bool fail = false;
      for (struct llist_node *node2 = llist_head(modifier->value); node2; node2 = llist_node_next(node2))
        {
          struct ykmModifierValue *v = llist_node_data(node2);
          if (v->name)
            {
              fail = true;
              break;
            }
          value |= v->value;
        }
      if (fail)
        llist_add_tail(pending_modifiers, modifier);
      else
        ykbmapInsertModifier(map, modifier->name, value);
    }

  /* All other passes: try to insert some values */
  while (llist_length(pending_modifiers) > 0)
    {
      uint32_t old_length = llist_length(pending_modifiers);
      struct llist_node *next;
      for (struct llist_node *node = llist_head(pending_modifiers); node; node = next)
        {
          next = llist_node_next(node);
          struct ykmModifier *modifier = llist_node_data(node);

          uint16_t value;
          if (!ykbmapResolveModifier(map, modifier->value, &value, false))
            continue;

          ykbmapInsertModifier(map, modifier->name, value);
          llist_node_delete(node);
        }

      /* Stop when we no longer have any matches */
      if (llist_length(pending_modifiers) == old_length)
        break;
    }

  for (struct llist_node *node = llist_head(pending_modifiers); node; node = llist_node_next(node))
    {
      struct ykmModifier *modifier = llist_node_data(node);
      Y_WARN("Failed to resolve modifier %s", modifier->name);
    }

  free_llist(pending_modifiers);
}

static void
ykbmapInsertAction(struct ykbMap *map, struct ykmSequenceAction *action, struct ykbMapEdge *edge)
{
  /* Discard ykbaNone here */
  if (action->type == ykbaNone)
    return;

  /* ykbaClear is never inserted - it just flushes everything
   * currently in this edge
   */
  if (action->type == ykbaClear)
    {
      llist_destroy(edge->actions, ykbActionDestroy);
      edge->actions = new_llist();
      return;
    }

  switch (action->type)
    {
    case ykbaSetModifiers:
    case ykbaMaskModifiers:
    case ykbaToggleModifiers:
    case ykbaSetStickyModifiers:
    case ykbaMaskStickyModifiers:
    case ykbaToggleStickyModifiers:
      {
        uint16_t value;
        if (!ykbmapResolveModifier(map, action->modifiers, &value, true))
          {
            Y_WARN("Failed to resolve modifier in %s argument, ignoring this action", ykbActionName(action->type));
            return;
          }
        struct ykbAction *a = ykbActionCreate();
        a->type = action->type;
        a->modifiers = action->not ? ~value : value;
        llist_add_tail(edge->actions, a);
        return;
      }

    case ykbaAbortExtended:
    case ykbaFlushKeymap:
    case ykbaRestoreState:
      {
        struct ykbAction *a = ykbActionCreate();
        a->type = action->type;
        llist_add_tail(edge->actions, a);
        return;
      }
    case ykbaBeginExtended:
    case ykbaUnsetOption:
    case ykbaAddKeymap:
    case ykbaRemoveKeymap:
    case ykbaString:
    case ykbaEvent:
      {
        struct ykbAction *a = ykbActionCreate();
        a->type = action->type;
        a->str1 = ystrdup(action->str1);
        llist_add_tail(edge->actions, a);
        return;
      }
    case ykbaSetOption:
      {
        struct ykbAction *a = ykbActionCreate();
        a->type = action->type;
        a->str1 = ystrdup(action->str1);
        a->str2 = ystrdup(action->str2);
        llist_add_tail(edge->actions, a);
        return;
      }

    case ykbaClear:
    case ykbaNone:
      abort();
    }
}

static void
ykbmapInsertSequence(struct ykbMap *map, struct ykmSequence *seq)
{
  struct ykbMapNode *map_node = map->start;
  struct ykbMapEdge *prev_edge = NULL;
  for (struct llist_node *node = llist_head(seq->members); node; node = llist_node_next(node))
    {
      struct ykmSequenceMember *member = llist_node_data(node);
      if (member->keycode)
        {
          bool direction = member->keycode->direction;
          struct ykbKeycode *keycode;
          uint16_t modifiers_value;
          uint16_t modifiers_mask;

          if (!ykbmapResolveKeycode(map, member->keycode->keycode, &keycode))
            {
              Y_WARN("Failed to resolve keycode %s (in a sequence, ignoring the rest of this sequence)", member->keycode->keycode);
              return;
            }

          if (!ykbmapResolveModifier(map, member->keycode->modifiers_value, &modifiers_value, true))
            {
              Y_WARN("Failed to resolve modifier in a sequence, ignoring the rest of this sequence");
              return;
            }

          if (!ykbmapResolveModifier(map, member->keycode->modifiers_mask, &modifiers_mask, true))
            {
              Y_WARN("Failed to resolve modifier mask in a sequence, ignoring the rest of this sequence");
              return;
            }

          /* Now apply the modifiers associated with the keycode. Note
           * that masks are combined with &, while values are combined
           * with |
           */
          modifiers_value |= keycode->modifiers_value;
          modifiers_mask &= keycode->modifiers_mask;

          if (prev_edge)
            {
              if (prev_edge->destination)
                map_node = prev_edge->destination;
              else
                {
                  struct ykbMapNode *next = ykbMapNodeAdd(map);
                  prev_edge->destination = next;
                  map_node = next;
                }
            }

          uint16_t no_mask = 0;
          no_mask = ~no_mask;

          if (modifiers_mask == no_mask)
            {
              struct ykbMapEdge *edge = ykbMapFindEdge(map_node, direction, keycode->value, modifiers_value);
              if (edge)
                prev_edge = edge;
              else
                prev_edge = ykbMapEdgeAdd(map_node, direction, keycode->value, modifiers_value);
            }
          else
            {
              struct ykbMapEdge *edge = ykbMapFindMaskEdge(map_node, direction, keycode->value, modifiers_mask, modifiers_value);
              if (edge)
                prev_edge = edge;
              else
                prev_edge = ykbMapMaskEdgeAdd(map_node, direction, keycode->value, modifiers_mask, modifiers_value)->edge;
            }
        }
      else
        {
          ykbmapInsertAction(map, member->action, prev_edge);
        }
    }
}

static void
ykbmapInsertSequences(struct ykbMap *map, struct ykmKeymap *source)
{
  for (struct llist_node *node = llist_head(source->sequences); node; node = llist_node_next(node))
    {
      struct ykmSequence *seq = llist_node_data(node);
      ykbmapInsertSequence(map, seq);
    }
}

static void
ykbmapInsertKeyNames(struct ykbMap *map, struct ykmKeymap *source)
{
  for (struct rbtree_node *node = rbtree_head(source->keycode_names); node; node = rbtree_node_next(node))
    {
      struct ykmKeycodeName *kn = rbtree_node_data(node);
      struct ykbKeycode *keycode;
      if (!ykbmapResolveKeycode(map, kn->keycode, &keycode))
        {
          Y_WARN("Failed to resolve keycode %s (trying to insert keycode name '%s', ignoring)", kn->keycode, kn->name);
          continue;
        }

      uint16_t modifiers_value;
      uint16_t modifiers_mask;

      if (!ykbmapResolveModifier(map, kn->modifiers_value, &modifiers_value, true))
        {
          Y_WARN("Failed to resolve modifier for keycode %s name '%s', ignoring", kn->keycode, kn->name);
          continue;
        }

      if (!ykbmapResolveModifier(map, kn->modifiers_mask, &modifiers_mask, true))
        {
          Y_WARN("Failed to resolve modifier mask for keycode %s name '%s', ignoring", kn->keycode, kn->name);
          continue;
        }

      ykbmapInsertKeyName(map, keycode, kn->name, modifiers_value, modifiers_mask);
    }
}

static void
ykbmapInsertModifierNames(struct ykbMap *map, struct ykmKeymap *source)
{
  for (struct llist_node *node = llist_head(source->modifier_names); node; node = llist_node_next(node))
    {
      struct ykmModifierName *mn = llist_node_data(node);

      uint16_t modifiers_value;
      uint16_t modifiers_mask;

      if (!ykbmapResolveModifier(map, mn->value, &modifiers_value, true))
        {
          Y_WARN("Failed to resolve modifier in a modifier name, ignoring");
          continue;
        }

      if (!ykbmapResolveModifier(map, mn->mask, &modifiers_mask, true))
        {
          Y_WARN("Failed to resolve modifier mask in a modifier name, ignoring");
          continue;
        }

      llist_add_tail(map->modifier_names, ykbModifierNameCreate(modifiers_value, modifiers_mask, mn->name));
    }
}

static void
ykbmapInsert(struct ykbMap *map, struct ykmKeymap *source)
{
  ykbmapInsertModifiers(map, source);
  ykbmapInsertKeycodes(map, source);
  ykbmapInsertSequences(map, source);
  ykbmapInsertKeyNames(map, source);
  ykbmapInsertModifierNames(map, source);
}

static bool
ykbmapIsActiveOption(struct ykbMapSet *mapset, struct ykbmapOption *option)
{
  /* Options with no parent are always active */
  if (!option->parent || option->parent[0] == '\0')
    return true;

  /* Otherwise an option is only active if its parent is selected */
  char *c = strstr(option->parent, "::");

  if (c)
    {
      /* The parent is an option member */
      size_t parent_option_len = c - option->parent;
      char parent_option[parent_option_len + 1];
      memcpy(parent_option, option->parent, parent_option_len);
      parent_option[parent_option_len] = '\0';

      const char *parent_keymap = c + 2;

      struct rbtree_node *node = rbtree_find(mapset->options, parent_option);
      /* If the parent option is not set at all, this option cannot be
       * active
       */
      if (!node)
        return false;

      struct ykbmapOption *ymo = rbtree_node_data(node);

      /* The parent option must be active for this option to be
       * active
       */
      if (!ykbmapIsActiveOption(mapset, ymo))
        return false;

      /* Otherwise, this option is active iff the parent option is set
       * to the parent keymap
       */
      if (strcmp(parent_keymap, ymo->keymap) == 0)
        return true;
      else
        return false;
    }
  else
    {
      /* The parent is a global keymap */
      struct rbtree_node *node = rbtree_find(mapset->selected_maps, option->parent);

      /* This option is active iff the parent map is selected */
      if (node)
        return true;
      else
        return false;
    }
}

static void
ykbmapRebuild(struct ykbMapSet *mapset)
{
  if (mapset->inhibit_rebuild)
    {
      mapset->rebuild_pending = true;
      return;
    }

  ykbMapDestroy(mapset->map);
  mapset->map = ykbMapCreate();

  struct llist *active_keymaps = new_llist();

  for (struct llist_node *node = llist_head(mapset->active_maps); node; node = llist_node_next(node))
    {
      struct ykbmapActiveKeymap *ak = llist_node_data(node);
      if (ak->keymap)
        {
          struct rbtree_node *node2 = rbtree_find(mapset->ykm->keymaps, ak->keymap->name);
          if (!node2)
            {
              Y_WARN("Undefined keymap '%s' in the active list; behaviour may be erratic", ak->keymap->name);
              continue;
            }
          struct ykmKeymap *keymap = rbtree_node_data(node2);
          assert(keymap);
          llist_add_tail(active_keymaps, keymap);
        }
      else
        {
          if (!ykbmapIsActiveOption(mapset, ak->option))
            continue;

          struct rbtree_node *node2 = rbtree_find(mapset->ykm->options, ak->option->name);
          if (!node2)
            {
              Y_WARN("Undefined option '%s' in the active list; behaviour may be erratic", ak->option->name);
              continue;
            }
          struct ykmOption *option = rbtree_node_data(node2);
          assert(option);
          node2 = rbtree_find(option->keymaps, ak->option->keymap);
          if (!node2)
            {
              Y_WARN("Undefined keymap '%s' for option '%s' in the active list; behaviour may be erratic", ak->option->keymap, ak->option->name);
              continue;
            }
          struct ykmKeymap *keymap = rbtree_node_data(node2);
          assert(keymap);
          llist_add_tail(active_keymaps, keymap);
        }
    }

  /* Now, we order the insertions for late binding, so that a keymap
   * later in the list can set a keycode used in a sequence nearer the
   * start, etc.
   */

  /* Phase 1: insert modifiers */
  for (struct llist_node *node = llist_head(active_keymaps); node; node = llist_node_next(node))
    {
      struct ykmKeymap *keymap = llist_node_data(node);
      ykbmapInsertModifiers(mapset->map, keymap);
    }

  /* Phase 2: insert keycodes */
  for (struct llist_node *node = llist_head(active_keymaps); node; node = llist_node_next(node))
    {
      struct ykmKeymap *keymap = llist_node_data(node);
      ykbmapInsertKeycodes(mapset->map, keymap);
    }

  /* Phase 3: insert everything else */
  for (struct llist_node *node = llist_head(active_keymaps); node; node = llist_node_next(node))
    {
      struct ykmKeymap *keymap = llist_node_data(node);
      ykbmapInsertSequences(mapset->map, keymap);
      ykbmapInsertKeyNames(mapset->map, keymap);
      ykbmapInsertModifierNames(mapset->map, keymap);
    }

  free_llist(active_keymaps);
  mapset->rebuild_pending = false;
}

struct ykbMapSet *
ykbmapBuild(struct ykm *ykm)
{
  struct ykbMapSet *mapset = ykbMapSetCreate();
  mapset->ykm = ykm;

  for (struct rbtree_node *node = rbtree_head(ykm->options); node; node = rbtree_node_next(node))
    {
      struct ykmOption *option = rbtree_node_data(node);
      if (option->default_keymap && option->default_keymap[0] != '\0')
        ykbmapSetOption(mapset, option->name, option->default_keymap);
    }

  return mapset;
}

bool
ykbmapFlushKeymap(struct ykbMapSet *mapset)
{
  /* Just blow it all away */
  rbtree_destroy(mapset->selected_maps, ykbmapKeymapDestroy);
  rbtree_destroy(mapset->options, ykbmapOptionDestroy);
  llist_destroy(mapset->active_maps, ykbmapActiveKeymapDestroy);
  mapset->active_maps = new_llist();
  mapset->selected_maps = new_rbtree(ykbmapKeymapKey, ykbmapKeymapCmp);
  mapset->options = new_rbtree(ykbmapOptionKey, ykbmapOptionCmp);

  ykbmapRebuild(mapset);

  return true;
}

bool
ykbmapAddKeymap(struct ykbMapSet *mapset, const char *name)
{
  /* Don't insert the same keymap twice */
  if (rbtree_find(mapset->selected_maps, name))
    return true;

  struct rbtree_node *node = rbtree_find(mapset->ykm->keymaps, name);
  if (!node)
    {
      Y_WARN("Attempted to add undefined keymap '%s'; ignoring", name);
      return false;
    }

  struct ykmKeymap *keymap = rbtree_node_data(node);
  assert(keymap);

  struct ykbmapKeymap *ymk = ykbmapKeymapCreate(name);
  llist_add_tail(mapset->active_maps, ykbmapActiveKeymapCreate(ymk));
  ymk->maps_entry = llist_tail(mapset->active_maps);
  rbtree_insert(mapset->selected_maps, ymk);

  ykbmapRebuild(mapset);

  return true;
}

bool
ykbmapRemoveKeymap(struct ykbMapSet *mapset, const char *name)
{
  struct rbtree_node *node = rbtree_find(mapset->selected_maps, name);
  if (!node)
    return true;

  struct ykbmapKeymap *ymk = rbtree_node_data(node);
  rbtree_node_delete(node);
  struct ykbmapActiveKeymap *ak = llist_node_data(ymk->maps_entry);
  llist_node_delete(ymk->maps_entry);
  ykbmapKeymapDestroy(ymk);
  ykbmapActiveKeymapDestroy(ak);

  ykbmapRebuild(mapset);

  return true;
}

bool
ykbmapSetOption(struct ykbMapSet *mapset, const char *option_name, const char *keymap_name)
{
  struct rbtree_node *node = rbtree_find(mapset->options, option_name);
  if (node)
    {
      struct ykbmapOption *ymo = rbtree_node_data(node);
      /* If this is already set, do nothing */
      if (strcmp(ymo->keymap, keymap_name) == 0)
        return true;

      /* Validate the option */
      node = rbtree_find(mapset->ykm->options, option_name);
      if (!node)
        {
          Y_WARN("Attempted to set undefined option '%s'; ignoring", option_name);
          return false;
        }
      struct ykmOption *option = rbtree_node_data(node);
      assert(option);
      node = rbtree_find(option->keymaps, keymap_name);
      if (!node)
        {
          Y_WARN("Attempted to set undefined keymap '%s' for option '%s'; ignoring", keymap_name, option_name);
          return false;
        }
      struct ykmKeymap *keymap = rbtree_node_data(node);
      assert(keymap);

      /* Take out the old one */
      struct ykbmapActiveKeymap *ak = llist_node_data(ymo->maps_entry);
      llist_node_delete(ymo->maps_entry);
      yfree(ymo->keymap);

      /* And set the new one */
      ymo->keymap = ystrdup(keymap_name);
      llist_add_tail(mapset->active_maps, ak);
      ymo->maps_entry = llist_tail(mapset->active_maps);

      ykbmapRebuild(mapset);
    }
  else
    {
      node = rbtree_find(mapset->ykm->options, option_name);
      if (!node)
        {
          Y_WARN("Attempted to set undefined option '%s'; ignoring", option_name);
          return false;
        }
      struct ykmOption *option = rbtree_node_data(node);
      assert(option);
      node = rbtree_find(option->keymaps, keymap_name);
      if (!node)
        {
          Y_WARN("Attempted to set undefined keymap '%s' for option '%s'; ignoring", keymap_name, option_name);
          return false;
        }
      struct ykmKeymap *keymap = rbtree_node_data(node);
      assert(keymap);

      struct ykbmapOption *ymo = ykbmapOptionCreate(option->name, option->parent);
      ymo->keymap = ystrdup(keymap_name);
      llist_add_tail(mapset->active_maps, ykbmapActiveKeymapCreateOption(ymo));
      ymo->maps_entry = llist_tail(mapset->active_maps);
      rbtree_insert(mapset->options, ymo);

      ykbmapRebuild(mapset);
    }
  return true;
}

bool
ykbmapUnsetOption(struct ykbMapSet *mapset, const char *option_name)
{
  struct rbtree_node *node = rbtree_find(mapset->options, option_name);
  if (!node)
    return true;

  struct ykbmapOption *ymo = rbtree_node_data(node);
  rbtree_node_delete(node);
  struct ykbmapActiveKeymap *ak = llist_node_data(ymo->maps_entry);
  llist_node_delete(ymo->maps_entry);
  ykbmapOptionDestroy(ymo);
  ykbmapActiveKeymapDestroy(ak);

  ykbmapRebuild(mapset);

  return true;
}

bool
ykbmapInhibitRebuild(struct ykbMapSet *mapset, bool state)
{
  bool old_state = mapset->inhibit_rebuild;
  mapset->inhibit_rebuild = state;
  if (!mapset->inhibit_rebuild && mapset->rebuild_pending)
    ykbmapRebuild(mapset);
  return old_state;
}

/* arch-tag: 4c4063cc-ae27-4857-96a2-6c051e5ae403
 */
