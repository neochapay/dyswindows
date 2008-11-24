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

#include <Y/input/ykb_action.h>
#include <Y/util/rbtree.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

struct rbtree *action_name_index = NULL;

struct ykbActionName
{
  const char *name;
  enum ykbActionType type;
};

static int
ykbActionNameKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykbActionName *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykbActionNameCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykbActionName *obj1 = obj1_v;
  const struct ykbActionName *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

#define action(A, N) [A] = {.name = #N, .type = A}
static struct ykbActionName actions[] =
  {
    action(ykbaNone, none),
    action(ykbaSetModifiers, setModifiers),
    action(ykbaMaskModifiers, maskModifiers),
    action(ykbaToggleModifiers, toggleModifiers),
    action(ykbaSetStickyModifiers, setStickyModifiers),
    action(ykbaMaskStickyModifiers, maskStickyModifiers),
    action(ykbaToggleStickyModifiers, toggleStickyModifiers),
    action(ykbaBeginExtended, beginExtended),
    action(ykbaAbortExtended, abortExtended),
    action(ykbaFlushKeymap, flushKeymap),
    action(ykbaAddKeymap, addKeymap),
    action(ykbaRemoveKeymap, removeKeymap),
    action(ykbaSetOption, setOption),
    action(ykbaUnsetOption, unsetOption),
    action(ykbaString, string),
    action(ykbaEvent, event),
    action(ykbaRestoreState, restoreState),
    action(ykbaClear, clear)
  };
#undef action

enum ykbActionType
ykbLookupAction(const char *name)
{
  struct rbtree_node *node = rbtree_find(action_name_index, name);
  if (!node)
    return ykbaNone;
  struct ykbActionName *an = rbtree_node_data(node);
  return an->type;
}

const char *
ykbActionName(enum ykbActionType type)
{
  return actions[type].name;
}

void
ykbActionInitialise(void)
{
  action_name_index = new_rbtree(ykbActionNameKey, ykbActionNameCmp);
  for (int i = ykbaNone; i <= ykbaClear; i++)
    {
      assert(actions[i].name);
      rbtree_insert(action_name_index, &actions[i]);
    }
}

void
ykbActionFinalise(void)
{
  free_rbtree(action_name_index);
}

/* arch-tag: 9be1ba32-d961-4552-a392-ac55c02ee679
 */
