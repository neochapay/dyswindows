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

#ifndef Y_TEXT_UTF8_H
#define Y_TEXT_UTF8_H

#include <wchar.h>

extern void utf8Initialise(void);
extern void utf8Finalise(void);

/* These functions are mbstowcs and wcstombs without all the
 * issues. mbstowcs and wcstombc themselves are anathema within the Y
 * server, and WILL NOT WORK. Use these instead.
 */

/* from_len and to_len are counted in characters, not bytes */
extern size_t utf8towc(const char *from, size_t from_len, wchar_t *to, size_t to_len);
extern size_t utf8fromwc(const wchar_t *from, size_t from_len, char *to, size_t to_len);

#endif

/* arch-tag: 5a924478-c724-4d18-b8ac-a93ddb36f324
 */
