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

#include <Y/trace/trace.h>
#include <Y/util/yutil.h>
#include <Y/util/log.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

const uint32_t MAGIC_1 = 0x1ea7beefU;
const uint32_t MAGIC_1_DEAD = 0xdeadf001U;
const unsigned char MAGIC_2[] = { '\x15', '\xb1', '\x0a', '\x7d' };  /* big-endian 0x15B10a7d */
const unsigned char MAGIC_2_DEAD[] = { '\xCA', '\xFF', '\xE1', '\x4E' }; /* big-endian 0xCAFFE14E - thanks Dave :-) */

union ymemarea
{
  struct {
    size_t size;
    char padding[12 - sizeof(size_t)];
    uint32_t magic;
  } data;
  char padding[16];
};

void *
ymalloc (size_t size_wanted)
{
  void *buffer;
  union ymemarea *yma;
  unsigned char *magic2;
  trace ("entry",trace_var(size_wanted, trace_size));
  size_wanted += sizeof *yma + 4;
  trace ("calling malloc",trace_var(size_wanted, trace_size));
  buffer = malloc (size_wanted);
  trace ("malloc returns",trace_var(buffer, trace_ptr));
  if (!buffer)
    {
      Y_FATAL ("Y: out of memory\n");
      abort ();
    }
  trace ("adding magic start structure");
  yma = (union ymemarea *)buffer;
  yma->data.size = size_wanted - sizeof *yma - 4;
  yma->data.magic = MAGIC_1;
  trace ("added magic start structure",
      trace_var(yma->data.size,trace_size),
      trace_var(yma->data.magic,trace_uint32),
      trace_buf(yma->padding,16));
  magic2 = buffer;
  magic2 += size_wanted - 4;
  trace ("adding magic end number");
  magic2[0] = MAGIC_2[0];
  magic2[1] = MAGIC_2[1];
  magic2[2] = MAGIC_2[2];
  magic2[3] = MAGIC_2[3];
  trace ("added magic end number",
      trace_buf(magic2,4));
  ++yma;
  buffer = (void *)yma;
  trace ("exit",trace_var(buffer, trace_ptr));
  return buffer;  
}

/*
 * Right now this is just a wrapper around ymalloc,
 * I'm putting it in so as to not lose sight of where calloc
 * should be used instead of malloc. 
 * TODO: study ymalloc and design ycalloc better.
 * -DN
 */
void *
ycalloc (size_t n, size_t el_size)
{
  return ymalloc (el_size * n);
}

void
yfree (void *buffer)
{
  union ymemarea *yma;
  unsigned char *magic2;
  trace ("entry",trace_var(buffer, trace_ptr));
  if (buffer == NULL)
  {
    /* I think this should be an error, but okay */
    return;
  }
  yma = (union ymemarea *)buffer;
  --yma;
  trace ("checking magic start structure",
      trace_var(yma->data.size,trace_size),
      trace_var(yma->data.magic,trace_uint32),
      trace_buf(yma->padding,16));
  if (yma->data.magic != MAGIC_1)
    {
      trace ("magic start structure invlaid");
      Y_FATAL ("buffer underrun or yfree'd something that wasn't ymalloc'd? %" PRIxPTR "\n", (uintptr_t) buffer);
      abort ();
    }
  magic2 = buffer;
  magic2 += yma->data.size;
  trace ("checking magic end number",
      trace_buf(magic2,4));
  if( (magic2[0] != MAGIC_2[0]) ||
      (magic2[1] != MAGIC_2[1]) ||
      (magic2[2] != MAGIC_2[2]) ||
      (magic2[3] != MAGIC_2[3]) )
    {
      trace ("magic end number invalid");
      Y_FATAL ("buffer overrun or yfree'd something that wasn't ymalloc'd? %" PRIxPTR "\n", (uintptr_t) buffer);
      abort ();
    }
  yma->data.magic = 0xdeadf001;
  trace ("magic start structure de-magicked",
      trace_var(yma->data.size,trace_size),
      trace_var(yma->data.magic,trace_uint32),
      trace_buf(yma->padding,16));
  magic2[0] = MAGIC_2_DEAD[0];
  magic2[1] = MAGIC_2_DEAD[1];
  magic2[2] = MAGIC_2_DEAD[2];
  magic2[3] = MAGIC_2_DEAD[3];
  trace ("magic end number de-magicked",
      trace_buf(magic2,4));
  trace ("free()ing buffer");
  free (yma);
  trace ("exit");
}

char *
ystrdup (const char *s)
{
  trace ("entry", trace_var(s,trace_string));
  if (!s)
    return NULL;
  size_t len = strlen (s);
  trace ("got length", trace_var(len,trace_size));
  char * result = ymalloc (len + 1);
  trace ("allocated new string buffer", trace_var(result,trace_ptr));
  memcpy (result, s, len);
  result[len] = '\0';
  trace ("exit", trace_var(result, trace_string));
  return result;
}

/* arch-tag: 8d07f3f6-d59a-49a7-aac4-e0b0733b113a
 */
