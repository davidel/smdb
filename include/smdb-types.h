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


#ifndef _SMDB_TYPES_H
#define _SMDB_TYPES_H

struct smdb_db_key {
	void *data;
	unsigned long size;
};

struct smdb_db_ckey {
	void const *data;
	unsigned long size;
};

struct smdb_db_data {
	void *data;
	unsigned long size;
};

struct smdb_db_cdata {
	void const *data;
	unsigned long size;
};

struct smdb_db_record {
	void *record;
	struct smdb_db_key key;
	struct smdb_db_data data;
};

#endif

