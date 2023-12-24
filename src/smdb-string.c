/*    Copyright 2023 Davide Libenzi
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

