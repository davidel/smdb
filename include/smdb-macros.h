/*
 *  smdb by Davide Libenzi (Simple/Small DB Library)
 *  Copyright (C) 2010  Davide Libenzi
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#ifndef _SMDB_MACROS_H
#define _SMDB_MACROS_H

#define BUILD_BUG_IF(c) ((void) sizeof(char[1 - 2 * !!(c)]))
#define MIN(a, b) ((a) < (b) ? (a): (b))
#define MAX(a, b) ((a) > (b) ? (a): (b))
#define CSTRSIZE(s) (sizeof(s) - 1)
#define OFFSETOF(t, f) ((long) &((t *) 0)->f)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define LOCHAR(c) map_lowercase[(c) & 0xff]
#define HICHAR(c) map_uppercase[(c) & 0xff]
#define MZERO(s) smdb_memset(&(s), 0, sizeof(s))
#define OBJALLOC(m, t) ((t *) smdb_zalloc(m, sizeof(t)))

#ifdef _DEBUG
#include <stdio.h>

#define DBGPRINT(f, ...) fprintf(stderr, f, __VA_ARGS__)
#else
#define DBGPRINT(f, ...) do { } while (0)
#endif

#endif
