/************************************************************************
 *   Copyright (C) Dustin Norlander <dustin@dustismo.com>
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

/*
 * This class creates a grouping for individual radiobuttons.
 * Only one radiobutton may be selected at any one time.
 * 
 * 
 */
 
#include <stdlib.h>
#include <Y/object/class.h>
#include <Y/widget/yradiogroup.h>
#include <Y/util/yutil.h>


/* static prototypes */
static struct YRadioGroup *castBack (struct Object *);
static struct llist_node *groupFindNode (struct YRadioGroup *, struct YRadioButton *);

DEFINE_CLASS(YRadioGroup);
#include "YRadioGroup.yc"

/* SUPER
 * Object
 */

/* PROPERTY
 * 
 */
 
/*
 * Creates a new radiogroup, including allocating memory
 */
struct YRadioGroup *
yradiogroupCreate (void)
{
  struct YRadioGroup *self = ymalloc (sizeof (struct YRadioGroup));
  self->list = new_llist();
  self->selected = NULL;
  self->numElements = 0;
  
  objectInitialise (&(self -> o), CLASS(YRadioGroup));
  return self;
}
 
/* METHOD
 * YRadioGroup :: () -> (object)
 */
static struct Object *
yradiogroupInstantiate (void)
{
  return yradiogroup_to_object (yradiogroupCreate ());
}
  
/* METHOD
 * DESTROY :: () -> ()
 */
void
yradiogroupDestroy (struct YRadioGroup *self)
{
  free_llist(self->list);
  objectFinalise (yradiogroup_to_object (self));
  yfree (self);
}

struct Object *
yradiogroup_to_object (struct YRadioGroup *self)
{
  return &(self -> o);
}

static struct YRadioGroup *
castBack (struct Object *object)
{
  /* assert ( object -> c == widgetClass ); */
  return (struct YRadioGroup *)object;
}

struct YRadioGroup *
objectToYRadioGroup (struct Object *object)
{
  return castBack(object);
}

/*
 * Sets the selected radiobutton.
 * This function should make sure that one-and-only-one.
 * radiobutton is currently selected. 
 *
 * This function doesn't actually set the selected button's "pressed" property.
 * Though it does set the previously selected button to pressed = false..
 *
 * -DN
 */
void
groupSetSelectedRadioButton (struct YRadioGroup *self, struct YRadioButton *rad)
{
  if (!self)
    return;
  
  /* Now unset the previously selected radiobutton */
  if (self->selected)
    yradiobuttonSetPropertyPressed (self->selected, 0);
  
  self->selected = rad;

  /*
   * Now we emit a signal which contains the newly selected radiobutton.
   *
   * Since objectEmitSignal takes all its arguments as a tuple, we must 
   * construct a tuple of the selected button.
   * -DN
   */
  struct Tuple *selObj = tupleBuild(tb_object(yradiobutton_to_object(self->selected)));
  objectEmitSignal_ (yradiogroup_to_object (self), "change_selected", selObj); //note the objectEmitSignal_ <-underscore 
}

/*
 * It simply returns the current selected radiobutton, else NULL
 * -DN
 */
 
/* METHOD
 * getSelected :: () -> (object)
 */
static struct Object *
groupGetSelected (struct YRadioGroup *self)
{
  if (!self)
    return NULL;
  return yradiobutton_to_object (self->selected);
}

/*
 * Adds the radiobutton to this radiogroup
 * returns TRUE if successful, FALSE otherwise
 */
bool
groupAddRadioButton (struct YRadioGroup *self, struct YRadioButton *rad)
{
  if (!self || !rad)
    return FALSE;
    
  llist_add_head(self->list, rad);
  self->numElements++;
  
  rad->group = self;
  
  /*
   * We allow user to add a selected button if-and-only-if there is currently
   * nothing selected.
   */
  if (!self->selected && yradiobuttonGetPropertyPressed(rad))
    self->selected = rad;
  else
    yradiobuttonSetPropertyPressed (rad, 0);  //set pressed to false
  return TRUE;
}

/*
 * Finds the list node containing the given 
 * YRadioButton.  If the node is not found
 * NULL is returned.
 */
static struct llist_node *
groupFindNode (struct YRadioGroup *self, struct YRadioButton *rad)
{
  if (!self || !rad)
    return NULL;
  
  return llist_find_data(self->list, rad);
}

/*
 * Removes the given RadioButton from this group.
 *
 */
void 
groupRemoveRadioButton (struct YRadioGroup *self, struct YRadioButton *rad)
{
  if (!self || !rad)
    return;
    
  struct llist_node *node = groupFindNode (self, rad);
  if (node)
    self->numElements--;
  llist_node_delete(node);
  
  rad->group = NULL;
  
  /* Remember to NULL the selected radiobutton if that one is removed! */
  if (self->selected == rad)
    self->selected = NULL;
  
}
