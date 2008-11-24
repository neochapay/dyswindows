/************************************************************************
 *   Copyright (C) Mark Thomas <markbt@efaref.net>
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
#include <Y/const.h>
#include <Y/modules/module_interface.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <Y/main/control.h>
#include <Y/input/pointer.h>
#include <Y/input/ykb.h>
#include <Y/util/yutil.h>

#define  EVENT_FILE_BASE "/dev/input/event"

struct EvDevInputDriverData
{
  struct input_event cur;
  ssize_t bytes_read;
  struct Module *module;
  int fds[16];
};

static void
evdevDespatch (struct EvDevInputDriverData *data)
{
  switch (data->cur.type)
    {
      case EV_REL:
        switch (data->cur.code)
          {
             case REL_X:  pointerMovePosition (data->cur.value, 0); break;
             case REL_Y:  pointerMovePosition (0, data->cur.value); break;
             default:     ;
          }
        break;
      case EV_KEY:
        switch (data->cur.code)
          {
             case BTN_LEFT:    pointerButtonChange (0, data->cur.value); break;
             case BTN_MIDDLE:  pointerButtonChange (1, data->cur.value); break;
             case BTN_RIGHT:   pointerButtonChange (2, data->cur.value); break;
             case BTN_SIDE:    pointerButtonChange (3, data->cur.value); break;
             case BTN_EXTRA:   pointerButtonChange (4, data->cur.value); break;
             case BTN_FORWARD: pointerButtonChange (5, data->cur.value); break;
             case BTN_BACK:    pointerButtonChange (6, data->cur.value); break;
             default:
               if (data->cur.value == 0)
                 ykbKeyUp(data->cur.code);
               else
                 ykbKeyDown(data->cur.code);
               break;
          }
        break;
      default: ;
    }
}

static void
evdevDataReady (int fd, int causeMask, void *data_v)
{
  struct EvDevInputDriverData *data = data_v;
  ssize_t r;

  r = read (fd, &(data -> cur) + data -> bytes_read, sizeof(struct input_event));
  if (r < 0)
    {
      Y_ERROR ("EvDev: Error Reading: %s", strerror (errno));
      return;
    }

  data -> bytes_read += r;
  if (data -> bytes_read == sizeof(struct input_event))
    {
      evdevDespatch (data);
      data -> bytes_read = 0;
    } 
}

int
initialise (struct Module *module, const struct Tuple *args)
{
  int fd;
  struct EvDevInputDriverData *data;
  char buffer[1024];
  int i;

  static char moduleName[] = "Linux Event Device Input";
  module -> name = moduleName;

  data = ymalloc (sizeof (struct EvDevInputDriverData));
  module -> data = data;
  data -> bytes_read = 0;
  data -> module = module;

  for(i=0; i<16; ++i)
    {
      sprintf (buffer, "%s%d", EVENT_FILE_BASE, i);
      fd = open (buffer, O_RDONLY|O_NONBLOCK|O_NOCTTY);
      data -> fds[i] = fd;
      if (fd > 0)
        {
           controlRegisterFileDescriptor (fd, CONTROL_WATCH_READ,
                                          data, evdevDataReady);
        }
    }

  return 0;
}


int
finalise (struct Module *self)
{
  int i;
  struct EvDevInputDriverData *data = self -> data;
  for (i=0; i<16; ++i)
    if (data -> fds[i] > 0)
      {
        controlUnregisterFileDescriptor (data -> fds[i]);
        close (data -> fds[i]);
      }
  yfree (self -> data);
  return 0;
}

/* arch-tag: 185781ee-a23d-48d3-9c94-ad81950f15d8
 */
