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

#include <Y/input/ykm.h>
#include <Y/input/ykm_p.h>
#include <Y/input/ykb_action.h>

#include <Y/util/yutil.h>
#include <Y/util/llist.h>
#include <Y/util/rbtree.h>
#include <Y/util/dbuffer.h>

#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

static struct ykmModifierValue *
ykmModifierValueCreate(uint16_t value)
{
  struct ykmModifierValue *mv = ymalloc(sizeof(*mv));
  mv->name = NULL;
  mv->value = value;
  return mv;
}

static struct ykmModifierValue *
ykmModifierValueCreateAlias(const char *name)
{
  struct ykmModifierValue *mv = ymalloc(sizeof(*mv));
  mv->name = ystrdup(name);
  mv->value = 0;
  return mv;
}

static void
ykmModifierValueDestroy(struct ykmModifierValue *mv)
{
  if (!mv)
    return;
  yfree(mv->name);
  yfree(mv);
}

static struct ykmModifierValue *
ykmModifierValueDup(const struct ykmModifierValue *old_mv)
{
  if (old_mv->name)
    return ykmModifierValueCreateAlias(old_mv->name);
  else
    return ykmModifierValueCreate(old_mv->value);
}

static struct ykmSequenceAction *
ykmSequenceActionCreate(enum ykbActionType type)
{
  struct ykmSequenceAction *sa = ymalloc(sizeof(*sa));
  sa->type = type;
  sa->modifiers = NULL;
  sa->not = false;
  sa->str1 = NULL;
  sa->str2 = NULL;

  switch(type)
    {
    case ykbaSetModifiers:
    case ykbaMaskModifiers:
    case ykbaToggleModifiers:
    case ykbaSetStickyModifiers:
    case ykbaMaskStickyModifiers:
    case ykbaToggleStickyModifiers:
      sa->modifiers = new_llist();
      break;
    default:
      break;
    }

  return sa;
}

static void
ykmSequenceActionDestroy(struct ykmSequenceAction *sa)
{
  if (!sa)
    return;
  yfree(sa->str1);
  yfree(sa->str2);
  llist_destroy(sa->modifiers, ykmModifierValueDestroy);
  yfree(sa);
}

static void
ykmModifierValueDupVisitor(void *obj, void *data)
{
  struct ykmModifierValue *mv = obj;
  struct llist *list = data;
  llist_add_tail(list, ykmModifierValueDup(mv));
}

static struct ykmSequenceAction *
ykmSequenceActionDup(const struct ykmSequenceAction *old_sa)
{
  struct ykmSequenceAction *new_sa = ykmSequenceActionCreate(old_sa->type);
  new_sa->not = old_sa->not;
  new_sa->str1 = ystrdup(old_sa->str1);
  new_sa->str2 = ystrdup(old_sa->str2);
  if (old_sa->modifiers)
    {
      assert(new_sa->modifiers);
      llist_foreach(old_sa->modifiers, &ykmModifierValueDupVisitor, new_sa->modifiers);
    }
  return new_sa;
}

static struct ykmSequenceKeycode *
ykmSequenceKeycodeCreate(bool direction, const char *keycode)
{
  struct ykmSequenceKeycode *sk = ymalloc(sizeof(*sk));
  sk->direction = direction;
  sk->keycode = ystrdup(keycode);
  sk->modifiers_value = new_llist();
  sk->modifiers_mask = new_llist();
  return sk;
}

static void
ykmSequenceKeycodeDestroy(struct ykmSequenceKeycode *sk)
{
  if (!sk)
    return;
  llist_destroy(sk->modifiers_value, ykmModifierValueDestroy);
  llist_destroy(sk->modifiers_mask, ykmModifierValueDestroy);
  yfree(sk->keycode);
  yfree(sk);
}

static struct ykmSequenceKeycode *
ykmSequenceKeycodeDup(const struct ykmSequenceKeycode *old_sk)
{
  struct ykmSequenceKeycode *new_sk = ykmSequenceKeycodeCreate(old_sk->direction, old_sk->keycode);
  llist_foreach(old_sk->modifiers_value, &ykmModifierValueDupVisitor, new_sk->modifiers_value);
  llist_foreach(old_sk->modifiers_mask, &ykmModifierValueDupVisitor, new_sk->modifiers_mask);
  return new_sk;
}

static struct ykmSequenceMember *
ykmSequenceMemberCreateKeycode(struct ykmSequenceKeycode *sk)
{
  struct ykmSequenceMember *sm = ymalloc(sizeof(*sm));
  sm->keycode = sk;
  sm->action = NULL;
  return sm;
}

static struct ykmSequenceMember *
ykmSequenceMemberCreateAction(struct ykmSequenceAction *sa)
{
  struct ykmSequenceMember *sm = ymalloc(sizeof(*sm));
  sm->keycode = NULL;
  sm->action = sa;
  return sm;
}

static void
ykmSequenceMemberDestroy(struct ykmSequenceMember *sm)
{
  if (!sm)
    return;
  ykmSequenceKeycodeDestroy(sm->keycode);
  ykmSequenceActionDestroy(sm->action);
  yfree(sm);
}

static struct ykmSequenceMember *
ykmSequenceMemberDup(struct ykmSequenceMember *old_sm)
{
  if (old_sm->keycode)
    return ykmSequenceMemberCreateKeycode(old_sm->keycode);
  else
    return ykmSequenceMemberCreateAction(old_sm->action);
}

static struct ykmSequence *
ykmSequenceCreate(void)
{
  struct ykmSequence *seq = ymalloc(sizeof(*seq));
  seq->members = new_llist();
  return seq;
}

static void
ykmSequenceDestroy(struct ykmSequence *seq)
{
  if (!seq)
    return;
  llist_destroy(seq->members, ykmSequenceMemberDestroy);
  yfree(seq);
}

static void
ykmSequenceMemberDupVisitor(void *obj, void *data)
{
  struct ykmSequenceMember *mv = obj;
  struct llist *list = data;
  llist_add_tail(list, ykmSequenceMemberDup(mv));
}

static struct ykmSequence *
ykmSequenceDup(const struct ykmSequence *old_seq)
{
  struct ykmSequence *new_seq = ykmSequenceCreate();
  llist_foreach(old_seq->members, &ykmSequenceMemberDupVisitor, new_seq->members);
  return new_seq;
}

static struct ykmModifierName *
ykmModifierNameCreate(const char *name)
{
  struct ykmModifierName *mn = ymalloc(sizeof(*mn));
  mn->name = ystrdup(name);
  mn->value = new_llist();
  mn->mask = new_llist();
  return mn;
}

static void
ykmModifierNameDestroy(struct ykmModifierName *mn)
{
  if (!mn)
    return;
  llist_destroy(mn->value, ykmModifierValueDestroy);
  llist_destroy(mn->mask, ykmModifierValueDestroy);
  yfree(mn->name);
  yfree(mn);
}

static struct ykmModifierName *
ykmModifierNameDup(const struct ykmModifierName *old_mn)
{
  struct ykmModifierName *new_mn = ykmModifierNameCreate(old_mn->name);
  llist_foreach(old_mn->value, &ykmModifierValueDupVisitor, new_mn->value);
  llist_foreach(old_mn->mask, &ykmModifierValueDupVisitor, new_mn->mask);
  return new_mn;
}

static struct ykmKeycodeName *
ykmKeycodeNameCreate(const char *keycode, const char *name)
{
  struct ykmKeycodeName *kn = ymalloc(sizeof(*kn));
  kn->keycode = ystrdup(keycode);
  kn->name = ystrdup(name);
  kn->modifiers_value = new_llist();
  kn->modifiers_mask = new_llist();
  return kn;
}

static void
ykmKeycodeNameDestroy(struct ykmKeycodeName *kn)
{
  if (!kn)
    return;
  llist_destroy(kn->modifiers_value, ykmModifierValueDestroy);
  llist_destroy(kn->modifiers_mask, ykmModifierValueDestroy);
  yfree(kn->keycode);
  yfree(kn->name);
  yfree(kn);
}

static struct ykmKeycodeName *
ykmKeycodeNameDup(const struct ykmKeycodeName *old_kn)
{
  struct ykmKeycodeName *new_kn = ykmKeycodeNameCreate(old_kn->keycode, old_kn->name);
  llist_foreach(old_kn->modifiers_value, &ykmModifierValueDupVisitor, new_kn->modifiers_value);
  llist_foreach(old_kn->modifiers_mask, &ykmModifierValueDupVisitor, new_kn->modifiers_mask);
  return new_kn;
}

static int
ykmKeycodeNameKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykmKeycodeName *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykmKeycodeNameCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykmKeycodeName *obj1 = obj1_v;
  const struct ykmKeycodeName *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykmModifier *
ykmModifierCreate(const char *name)
{
  struct ykmModifier *mod = ymalloc(sizeof(*mod));
  mod->name = ystrdup(name);
  mod->value = new_llist();
  return mod;
}

static void
ykmModifierDestroy(struct ykmModifier *mod)
{
  if (!mod)
    return;
  llist_destroy(mod->value, ykmModifierValueDestroy);
  yfree(mod->name);
  yfree(mod);
}

static struct ykmModifier *
ykmModifierDup(const struct ykmModifier *old_mod)
{
  struct ykmModifier *new_mod = ykmModifierCreate(old_mod->name);
  llist_foreach(old_mod->value, &ykmModifierValueDupVisitor, new_mod->value);
  return new_mod;
}

static int
ykmModifierKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykmModifier *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykmModifierCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykmModifier *obj1 = obj1_v;
  const struct ykmModifier *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykmKeycode *
ykmKeycodeCreate(const char *name, uint16_t value)
{
  struct ykmKeycode *keycode = ymalloc(sizeof(*keycode));
  keycode->name = ystrdup(name);
  keycode->alias = NULL;
  keycode->value = value;
  keycode->modifiers_value = NULL;
  keycode->modifiers_mask = NULL;
  return keycode;
}

static struct ykmKeycode *
ykmKeycodeCreateAlias(const char *name, const char *alias)
{
  struct ykmKeycode *keycode = ymalloc(sizeof(*keycode));
  keycode->name = ystrdup(name);
  keycode->alias = ystrdup(alias);
  keycode->value = 0;
  keycode->modifiers_value = new_llist();
  keycode->modifiers_mask = new_llist();
  return keycode;
}

static void
ykmKeycodeDestroy(struct ykmKeycode *keycode)
{
  if (!keycode)
    return;
  yfree(keycode->name);
  yfree(keycode->alias);
  llist_destroy(keycode->modifiers_value, ykmModifierValueDestroy);
  llist_destroy(keycode->modifiers_mask, ykmModifierValueDestroy);
  yfree(keycode);
}

static struct ykmKeycode *
ykmKeycodeDup(const struct ykmKeycode *old_keycode)
{
  if (old_keycode->alias)
    {
      struct ykmKeycode *new_keycode = ykmKeycodeCreateAlias(old_keycode->name, old_keycode->alias);
      llist_foreach(old_keycode->modifiers_value, &ykmModifierValueDupVisitor, new_keycode->modifiers_value);
      llist_foreach(old_keycode->modifiers_mask, &ykmModifierValueDupVisitor, new_keycode->modifiers_mask);
      return new_keycode;
    }
  else
    return ykmKeycodeCreate(old_keycode->name, old_keycode->value);
}

static int
ykmKeycodeKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykmKeycode *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykmKeycodeCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykmKeycode *obj1 = obj1_v;
  const struct ykmKeycode *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykmKeymap *
ykmKeymapCreate(const char *name)
{
  struct ykmKeymap *keymap = ymalloc(sizeof(*keymap));
  keymap->name = ystrdup(name);
  keymap->refcount = 0;
  keymap->local_to_option = false;
  keymap->keycodes = new_rbtree(ykmKeycodeKey, ykmKeycodeCmp);
  keymap->modifiers = new_rbtree(ykmModifierKey, ykmModifierCmp);
  keymap->keycode_names = new_rbtree(ykmKeycodeNameKey, ykmKeycodeNameCmp);
  keymap->modifier_names = new_llist();
  keymap->sequences = new_llist();
  return keymap;
}

static void
ykmKeymapDestroy(struct ykmKeymap *keymap)
{
  if (!keymap)
    return;
  assert(keymap->refcount == 0);
  yfree(keymap->name);
  rbtree_destroy(keymap->keycodes, ykmKeycodeDestroy);
  rbtree_destroy(keymap->modifiers, ykmModifierDestroy);
  rbtree_destroy(keymap->keycode_names, ykmKeycodeNameDestroy);
  llist_destroy(keymap->modifier_names, ykmModifierNameDestroy);
  llist_destroy(keymap->sequences, ykmSequenceDestroy);
  yfree(keymap);
}

static void
ykmKeycodeDupVisitor(const void *obj, void *data)
{
  const struct ykmKeycode *keycode = obj;
  struct rbtree *tree = data;
  rbtree_insert(tree, ykmKeycodeDup(keycode));
}

static void
ykmModifierDupVisitor(const void *obj, void *data)
{
  const struct ykmModifier *modifier = obj;
  struct rbtree *tree = data;
  rbtree_insert(tree, ykmModifierDup(modifier));
}

static void
ykmKeycodeNameDupVisitor(const void *obj, void *data)
{
  const struct ykmKeycodeName *kn = obj;
  struct rbtree *tree = data;
  rbtree_insert(tree, ykmKeycodeNameDup(kn));
}

static void
ykmModifierNameDupVisitor(void *obj, void *data)
{
  const struct ykmModifierName *mn = obj;
  struct llist *list = data;
  llist_add_tail(list, ykmModifierNameDup(mn));
}

static void
ykmSequenceDupVisitor(void *obj, void *data)
{
  const struct ykmSequence *seq = obj;
  struct llist *list = data;
  llist_add_tail(list, ykmSequenceDup(seq));
}

static struct ykmKeymap *
ykmKeymapDup(const struct ykmKeymap *old_keymap)
{
  /* Note that we do *not* duplicate refcount */
  struct ykmKeymap *new_keymap = ykmKeymapCreate(old_keymap->name);
  new_keymap->local_to_option = old_keymap->local_to_option;
  rbtree_walk_const(old_keymap->keycodes, ykmKeycodeDupVisitor, new_keymap->keycodes);
  rbtree_walk_const(old_keymap->modifiers, ykmModifierDupVisitor, new_keymap->modifiers);
  rbtree_walk_const(old_keymap->keycode_names, ykmKeycodeNameDupVisitor, new_keymap->keycode_names);
  llist_foreach(old_keymap->modifier_names, ykmModifierNameDupVisitor, new_keymap->modifier_names);
  llist_foreach(old_keymap->sequences, ykmSequenceDupVisitor, new_keymap->sequences);
  return new_keymap;
}

/* This empties the keymap while retaining external references */
static void
ykmKeymapFlush(struct ykmKeymap *keymap)
{
  if (!keymap)
    return;
  rbtree_destroy(keymap->keycodes, ykmKeycodeDestroy);
  rbtree_destroy(keymap->modifiers, ykmModifierDestroy);
  rbtree_destroy(keymap->keycode_names, ykmKeycodeNameDestroy);
  llist_destroy(keymap->modifier_names, ykmModifierNameDestroy);
  llist_destroy(keymap->sequences, ykmSequenceDestroy);
  keymap->keycodes = new_rbtree(ykmKeycodeKey, ykmKeycodeCmp);
  keymap->modifiers = new_rbtree(ykmModifierKey, ykmModifierCmp);
  keymap->keycode_names = new_rbtree(ykmKeycodeNameKey, ykmKeycodeNameCmp);
  keymap->modifier_names = new_llist();
  keymap->sequences = new_llist();
}

static int
ykmKeymapKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykmKeymap *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykmKeymapCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykmKeymap *obj1 = obj1_v;
  const struct ykmKeymap *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static void
ykmKeymapAttach(struct ykmKeymap *keymap)
{
  assert(keymap);
  keymap->refcount++;
}

static void
ykmKeymapDetach(struct ykmKeymap *keymap)
{
  assert(keymap);
  assert(keymap->refcount > 0);
  keymap->refcount--;
  if (keymap->refcount == 0)
    ykmKeymapDestroy(keymap);
}

static struct ykmOption *
ykmOptionCreate(const char *name)
{
  struct ykmOption *option = ymalloc(sizeof(*option));
  option->name = ystrdup(name);
  option->parent = NULL;
  option->default_keymap = NULL;
  option->keymaps = new_rbtree(ykmKeymapKey, ykmKeymapCmp);
  return option;
}

static void
ykmOptionDestroy(struct ykmOption *option)
{
  if (!option)
    return;
  rbtree_destroy(option->keymaps, ykmKeymapDetach);
  yfree(option->parent);
  yfree(option->default_keymap);
  yfree(option->name);
  yfree(option);
}

/* Alas, there is simply no good way to do this */
struct ykmOptionKeymapDupVisitorArgs
{
  struct ykm *ykm;
  struct rbtree *keymaps;
};

static void
ykmOptionKeymapDupVisitor(const void *obj, void *data)
{
  const struct ykmKeymap *keymap = obj;
  struct ykmOptionKeymapDupVisitorArgs *args = data;
  struct ykmKeymap *new_keymap;
  if (keymap->local_to_option)
    {
      assert(keymap->refcount == 1);
      new_keymap = ykmKeymapDup(keymap);
    }
  else
    {
      /* This keymap is a reference to a global keymap. Instead of
       * duplicating it in the new option, we must maintain the
       * link
       */
      struct rbtree_node *node = rbtree_find(args->ykm->keymaps, keymap->name);
      /* If it doesn't exist, there's a logic error somewhere else -
       * we should have already duplicated it
       */
      assert(node);
      new_keymap = rbtree_node_data(node);
    }
  ykmKeymapAttach(new_keymap);
  rbtree_insert(args->keymaps, new_keymap);
}

static struct ykmOption *
ykmOptionDup(const struct ykmOption *old_option, struct ykm *new_ykm)
{
  struct ykmOption *new_option = ykmOptionCreate(old_option->name);
  struct ykmOptionKeymapDupVisitorArgs args = {.ykm = new_ykm, .keymaps = new_option->keymaps};
  rbtree_walk_const(old_option->keymaps, ykmOptionKeymapDupVisitor, &args);
  return new_option;
}

static int
ykmOptionKey(const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const struct ykmOption *obj = obj_v;
  return strcmp(key, obj->name);
}

static int
ykmOptionCmp(const void *obj1_v, const void *obj2_v)
{
  const struct ykmOption *obj1 = obj1_v;
  const struct ykmOption *obj2 = obj2_v;
  return strcmp(obj1->name, obj2->name);
}

static struct ykm *
ykmCreate(void)
{
  struct ykm *ykm = ymalloc(sizeof(*ykm));
  ykm->keymaps = new_rbtree(ykmKeymapKey, ykmKeymapCmp);
  ykm->options = new_rbtree(ykmOptionKey, ykmOptionCmp);
  return ykm;
}

void
ykmDestroy(struct ykm *ykm)
{
  if (!ykm)
    return;
  rbtree_destroy(ykm->keymaps, ykmKeymapDetach);
  rbtree_destroy(ykm->options, ykmOptionDestroy);
  yfree(ykm);
}

static void
ykmKeymapDupVisitor(const void *obj, void *data)
{
  const struct ykmKeymap *keymap = obj;
  struct rbtree *tree = data;
  struct ykmKeymap *new_keymap = ykmKeymapDup(keymap);
  rbtree_insert(tree, new_keymap);
  ykmKeymapAttach(new_keymap);
}

static void
ykmOptionDupVisitor(const void *obj, void *data)
{
  const struct ykmOption *option = obj;
  struct ykm *ykm = data;
  rbtree_insert(ykm->options, ykmOptionDup(option, ykm));
}

static struct ykm *
ykmDup(const struct ykm *old_ykm)
{
  struct ykm *new_ykm = ykmCreate();
  rbtree_walk_const(old_ykm->keymaps, ykmKeymapDupVisitor, new_ykm->keymaps);
  rbtree_walk_const(old_ykm->options, ykmOptionDupVisitor, new_ykm);
  return new_ykm;
}

static uint16_t
ykmGetShort(const char *p)
{
  uint16_t v;
  memcpy(&v, p, 2);
  return ntohs(v);
}

static bool
ykmValidIdentifier(const char *p, size_t len)
{
  if (len == 0)
    return true;

  if (!isalpha(p[0]) && p[0] != '_')
    return false;

  for (size_t i = 1; i < len; i++)
    {
      if (!isalnum(p[i]) && p[i] != '_')
        return false;
    }

  return true;
}

static bool
ykmValidString(const char *p, size_t len)
{
  if (len == 0)
    return true;

  for (size_t i = 0; i < len; i++)
    {
      if (p[i] == '\0')
        return false;
    }

  return true;
}

/* Note, these macros are not statement-like */
#define ensure_len(L)                           \
  do {                                          \
    if (l < L)                                  \
      {                                         \
        Y_WARN("Invalid YKM data: too short");  \
        return false;                           \
      }                                         \
  } while (0)

#define get_byte(B)                             \
  ensure_len(1);                                \
  uint8_t B = p[0];                             \
  p += 1;                                       \
  l -= 1

#define get_short(N)                            \
  ensure_len(2);                                \
  uint16_t N = ykmGetShort(p);                  \
  p += 2;                                       \
  l -= 2

#define get_identifier_(S, L)                           \
  get_short(L);                                         \
  ensure_len(L);                                        \
  if (!ykmValidIdentifier(p, L))                        \
    {                                                   \
      Y_WARN("Invalid YKM data: invalid identifier");   \
      return false;                                     \
    }                                                   \
  char S[L + 1];                                        \
  memcpy(S, p, L);                                      \
  S[L] = '\0';                                          \
  p += L;                                               \
  l -= L

#define get_identifier(S) get_identifier_(S, S ## _len)

#define get_string_(S, L)                                 \
  get_short(L);                                           \
  ensure_len(L);                                          \
  if (!ykmValidString(p, L))                              \
    {                                                     \
      Y_WARN("Invalid YKM data: invalid string");         \
      return false;                                       \
    }                                                     \
  char S[L + 1];                                          \
  memcpy(S, p, L);                                        \
  S[L] = '\0';                                            \
  p += L;                                                 \
  l -= L

#define get_string(S) get_string_(S, S ## _len)

static bool
ykmParseModifierMask(uint16_t count, struct llist *values, const char **data, size_t *len)
{
  const char *p = *data;
  size_t l = *len;

  for (uint16_t i = 0; i < count; i++)
    {
      get_byte(is_alias);
      if (is_alias)
        {
          get_identifier(alias_name);
          llist_add_tail(values, ykmModifierValueCreateAlias(alias_name));
        }
      else
        {
          get_short(value);
          llist_add_tail(values, ykmModifierValueCreate(value));
        }
    }

  assert(llist_length(values) == count);

  *data = p;
  *len = l;
  return true;
}

static bool
ykmUpdateKeymap(struct rbtree *keymaps, const char **data, size_t *len)
{
  const char *p = *data;
  size_t l = *len;

  get_identifier(keymap_name);
  get_byte(flush);
  get_short(keycode_count);
  get_short(modifier_count);
  get_short(keycode_name_count);
  get_short(modifier_name_count);
  get_short(sequence_count);

  struct ykmKeymap *keymap;

  {
    struct rbtree_node *node = rbtree_find(keymaps, keymap_name);
    if (node)
      {
        keymap = rbtree_node_data(node);
        if (flush)
          ykmKeymapFlush(keymap);
      }
    else
      {
        keymap = ykmKeymapCreate(keymap_name);
        rbtree_insert(keymaps, keymap);
        ykmKeymapAttach(keymap);
      }
  }

  for (uint16_t i = 0; i < keycode_count; i++)
    {
      get_byte(is_alias);
      get_identifier(keycode_name);

      struct rbtree_node *node = rbtree_find(keymap->keycodes, keycode_name);
      if (node)
        {
          struct ykmKeycode *keycode = rbtree_node_data(node);
          rbtree_node_delete(node);
          ykmKeycodeDestroy(keycode);
        }
      if (is_alias)
        {
          get_identifier(alias_name);
          get_short(modifier_value_count);
          get_short(modifier_mask_count);

          struct ykmKeycode *keycode = ykmKeycodeCreateAlias(keycode_name, alias_name);
          rbtree_insert(keymap->keycodes, keycode);

          if (!ykmParseModifierMask(modifier_value_count, keycode->modifiers_value, &p, &l))
            return false;
          if (!ykmParseModifierMask(modifier_mask_count, keycode->modifiers_mask, &p, &l))
            return false;
        }
      else
        {
          get_short(value);
          rbtree_insert(keymap->keycodes, ykmKeycodeCreate(keycode_name, value));
        }
    }

  for (uint16_t i = 0; i < modifier_count; i++)
    {
      get_identifier(modifier_name);
      get_short(value_count);

      struct rbtree_node *node = rbtree_find(keymap->modifiers, modifier_name);
      if (node)
        {
          struct ykmModifier *modifier = rbtree_node_data(node);
          rbtree_node_delete(node);
          ykmModifierDestroy(modifier);
        }
      struct ykmModifier *modifier = ykmModifierCreate(modifier_name);
      rbtree_insert(keymap->modifiers, modifier);
      if (!ykmParseModifierMask(value_count, modifier->value, &p, &l))
        return false;
    }

  for (uint16_t i = 0; i < keycode_name_count; i++)
    {
      get_identifier(keycode_name);
      get_string(description);
      get_short(value_count);
      get_short(mask_count);

      struct rbtree_node *node = rbtree_find(keymap->keycode_names, keycode_name);
      if (node)
        {
          struct ykmKeycodeName *kn = rbtree_node_data(node);
          rbtree_node_delete(node);
          ykmKeycodeNameDestroy(kn);
        }
      struct ykmKeycodeName *kn = ykmKeycodeNameCreate(keycode_name, description);
      rbtree_insert(keymap->keycode_names, kn);

      if (!ykmParseModifierMask(value_count, kn->modifiers_value, &p, &l))
        return false;
      if (!ykmParseModifierMask(mask_count, kn->modifiers_mask, &p, &l))
        return false;
    }

  for (uint16_t i = 0; i < modifier_name_count; i++)
    {
      get_string(description);
      get_short(value_count);
      get_short(mask_count);

      struct ykmModifierName *modifier_name = ykmModifierNameCreate(description);
      llist_add_tail(keymap->modifier_names, modifier_name);
      if (!ykmParseModifierMask(value_count, modifier_name->value, &p, &l))
        return false;
      if (!ykmParseModifierMask(mask_count, modifier_name->mask, &p, &l))
        return false;
    }

  for (uint16_t i = 0; i < sequence_count; i++)
    {
      get_short(member_count);

      struct ykmSequence *seq = ykmSequenceCreate();
      llist_add_tail(keymap->sequences, seq);

      for (uint16_t j = 0; j < member_count; j++)
        {
          get_byte(is_action);
          if (is_action)
            {
              get_identifier(action_name);
              enum ykbActionType action = ykbLookupAction(action_name);
              struct ykmSequenceAction *sa = ykmSequenceActionCreate(action);
              llist_add_tail(seq->members, ykmSequenceMemberCreateAction(sa));
              switch (action)
                {
                case ykbaNone:
                  Y_WARN("Invalid YKM data: unrecognised action '%s'", action_name);
                  return false;
                case ykbaSetModifiers:
                case ykbaMaskModifiers:
                case ykbaToggleModifiers:
                case ykbaSetStickyModifiers:
                case ykbaMaskStickyModifiers:
                case ykbaToggleStickyModifiers:
                  {
                    get_byte(not);
                    get_short(value_count);
                    sa->not = not;
                    if (!ykmParseModifierMask(value_count, sa->modifiers, &p, &l))
                      return false;
                    break;
                  }
                case ykbaAbortExtended:
                case ykbaFlushKeymap:
                case ykbaRestoreState:
                case ykbaClear:
                  {
                    break;
                  }
                case ykbaBeginExtended:
                case ykbaUnsetOption:
                case ykbaAddKeymap:
                case ykbaRemoveKeymap:
                  {
                    get_identifier(id);
                    sa->str1 = ystrdup(id);
                    break;
                  }
                case ykbaString:
                case ykbaEvent:
                  {
                    get_string(str);
                    sa->str1 = ystrdup(str);
                    break;
                  }
                case ykbaSetOption:
                  {
                    get_identifier(option_name);
                    get_identifier(keymap_name);
                    sa->str1 = ystrdup(option_name);
                    sa->str2 = ystrdup(keymap_name);
                    break;
                  }
                }
            }
          else
            {
              get_byte(direction);
              get_identifier(keycode);
              get_short(modifier_value_count);
              get_short(modifier_mask_count);
              struct ykmSequenceKeycode *sk = ykmSequenceKeycodeCreate(direction, keycode);
              llist_add_tail(seq->members, ykmSequenceMemberCreateKeycode(sk));

              if (!ykmParseModifierMask(modifier_value_count, sk->modifiers_value, &p, &l))
                return false;
              if (!ykmParseModifierMask(modifier_mask_count, sk->modifiers_mask, &p, &l))
                return false;
            }
        }
    }

  *data = p;
  *len = l;
  return true;
}

static bool
ykmUpdateOption(struct ykm *ykm, const char **data, size_t *len)
{
  const char *p = *data;
  size_t l = *len;

  get_string(option_name);
  get_string(parent);
  get_string(default_keymap);
  get_byte(flush);
  get_short(keymap_count);

  struct ykmOption *option;
  {
    struct rbtree_node *node = rbtree_find(ykm->options, option_name);
    if (node)
      {
        option = rbtree_node_data(node);
        if (flush)
          {
            rbtree_node_delete(node);
            ykmOptionDestroy(option);
            option = ykmOptionCreate(option_name);
            rbtree_insert(ykm->options, option);
          }
      }
    else
      {
        option = ykmOptionCreate(option_name);
        rbtree_insert(ykm->options, option);
      }
  }

  yfree(option->parent);
  option->parent = ystrdup(parent);
  yfree(option->default_keymap);
  option->default_keymap = ystrdup(default_keymap);

  for (uint16_t i = 0; i < keymap_count; i++)
    {
      get_byte(reference);
      if (reference)
        {
          get_identifier(keymap_name);

          /* Detach whatever we currently have in this slot */
          struct rbtree_node *node = rbtree_find(option->keymaps, keymap_name);
          if (node)
            {
              struct ykmKeymap *keymap = rbtree_node_data(node);
              rbtree_node_delete(node);
              ykmKeymapDetach(keymap);
            }

          node = rbtree_find(ykm->keymaps, keymap_name);
          if (node)
            {
              struct ykmKeymap *keymap = rbtree_node_data(node);
              ykmKeymapAttach(keymap);
              rbtree_insert(option->keymaps, keymap);
            }
          else
            {
              Y_WARN("YKM option %s references unknown keymap %s; ignoring", option_name, keymap_name);
            }
        }
      else
        {
          /* Hack to avoid duplicating the macro; the compiler can sort this mess out */
          const char *save_p = p;
          size_t save_l = l;
          get_identifier(keymap_name);
          p = save_p;
          l = save_l;

          struct rbtree_node *node = rbtree_find(option->keymaps, keymap_name);
          if (node)
            {
              struct ykmKeymap *keymap = rbtree_node_data(node);
              /* The YKM data says that this keymap should be local to
               * this option. If we have a reference to a global
               * keymap instead, detach it from this option;
               * ykbUpdateKeymap will create a new one.
               *
               * This protects us from updating the global keymap by
               * accident.
               */
              if (!keymap->local_to_option)
                {
                  rbtree_node_delete(node);
                  ykmKeymapDetach(keymap);
                }
            }

          if (!ykmUpdateKeymap(option->keymaps, &p, &l))
            return false;

          /* And note that this keymap is local to the option */
          node = rbtree_find(option->keymaps, keymap_name);
          assert(node);
          struct ykmKeymap *keymap = rbtree_node_data(node);
          keymap->local_to_option = true;
        }
    }

  *data = p;
  *len = l;
  return true;
}

static bool
ykmUpdate_(struct ykm *ykm, const char *data, size_t len)
{
  if (len < 9)
    {
      Y_WARN("Invalid YKM data: too short");
      return false;
    }

  if (memcmp(data, "\0YKM", 4) != 0)
    {
      Y_WARN("Invalid YKM data: invalid header signature");
      return false;
    }

  unsigned char version = data[4];
  if (version != 0)
    {
      Y_WARN("Invalid YKM data: unsupported version %ud", version);
      return false;
    }

  uint16_t keymap_count = ykmGetShort(data + 5);
  uint16_t option_count = ykmGetShort(data + 7);

  /***** End of the fixed length header *****/

  const char *p = data + 9;
  size_t l = len - 9;

  for (uint16_t i = 0; i < keymap_count; i++)
    if (!ykmUpdateKeymap(ykm->keymaps, &p, &l))
      return false;

  for (uint16_t i = 0; i < option_count; i++)
    if (!ykmUpdateOption(ykm, &p, &l))
      return false;

  return true;
}

bool
ykmUpdate(struct ykm *ykm, const char *data, size_t len)
{
  /* In order to apply the changes atomically, we duplicate and work
   * on the copy. This function is therefore rather slow. Sorry.
   */
  struct ykm *working_ykm = ykmDup(ykm);
  if (!ykmUpdate_(working_ykm, data, len))
    {
      ykmDestroy(working_ykm);
      return false;
    }

  /* Success; shuffle the new trees into place */
  rbtree_destroy(ykm->keymaps, ykmKeymapDetach);
  rbtree_destroy(ykm->options, ykmOptionDestroy);
  ykm->keymaps = working_ykm->keymaps;
  ykm->options = working_ykm->options;
  yfree(working_ykm);
  return true;
}

struct ykm *
ykmParse(const char *data, size_t len)
{
  /* No need to duplicate-and-shuffle here, so we work with ykmUpdate_ */
  struct ykm *ykm = ykmCreate();
  if (ykmUpdate_(ykm, data, len))
    return ykm;
  else
    {
      ykmDestroy(ykm);
      return NULL;
    }
}

struct ykm *
ykmLoad(const char *filename)
{
  int fd = open(filename, O_RDONLY | O_NOCTTY);

  if (fd < 0)
    {
      Y_ERROR ("Error loading keymap %s: %s", filename, strerror (errno));
      return NULL;
    }

  FILE *f = fdopen(fd, "r");

  struct dbuffer *buf = new_dbuffer();

  for (;;)  
    {
      char data[1024];
      size_t len = fread(data, 1, sizeof(data), f);

      if (ferror(f))
        {
          Y_ERROR ("Error reading keymap %s: %s", filename, strerror (errno));
          fclose(f);
          free_dbuffer(buf);
          return NULL;
        }

      if (len > 0)
        dbuffer_add(buf, data, len);

      if (feof(f))
        break;
    }

  fclose (f);

  size_t len = dbuffer_len(buf);
  char *data = ymalloc(len);
  if (dbuffer_get(buf, data, len) != len)
    abort();
  free_dbuffer(buf);

  struct ykm *ykm = ykmParse(data, len);
  yfree(data);
  return ykm;
}

/* arch-tag: 492d8532-a796-43d5-8870-f6ffb084d18f
 */
