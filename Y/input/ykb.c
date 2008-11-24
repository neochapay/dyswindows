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

#include <Y/y.h>
#include <Y/main/control.h>
#include <Y/widget/widget.h>
#include <Y/input/ykb.h>
#include <Y/input/ykbmap.h>
#include <Y/input/ykbmap_p.h>
#include <Y/input/ykb_action.h>
#include <Y/input/ykm.h>
#include <Y/util/yutil.h>

#include <stddef.h>
#include <assert.h>
#include <stdio.h>

static struct ykbMapSet *mapset = NULL;
static struct ykbMapNode *state = NULL;
static struct ykbMapNode *saved_state = NULL;
static struct Widget *focus = NULL;
static uint16_t modifiers = 0;
static uint16_t sticky_modifiers = 0;

/* Values in msec */
static uint32_t repeat_delay = 500;
static uint32_t repeat_rate = 50;

static int repeat_timer_id = 0;
static uint16_t repeat_keycode = 0;

static struct ykbMapSet *config_mapset = NULL;
static struct ykm *config_ykm = NULL;

void
ykbSetFocus (struct Widget *w)
{
  focus = w;
}

void
ykbUnsetFocus (struct Widget *w)
{
  if (focus == w)
    focus = NULL;
}

static void
ykbDispatchString(const char *str)
{
  if (!focus)
    return;
  widget_ykb_string(focus, str, modifiers | sticky_modifiers);
}

static void
ykbDispatchEvent(const char *event)
{
  if (!focus)
    return;
  widget_ykb_event(focus, event, modifiers | sticky_modifiers);
}

static void
ykbDispatchStroke(bool direction, uint16_t keycode)
{
  if (!focus)
    return;
  widget_ykb_stroke(focus, direction, keycode, modifiers | sticky_modifiers);
}

static void
ykbSetState(struct ykbMapNode *to)
{
  if (to)
    state = to;
  else
    {
      if (mapset)
        state = mapset->map->start;
      else
        state = NULL;
    }
}

static void
ykbAct(struct ykbAction *action)
{
  switch (action->type)
    {
    case ykbaSetModifiers:
      modifiers |= action->modifiers;
      break;
    case ykbaMaskModifiers:
      modifiers &= action->modifiers;
      break;
    case ykbaToggleModifiers:
      modifiers ^= action->modifiers;
      break;
    case ykbaSetStickyModifiers:
      sticky_modifiers |= action->modifiers;
      break;
    case ykbaMaskStickyModifiers:
      sticky_modifiers &= action->modifiers;
      break;
    case ykbaToggleStickyModifiers:
      sticky_modifiers ^= action->modifiers;
      break;
    case ykbaBeginExtended:
      break;
    case ykbaAbortExtended:
      break;
    case ykbaFlushKeymap:
      ykbmapFlushKeymap(mapset);
      /* Setting state to NULL forces us to go back to the start once
       * we've finished running actions - this is because we're about
       * to rebuild the keymap, so we can't keep any pointers into
       * it
       */
      state = NULL;
      break;
    case ykbaAddKeymap:
      ykbmapAddKeymap(mapset, action->str1);
      state = NULL;
      break;
    case ykbaRemoveKeymap:
      ykbmapRemoveKeymap(mapset, action->str1);
      state = NULL;
      break;
    case ykbaSetOption:
      ykbmapSetOption(mapset, action->str1, action->str2);
      state = NULL;
      break;
    case ykbaUnsetOption:
      ykbmapUnsetOption(mapset, action->str1);
      state = NULL;
      break;
    case ykbaClear:
      break;
    case ykbaString:
      ykbDispatchString(action->str1);
      break;
    case ykbaEvent:
      ykbDispatchEvent(action->str1);
      break;
    case ykbaRestoreState:
      ykbSetState(saved_state);
      saved_state = NULL;
      break;
    case ykbaNone:
      break;
    }
}

static void
ykbTransition(struct ykbMapEdge *edge)
{
  if (!edge)
    return;

  /* Inhibit rebuilds while we're walking the action list, so that it
   * isn't deallocated from under us. This means that keymap-modifying
   * actions don't take effect until the end of the list.
   */
  bool old_rebuild_state = ykbmapInhibitRebuild(mapset, true);

  ykbSetState(edge->destination);
  for (struct llist_node *n = llist_head (edge->actions);
       n != NULL;
       n = llist_node_next (n))
    {
      struct ykbAction *action = llist_node_data (n);
      ykbAct(action);
    }

  ykbmapInhibitRebuild(mapset, old_rebuild_state);

  /* If we cleared our state, return to the start now that we've
   * rebuilt
   */
  if (!state)
    ykbSetState(NULL);
}

static void
ykbKeyStroke(bool direction, uint16_t keycode)
{
  ykbDispatchStroke(direction, keycode);

  if (!state)
    return;

  assert(mapset);

  struct ykbMapEdge *edge = ykbMapLookup(state, direction, keycode, modifiers | sticky_modifiers);
  if (!edge)
    {
      /* This keystroke doesn't form a valid sequence */

      /* If we're at the start, discard this keystroke */
      if (state == mapset->map->start)
        {
          saved_state = NULL;
          return;
        }

      /* Otherwise, we'll try again from the start. We save our state
       * at this point, so that the sequence we are attempting can
       * maybe restore it. Then we go back to the start and try
       * again
       */
      saved_state = state;
      ykbSetState(NULL);
      edge = ykbMapLookup(state, direction, keycode, modifiers | sticky_modifiers);

      /* If there's nothing at the start either, discard it */
      if (!edge)
        {
          saved_state = NULL;
          return;
        }
    }

  ykbTransition(edge);

  /* If we transitioned back to the start, we don't want to
   * accidentally restore to an old state again, so clear it - this
   * should give sane behaviour in most corner cases
   */
  if (state == mapset->map->start)
    saved_state = NULL;
}

static void
ykbRepeat (void *data)
{
  ykbKeyStroke(true, repeat_keycode);

  repeat_timer_id = controlTimerDelay(0, repeat_rate, NULL, &ykbRepeat);
}

void
ykbKeyDown (uint16_t keycode)
{
  ykbKeyStroke(true, keycode);

  repeat_keycode = keycode;
  if (repeat_timer_id != 0)
    controlCancelTimerDelay(repeat_timer_id);
  repeat_timer_id = controlTimerDelay(0, repeat_delay, NULL, &ykbRepeat);
}

void
ykbKeyUp (uint16_t keycode)
{
  ykbKeyStroke(false, keycode);

  if (repeat_timer_id != 0 && repeat_keycode == keycode)
    {
      controlCancelTimerDelay(repeat_timer_id);
      repeat_timer_id = 0;
      repeat_keycode = 0;
    }
}

static void
ykbUseMapSet(struct ykbMapSet *mapset_)
{
  mapset = mapset_;
  ykbSetState(NULL);
}

static inline unsigned int
sidesum16(uint16_t v)
{
  /* Freed, Edwin E. 1983. "Binary Magic Numbers," Dr. Dobb's Journal
   * Vol. 78 (April), pp. 24-37.
   *
   * Don't ask.
   */
  unsigned int c = 0;
  static const int S[] = {1, 2, 4, 8};
  static const int B[] = {0x5555, 0x3333, 0x0F0F, 0x00FF};

  c = v;
  c = ((c >> S[0]) & B[0]) + (c & B[0]);
  c = ((c >> S[1]) & B[1]) + (c & B[1]);
  c = ((c >> S[2]) & B[2]) + (c & B[2]);
  c = ((c >> S[3]) & B[3]) + (c & B[3]);
  return c;
}

/* Note that this function allocates a new string; the caller must
 * free it
 */
char *
ykbDescribeKey(uint16_t keycode, uint16_t modifiers)
{
  if (!mapset)
    return NULL;

  struct ykbKeyNameList *knl = ykbKeyLookup(mapset->map, keycode);
  if (!knl)
    return NULL;

  struct ykbKeyName *key_name = NULL;
  {
    unsigned int modifiers_sidesum = 0;
    unsigned int mask_sidesum = 0;
    for (struct llist_node *node = llist_head(knl->list); node; node = llist_node_next(node))
      {
        struct ykbKeyName *kn = llist_node_data(node);

        /* This will prefer:
         *
         * shift over nothing (by modifier sidesum)
         * shift over shift/shift|altgr (by mask sidesum)
         */

        /* We're not interested in modifiers with a smaller modifier sidesum... */
        if (sidesum16(kn->modifiers_value) < modifiers_sidesum)
          continue;
        /* ...or with a smaller mask sidesum */
        if (sidesum16(kn->modifiers_mask) < mask_sidesum)
          continue;

        if ((modifiers & kn->modifiers_mask) == kn->modifiers_value)
          {
            key_name = kn;
            modifiers_sidesum = sidesum16(kn->modifiers_value);
            mask_sidesum = sidesum16(kn->modifiers_mask);
          }
      }
  }

  if (!key_name)
    return NULL;

  /* Null or zero-length string means "explicitly no name" */
  if (!key_name->str || key_name->str[0] == '\0')
    return NULL;

  /* Don't emit duplicate modifier descriptions for anything we
   * matched above
   */
  uint16_t remaining_modifiers = modifiers & ~key_name->modifiers_value;

  /* Length of the description string we're going to create */
  size_t len = strlen(key_name->str);

  struct llist *mods = new_llist();

  for (struct llist_node *n = llist_head(mapset->map->modifier_names); n; n = llist_node_next(n))
    {
      struct ykbModifierName *mod_name = llist_node_data(n);
      if ((remaining_modifiers & mod_name->mask) == mod_name->modifiers)
        {
          len += strlen(mod_name->str) + 1;
          llist_add_tail(mods, mod_name);
        }
    }

  char *str = ymalloc(len + 1);
  char *c = str;

  for (struct llist_node *n = llist_head(mods); n; n = llist_node_next(n))
    {
      struct ykbModifierName *mod_name = llist_node_data(n);
      size_t l = strlen(mod_name->str);
      memcpy(c, mod_name->str, l);
      c += l;
      *c++ = '-';
    }

  size_t l = strlen(key_name->str);
  memcpy(c, key_name->str, l);
  c += l;
  *c = '\0';
  assert(c == (str + len));

  return str;
}

void
ykbInitialise (struct Config *serverConfig)
{
  ykbActionInitialise();

  struct TupleType layoutType = {.count = 1, .list = (enum Type []) {t_list}};
  struct Tuple *layoutTuple = configGet(serverConfig, "keymap", "layout", &layoutType);

  if (!layoutTuple)
    {
      Y_WARN("No keymap:layout specified in config file; not loading a keymap");
      return;
    }

  if (layoutTuple->count == 0)
    {
      Y_WARN("No keymap:layout arguments specified in config file; not loading a keymap");
      tupleDestroy(layoutTuple);
      return;
    }

  assert(layoutTuple->count >= 1);
  if (layoutTuple->error)
    {
      Y_WARN("Error retrieving keymap:layout from config file: %s", layoutTuple->list[0].string.data);
      tupleDestroy(layoutTuple);
      return;
    }

  if (layoutTuple->list[0].type != t_string)
    {
      Y_WARN("Unrecognised argument for keymap:layout; ignoring this entry");
      tupleDestroy(layoutTuple);
      return;
    }

  const char *layout = layoutTuple->list[0].string.data;
  static const char keymapfmt[] = "%s/keymaps/%s.ykm";
  char path[strlen(keymapfmt) + strlen(yConfigDir) + strlen(layout) + 1];
  snprintf(path, sizeof(path), keymapfmt, yConfigDir, layout);

  config_ykm = ykmLoad(path);
  if (config_ykm)
    config_mapset = ykbmapBuild(config_ykm);
  if (config_mapset && !ykbmapAddKeymap(config_mapset, layout))
    Y_WARN("Error selecting keymap %s", layout);
  if (config_mapset)
    ykbUseMapSet(config_mapset);

  for (uint32_t i = 1; i < layoutTuple->count; i++)
    {
      if (layoutTuple->list[0].type != t_string)
        {
          Y_WARN("Unrecognised argument %d for keymap:layout; ignoring this entry", i);
          tupleDestroy(layoutTuple);
          return;
        }

      const char *arg = layoutTuple->list[i].string.data;

      const char *eq;
      if ((eq = strchr(arg, '=')))
        {
          size_t option_len = eq - arg;
          char option[option_len + 1];
          memcpy(option, arg, option_len);
          option[option_len] = '\0';
          const char *keymap = eq + 1;
          if (!ykbmapSetOption(config_mapset, option, keymap))
            Y_WARN("Error setting keymap option %s to %s", option, keymap);
        }
      else
        {
          if (!ykbmapAddKeymap(config_mapset, arg))
            Y_WARN("Error adding keymap %s", arg);
        }
    }

  tupleDestroy(layoutTuple);
}

void
ykbFinalise(void)
{
  ykbUseMapSet(NULL);
  ykbMapSetDestroy(config_mapset);
  ykmDestroy(config_ykm);
  ykbActionFinalise();
}

/* arch-tag: 3f5d46aa-f7bf-4111-b217-9a9cdcfed7c4
 */
