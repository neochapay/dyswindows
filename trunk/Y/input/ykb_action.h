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

#ifndef Y_INPUT_YKB_ACTION_H
#define Y_INPUT_YKB_ACTION_H

/* Some thoughts on extended input methods:
 *
 * They operate only on string and event actions. All other actions
 * are unaffected. Processing proceeds as usual, but while an extended
 * input method is active, all string and event actions are sent to
 * the extended method instead of being dispatched directly. The
 * extended input method is responsible for dispatching strings and
 * events as appropriate, and for informing YKB when extended input
 * has completed.
 *
 * You probably want to combine ykbaBeginExtended with ykbaSetKeymap
 * in most cases, since at least some key sequences will behave
 * differently while the extended input method is active.
 */

enum ykbActionType
  {
    ykbaNone,
    /* modifiers is a bitmask to | with the modifier state */
    ykbaSetModifiers,
    /* modifiers is a bitmask to & with the modifier state */
    ykbaMaskModifiers,
    /* modifiers is a bitmask to ^ with the modifier state */
    ykbaToggleModifiers,
    /* These are like the previous pair, but they change the sticky modifier state
     *
     * Effective modifier state is (regular | sticky). Otherwise there
     * is no difference.
     *
     * This exists so that, for example:
     *
     * shift-down sets the shift modifier
     * shift-up masks out the shift modifier
     * caps-lock toggles the sticky shift modifier
     *
     * And any sequence of the above three events will do the "right"
     * thing.
     */
    ykbaSetStickyModifiers,
    ykbaMaskStickyModifiers,
    ykbaToggleStickyModifiers,
    /* str is the name of an extended input method to trigger */
    ykbaBeginExtended,
    /* no arguments, abort the current extended input method (if any) */
    ykbaAbortExtended,
    /* flush the active keymap */
    ykbaFlushKeymap,
    /* map is the map to add to the active keymap */
    ykbaAddKeymap,
    /* map is the map to remove from the active keymap */
    ykbaRemoveKeymap,
    /* map is selected from option, removing any previous selection */
    ykbaSetOption,
    /* any current selection from option is removed */
    ykbaUnsetOption,
    /* str is a UTF-8 string */
    ykbaString,
    /* str is an ASCII event name */
    ykbaEvent,
    /* restores the state to the last saved point */
    ykbaRestoreState,
    /* does nothing, but flushes any other actions currently on this edge when added */
    ykbaClear
  };

enum ykbActionType ykbLookupAction(const char *name);
const char *ykbActionName(enum ykbActionType type);

extern void ykbActionInitialise(void);
extern void ykbActionFinalise(void);

#endif

/* arch-tag: 3901f1d1-4e84-4acf-a81c-69e425fb7a3e
 */
