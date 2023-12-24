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
