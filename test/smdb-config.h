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

