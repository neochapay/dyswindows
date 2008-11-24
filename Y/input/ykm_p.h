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

#ifndef Y_INPUT_YKM_P_H
#define Y_INPUT_YKM_P_H

#include <inttypes.h>
#include <stdbool.h>
#include <Y/input/ykb_action.h>

struct ykm
{
  struct rbtree *keymaps;
  struct rbtree *options;
};

struct ykmOption
{
  char *name;
  char *parent;
  char *default_keymap;
  struct rbtree *keymaps;
};

struct ykmKeymap
{
  char *name;
  /* Reference-counted because they can appear both at the top level
   * and in multiple structs ykmOption
   */
  int refcount;
  /* True if this keymap is local to a ykmOption, false if it is
   * global
   */
  bool local_to_option;
  struct rbtree *keycodes;
  struct rbtree *modifiers;
  struct rbtree *keycode_names;
  struct llist *modifier_names;
  struct llist *sequences;
};

struct ykmKeycode
{
  char *name;
  /* iff alias == NULL, use value */
  char *alias;
  uint16_t value;
  /* These are only defined if alias != NULL */
  struct llist *modifiers_value;
  struct llist *modifiers_mask;
};

struct ykmModifier
{
  char *name;
  struct llist *value;
};

struct ykmKeycodeName
{
  char *keycode;
  char *name;
  struct llist *modifiers_value;
  struct llist *modifiers_mask;
};

struct ykmModifierName
{
  struct llist *value;
  struct llist *mask;
  char *name;
};

struct ykmSequence
{
  struct llist *members;
};

struct ykmSequenceMember
{
  /* Precisely one of these is NULL */
  struct ykmSequenceKeycode *keycode;
  struct ykmSequenceAction *action;
};

struct ykmSequenceKeycode
{
  bool direction;
  char *keycode;
  struct llist *modifiers_value;
  struct llist *modifiers_mask;
};

struct ykmSequenceAction
{
  enum ykbActionType type;
  struct llist *modifiers;
  bool not;
  char *str1;
  char *str2;
};

struct ykmModifierValue
{
  /* iff name == NULL, use value */
  char *name;
  uint16_t value;
};

#endif

/* arch-tag: cea4ce20-4d5e-423b-8fda-5d07b1add4d9
 */
