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

#include "smdb-incl.h"


char *smdb_strdup(struct smdbxi_mem *mem, char const *str)
{
	int len = smdb_strlen(str);
	char *dup;

	if ((dup = (char *) SMDBXI_MM_ALLOC(mem, len + 1)) == NULL)
		return NULL;
	smdb_memcpy(dup, str, len + 1);

	return dup;
}

#ifndef SMDB_STRLEN
int smdb_strlen(char const *str)
{
	char const *tmp;

	for (tmp = str; *tmp; tmp++);

	return (int) (tmp - str);
}
#endif

#ifndef SMDB_STRCPY
char *smdb_strcpy(char *dest, char const *src)
{
	char *d;

	for (d = dest; *src; d++, src++)
		*d = *src;

	return dest;
}
#endif

#ifndef SMDB_STRNCPY
char *smdb_strncpy(char *dest, char const *src, int n)
{
	char *d;

	for (d = dest; *src && n > 0; d++, src++, n--)
		*d = *src;
	if (n > 0)
		*d = '\0';

	return dest;
}
#endif

#ifndef SMDB_STRCHR
char *smdb_strchr(char const *str, int c)
{
	for (; *str; str++)
		if (*str == (char) c)
			return (char *) str;

	return NULL;
}
#endif

#ifndef SMDB_STRCMP
int smdb_strcmp(char const *s1, char const *s2)
{
	for (; *s1 && *s2 && *s1 == *s2; s1++, s2++);

	return (int) *s1 - (int) *s2;
}
#endif

#ifndef SMDB_STRNCMP
int smdb_strncmp(char const *s1, char const *s2, int n)
{
	for (n--; n > 0 && *s1 && *s2 && *s1 == *s2; s1++, s2++, n--);

	return n < 0 ? 0: (int) *s1 - (int) *s2;
}
#endif

#ifndef SMDB_STRCASECMP
int smdb_strcasecmp(char const *s1, char const *s2)
{
	for (; *s1 && *s2 && LOCHAR(*s1) == LOCHAR(*s2); s1++, s2++);

	return (int) LOCHAR(*s1) - (int) LOCHAR(*s2);
}
#endif

#ifndef SMDB_STRNCASECMP
int smdb_strncasecmp(char const *s1, char const *s2, int n)
{
	for (n--; n > 0 && *s1 && *s2 && LOCHAR(*s1) == LOCHAR(*s2);
	     s1++, s2++, n--);

	return n < 0 ? 0: (int) LOCHAR(*s1) - (int) LOCHAR(*s2);
}
#endif

#ifndef SMDB_MEMCPY
void *smdb_memcpy(void *dest, void const *src, int n)
{
	unsigned char *d = (unsigned char *) dest;
	unsigned char const *s = (unsigned char const *) src;

	d = (unsigned char *) dest;
	s = (unsigned char const *) src;
	for (; n > 0; d++, s++, n--)
		*d = *s;

	return dest;
}
#endif

#ifndef SMDB_MEMSET
void *smdb_memset(void *dest, int c, int n)
{
	unsigned char *d;

	for (d = (unsigned char *) dest; n > 0; d++, n--)
		*d = (unsigned char) c;

	return dest;
}
#endif

#ifndef SMDB_MEMCHR
void *smdb_memchr(void const *buf, int c, int n)
{
	unsigned char const *tmp;

	for (tmp = (unsigned char const *) buf; n > 0; n--, tmp++)
		if (*tmp == (unsigned char) c)
			return (void *) tmp;

	return NULL;
}
#endif

#ifndef SMDB_MEMCMP
int smdb_memcmp(void const *m1, void const *m2, int n)
{
	unsigned char const *u1 = (unsigned char const *) m1;
	unsigned char const *u2 = (unsigned char const *) m2;

	for (; n > 0 && *u1 == *u2; u1++, u2++, n--);

	return n == 0 ? 0: (int) *u1 - (int) *u2;
}
#endif

