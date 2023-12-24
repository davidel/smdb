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

#ifndef _SMDB_CONFIG_H
#define _SMDB_CONFIG_H

/*
 * You can define those if the ones automatically calculated by the
 * SMDB machinery do not fit.
 *
 * #define SMDB_TYPE_8BIT ...
 * #define SMDB_TYPE_16BIT ...
 * #define SMDB_TYPE_32BIT ...
 * #define SMDB_TYPE_64BIT ...
 */

#define HAVE_STRING_H

#define SMDB_STRLEN(s) ((int) strlen(s))
#define SMDB_STRCPY(d, s) strcpy(d, s)
#define SMDB_STRNCPY(d, s, n) strcnpy(d, s, n)
#define SMDB_STRCHR(s, c) strchr(s, c)
#define SMDB_STRCMP(s1, s2) strcmp(s1, s2)
#define SMDB_STRNCMP(s1, s2, n) strncmp(s1, s2, n)
#define SMDB_MEMCPY(dest, src, n) memcpy(dest, src, n)
#define SMDB_MEMSET(dest, c, n) memset(dest, c, n)
#define SMDB_MEMCHR(buf, c, n) memchr(buf, c, n)
#define SMDB_MEMCMP(m1, m2, n) memcmp(m1, m2, n)

#ifdef _WIN32
#define SMDB_STRCASECMP(s1, s2) stricmp(s1, s2)
#define SMDB_STRNCASECMP(s1, s2, n) strnicmp(s1, s2, n)
#else
#define SMDB_STRCASECMP(s1, s2) strcasecmp(s1, s2)
#define SMDB_STRNCASECMP(s1, s2, n) strncasecmp(s1, s2, n)
#endif

#endif

