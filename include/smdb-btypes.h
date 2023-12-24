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

