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

#ifndef _SMDB_BTYPES_H
#define _SMDB_BTYPES_H

#ifndef SMDB_TYPE_8BIT
#define SMDB_TYPE_8BIT char
#endif

#ifndef SMDB_TYPE_16BIT
#define SMDB_TYPE_16BIT short
#endif

#ifndef SMDB_TYPE_32BIT
#define SMDB_TYPE_32BIT int
#endif

#ifndef SMDB_TYPE_64BIT
#if defined(_MSC_VER)
#define SMDB_TYPE_64BIT __int64
#else
#define SMDB_TYPE_64BIT long long
#endif
#endif

typedef unsigned SMDB_TYPE_8BIT smdb_u8;
typedef signed SMDB_TYPE_8BIT smdb_s8;
typedef unsigned SMDB_TYPE_16BIT smdb_u16;
typedef signed SMDB_TYPE_16BIT smdb_s16;
typedef unsigned SMDB_TYPE_32BIT smdb_u32;
typedef signed SMDB_TYPE_32BIT smdb_s32;
typedef unsigned SMDB_TYPE_64BIT smdb_u64;
typedef signed SMDB_TYPE_64BIT smdb_s64;

#endif

