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

#ifndef Y_INPUT_YKM_H
#define Y_INPUT_YKM_H

#include <sys/types.h>
#include <stdbool.h>

struct ykm;

extern bool ykmUpdate(struct ykm *ykm, const char *data, size_t len);
extern struct ykm *ykmParse(const char *data, size_t len);
extern struct ykm *ykmLoad(const char *filename);

extern void ykmDestroy(struct ykm *ykm);

#endif

/* arch-tag: 31a67c94-70c1-4cef-bc18-db333fcf9567
 */
