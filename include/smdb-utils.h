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


#ifndef _SMDB_UTILS_H
#define _SMDB_UTILS_H

struct smdb_bits_find_ctx {
	unsigned long base;
	unsigned long bccount;
	unsigned long bfirst;
};

EXTC_BEGIN;

void *smdb_zalloc(struct smdbxi_mem *mem, unsigned int size);
void smdb_bits_set(smdb_u32 *bmp, unsigned long start_bit, unsigned long nbits);
void smdb_bits_clear(smdb_u32 *bmp, unsigned long start_bit, unsigned long nbits);
int smdb_bits_find_clear(smdb_u32 const *bmp, unsigned long start_bit,
			 unsigned long nbits, unsigned long bsize,
			 struct smdb_bits_find_ctx *fctx, unsigned long *pbitno);
int smdb_get_order(unsigned long count);
unsigned long smdb_get_hash(void const *data, unsigned long size,
                            unsigned long hashv);
int smdb_off_read(struct smdbxi_file *file, smdb_offset_t offset,
		  void *data, int size);
int smdb_off_write(struct smdbxi_file *file, smdb_offset_t offset,
		   void const *data, int size);

EXTC_END;

#endif

