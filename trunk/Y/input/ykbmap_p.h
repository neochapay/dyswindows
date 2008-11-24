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

#ifndef Y_INPUT_YKBMAP_P_H
#define Y_INPUT_YKBMAP_P_H

#include <inttypes.h>
#include <Y/util/llist.h>
#include <Y/util/rbtree.h>
#include <Y/input/ykb_action.h>
#include <stdbool.h>

/* A keymap is effectively a DFA, where an edge is a (keycode, stroke
 * direction, modifier set) tuple and a list of ykbAction. When the
 * DFA transitions along an edge, its action list is executed.
 */

struct ykbAction
{
  enum ykbActionType type;
  /* Only some of these fields are valid, depending on the value of type */
  char *str1;
  char *str2;
  uint16_t modifiers;
};

/* These next few structures represent the DFA itself */

struct ykbMapNode;

/* A mask edge is considered to match x if all of these are true:
 *   x->direction         == edge->direction
 *   x->keycode           == edge->keycode
 *  (x->modifiers & mask) == edge->modifiers
 */
struct ykbMapMaskEdge
{
  uint16_t mask;
  struct ykbMapEdge *edge;
};

struct ykbMapMaskEdgeList
{
  bool direction;
  uint16_t keycode;
  /* List of ykbMapMaskEdge */
  struct llist *mask_edges;
};

struct ykbMapEdge
{
  /* true for down, false for up */
  bool direction;
  uint16_t keycode;
  uint16_t modifiers;
  /* NULL means return to the start node */
  struct ykbMapNode *destination;
  struct llist *actions;
};

struct ykbMapNode
{
  /* rbtree of ykbMapEdge */
  struct rbtree *edges;
  /* rbtree of ykbMapMaskEdgeList */
  struct rbtree *mask_edge_lists;
};

extern struct ykbMapEdge *ykbMapLookup(struct ykbMapNode *node, bool direction, uint16_t keycode, uint16_t modifiers);

struct ykbKeycode
{
  char *name;
  uint16_t value;
  uint16_t modifiers_value;
  uint16_t modifiers_mask;
};

struct ykbModifier
{
  char *name;
  uint16_t value;
};

struct ykbKeyName
{
  uint16_t keycode;
  uint16_t modifiers_value;
  uint16_t modifiers_mask;
  char *str;
};

struct ykbKeyNameList
{
  uint16_t keycode;
  struct llist *list;
};

struct ykbModifierName
{
  uint16_t modifiers;
  uint16_t mask;
  char *str;
};

struct ykbMap
{
  struct ykbMapNode *start;
  struct llist *nodes;

  struct rbtree *keycodes;
  struct rbtree *modifiers;

  struct rbtree *keycode_names;
  struct llist *modifier_names;
};

extern struct ykbKeyNameList *ykbKeyLookup(struct ykbMap *map, uint16_t keycode);

struct ykbmapKeymap
{
  char *name;
  struct llist_node *maps_entry;
};

struct ykbmapOption
{
  char *name;
  char *parent;
  char *keymap;
  struct llist_node *maps_entry;
};

struct ykbmapActiveKeymap
{
  struct ykbmapKeymap *keymap;
  struct ykbmapOption *option;
};

struct ykbMapSet
{
  /* The active keymap */
  struct ykbMap *map;

  /* true if rebuilds should be postponed */
  bool inhibit_rebuild;
  /* true if a rebuild should be performed as soon as they are not
   * inhibited
   */
  bool rebuild_pending;

  struct ykm *ykm;

  /* list of ykbmapActiveKeymap, in order of application */
  struct llist *active_maps;

  /* The next two structures are indexes, to query and remove entries
   * from active_maps efficiently
   */

  /* rbtree of ykbmapKeymap */
  struct rbtree *selected_maps;
  /* rbtree of ykbmapOption */
  struct rbtree *options;
};

#endif /* header guard */

/* arch-tag: 2e7f8d17-4c41-448b-8ec1-369f21bac22b
 */
