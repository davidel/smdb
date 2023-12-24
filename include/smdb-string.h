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

#ifndef _SMDB_STRING_H
#define _SMDB_STRING_H

#ifdef HAVE_STRING_H
#include <string.h>
#endif

EXTC_BEGIN;

char *smdb_strdup(struct smdbxi_mem *mem, char const *str);

#ifdef SMDB_STRLEN
#define smdb_strlen(str) SMDB_STRLEN(str)
#else
int smdb_strlen(char const *str);
#endif

#ifdef SMDB_STRCPY
#define smdb_strcpy(dest, src) SMDB_STRCPY(dest, src)
#else
char *smdb_strcpy(char *dest, char const *src);
#endif

#ifdef SMDB_STRNCPY
#define smdb_strncpy(dest, src, n) SMDB_STRNCPY(dest, src, n)
#else
char *smdb_strncpy(char *dest, char const *src, int n);
#endif

#ifdef SMDB_STRCHR
#define smdb_strchr(str, c) SMDB_STRCHR(str, c)
#else
char *smdb_strchr(char const *str, int c);
#endif

#ifdef SMDB_STRCMP
#define smdb_strcmp(s1, s2) SMDB_STRCMP(s1, s2)
#else
int smdb_strcmp(char const *s1, char const *s2);
#endif

#ifdef SMDB_STRNCMP
#define smdb_strncmp(s1, s2, n) SMDB_STRNCMP(s1, s2, n)
#else
int smdb_strncmp(char const *s1, char const *s2, int n);
#endif

#ifdef SMDB_STRCASECMP
#define smdb_strcasecmp(s1, s2) SMDB_STRCASECMP(s1, s2)
#else
int smdb_strcasecmp(char const *s1, char const *s2);
#endif

#ifdef SMDB_STRNCASECMP
#define smdb_strncasecmp(s1, s2, n) SMDB_STRNCASECMP(s1, s2, n)
#else
int smdb_strncasecmp(char const *s1, char const *s2, int n);
#endif

#ifdef SMDB_MEMCPY
#define smdb_memcpy(dest, src, n) SMDB_MEMCPY(dest, src, n)
#else
void *smdb_memcpy(void *dest, void const *src, int n);
#endif

#ifdef SMDB_MEMSET
#define smdb_memset(dest, c, n) SMDB_MEMSET(dest, c, n)
#else
void *smdb_memset(void *dest, int c, int n);
#endif

#ifdef SMDB_MEMCHR
#define smdb_memchr(buf, c, n) SMDB_MEMCHR(buf, c, n)
#else
void *smdb_memchr(void const *buf, int c, int n);
#endif

#ifdef SMDB_MEMCMP
#define smdb_memcmp(m1, m2, n) SMDB_MEMCMP(m1, m2, n)
#else
int smdb_memcmp(void const *m1, void const *m2, int n);
#endif

EXTC_END;

#endif

