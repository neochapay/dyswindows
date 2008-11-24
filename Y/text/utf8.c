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

#include <Y/text/utf8.h>
#include <Y/util/log.h>

#include <wchar.h>
#include <iconv.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

static iconv_t fromutf8 = (iconv_t)-1;
static iconv_t toutf8 = (iconv_t)-1;

void
utf8Initialise(void)
{
  fromutf8 = iconv_open("WCHAR_T", "UTF-8");
  if (fromutf8 == (iconv_t)-1)
    {
      Y_FATAL("Failed to get iconf handle for UTF-8 -> WCHAR_T: %s", strerror(errno));
      exit(1);
    }
  toutf8 = iconv_open("UTF-8", "WCHAR_T");
  if (toutf8 == (iconv_t)-1)
    {
      Y_FATAL("Failed to get iconf handle for WCHAR_T -> UTF-8: %s", strerror(errno));
      exit(1);
    }
}

void
utf8Finalise(void)
{
  iconv_close(fromutf8);
  iconv_close(toutf8);
}

size_t
utf8towc(const char *from, size_t from_len, wchar_t *to, size_t to_len)
{
  size_t in_buf_len = from_len;
  char in_buf_data[in_buf_len];
  memcpy(in_buf_data, from, in_buf_len);
  char *in_buf = in_buf_data;
  size_t out_buf_len = to_len * sizeof(wchar_t);
  char *out_buf = (char *)to;

  /* Reset the state */
  if (iconv(fromutf8, NULL, 0, &out_buf, &out_buf_len) == (size_t)-1)
    return -1;

  /* Do the conversion */
  size_t ret = iconv(fromutf8, &in_buf, &in_buf_len, &out_buf, &out_buf_len);
  if (ret == (size_t)-1)
    return -1;

  /* Compute the number of characters placed in the output buffer */
  size_t out_chars = to_len - (out_buf_len / sizeof(wchar_t));

  return out_chars;
}

size_t
utf8fromwc(const wchar_t *from, size_t from_len, char *to, size_t to_len)
{
  size_t in_buf_len = from_len * sizeof(wchar_t);
  char in_buf_data[in_buf_len];
  memcpy(in_buf_data, from, in_buf_len);
  char *in_buf = in_buf_data;
  size_t out_buf_len = to_len;
  char *out_buf = to;

  /* Reset the state */
  if (iconv(toutf8, NULL, 0, &out_buf, &out_buf_len) == (size_t)-1)
    return -1;

  /* Do the conversion */
  size_t ret = iconv(toutf8, &in_buf, &in_buf_len, &out_buf, &out_buf_len);
  if (ret == (size_t)-1)
    return -1;

  /* Compute the number of characters placed in the output buffer */
  size_t out_chars = to_len - out_buf_len;

  return out_chars;
}

/* arch-tag: 7cbb9ddc-5725-4e4d-9913-fb9fdf497e0d
 */
