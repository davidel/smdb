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

