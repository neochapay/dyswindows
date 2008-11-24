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

#include <Y/screen/renderer.h>
#include <Y/screen/rendererclass.h>

#include <Y/util/yutil.h>
#include <Y/util/index.h>

#include <stdlib.h>
#include <string.h>

typedef struct 
{
  struct Rectangle clip;
  int translateX, translateY;
} RenderRegion;

typedef struct
{
  char *key;
  char *value;
} RendererOption;

static int
rendererOptionKeyFunction (const void *key_v, const void *obj_v)
{
  const char *key = key_v;
  const RendererOption *obj = obj_v;
  return strcmp(key, obj->key);
}

static int
rendererOptionComparisonFunction (const void *obj1_v, const void *obj2_v)
{
  const RendererOption *obj1 = obj1_v;
  const RendererOption *obj2 = obj2_v;
  return strcmp(obj1->key, obj2->key);
}

static RendererOption *
rendererOptionCreate (const char *key, const char *value)
{
  RendererOption *obj = ymalloc(sizeof(*obj));
  obj->key = ystrdup(key);
  obj->value = ystrdup(value);
  return obj;
}

static void
rendererOptionDestroy (void *obj_v)
{
  RendererOption *obj = obj_v;
  yfree(obj->key);
  yfree(obj->value);
  yfree(obj);
}

void
renderer_initialise (Renderer *self)
{
  self -> regions = new_llist ();
  self -> options = indexCreate (rendererOptionKeyFunction, rendererOptionComparisonFunction);
}

void
renderer_complete (Renderer *self)
{
  if (self != NULL)
    self -> c -> complete (self);
}

void
renderer_destroy (Renderer *self)
{
  if (self != NULL)
    {
      llist_destroy (self -> regions, yfree);
      indexDestroy (self -> options, rendererOptionDestroy);
      self -> c -> destroy (self);
    }
}

int
renderer_enter (Renderer *self, const struct Rectangle *rect,
               int dx, int dy)
{
  RenderRegion *reg = ymalloc (sizeof (RenderRegion));
  RenderRegion *prev = llist_node_data (llist_head (self -> regions));
  reg -> clip.x = rect -> x;
  reg -> clip.y = rect -> y;
  reg -> clip.w = rect -> w;
  reg -> clip.h = rect -> h;
  reg -> translateX = dx;
  reg -> translateY = dy;
  if (prev != NULL)
    {
      reg -> clip.x += prev -> translateX;
      reg -> clip.y += prev -> translateY;
      reg -> translateX += prev -> translateX;
      reg -> translateY += prev -> translateY;
      if (!rectangleIntersect (&(reg -> clip), &(reg -> clip),
                               &(prev -> clip)))
         {
           yfree (reg);
           return 0;
         }
    }
  llist_add_head (self -> regions, reg);
  return 1;
}

void
renderer_leave (Renderer *self)
{
  if (self != NULL)
    {
      if (llist_length (self -> regions) != 0)
        {
          struct llist_node *head = llist_head (self->regions);
          RenderRegion *r = llist_node_data (head);
          llist_node_delete (head);
          yfree (r);
        }
    }
}

void
renderer_render_buffer (Renderer *self, Buffer *buffer,
                      int x, int y)
{
  struct Rectangle r = { x, y, 0, 0 };
  struct Rectangle r2;
  buffer_get_size (buffer, &r.w, &r.h);
  if (self != NULL)
    {
      RenderRegion *reg = llist_node_data (llist_head (self->regions));
      if (reg != NULL)
        {
          r.x += reg->translateX;
          r.y += reg->translateY;
          if (!rectangleIntersect (&r2, &r, &(reg -> clip)))
            return;
        }
      if (self->c->render_buffer != NULL)
        self->c->render_buffer (self, buffer, r.x, r.y,
               r2.x-r.x, r2.y-r.y, r2.w, r2.h);
    }
}

void
renderer_copy_buffer (Renderer *self, Buffer *buffer,
                        int x, int y)
{
  struct Rectangle r = { x, y, 0, 0 };
  struct Rectangle r2;
  buffer_get_size (buffer, &r.w, &r.h);
  if (self != NULL)
  {
    RenderRegion *reg = llist_node_data (llist_head (self->regions));
    if (reg != NULL)
    {
      r.x += reg->translateX;
      r.y += reg->translateY;
      if (!rectangleIntersect (&r2, &r, &(reg -> clip)))
        return;
    }
    if (self->c->render_buffer != NULL)
      self->c->copy_buffer (self, buffer, r.x, r.y,
                              r2.x-r.x, r2.y-r.y, r2.w, r2.h);
  }
}

void
renderer_draw_filled_rectangle (Renderer *self, uint32_t colour,
                             int x, int y, int w, int h)
{
  struct Rectangle r = { x, y, w, h };
  if (self != NULL)
    {
      RenderRegion *reg = llist_node_data (llist_head (self->regions));
      if (reg != NULL)
        {
          r.x += reg -> translateX;
          r.y += reg -> translateY;
          rectangleIntersect (&r, &r, &(reg -> clip));
        }
      self -> c -> draw_filled_rectangle (self, colour, r.x, r.y, r.w, r.h);
    }
}

void
renderer_set_option (Renderer *self, const char *key, const char *value)
{
  if (!key)
    return;

  void *obj = indexFind(self -> options, key);

  if (obj)
    rendererOptionDestroy(obj);

  if (!value)
    return;

  obj = rendererOptionCreate(key, value);

  indexAdd(self -> options, obj);
}

const char *
renderer_get_option (Renderer *self, const char *key)
{
  RendererOption *obj = indexFind(self -> options, key);

  if (!obj)
    return NULL;

  return obj->value;
}

