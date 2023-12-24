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


void *smdb_zalloc(struct smdbxi_mem *mem, unsigned int size)
{
	void *data;

	if ((data = SMDBXI_MM_ALLOC(mem, size)) != NULL)
		smdb_memset(data, 0, size);

	return data;
}

void smdb_bits_set(smdb_u32 *bmp, unsigned long start_bit, unsigned long nbits)
{
	unsigned long n, bitno;

	bmp += start_bit / 32;
	bitno = start_bit % 32;
	if (bitno > 0) {
		if ((n = 32 - bitno) > nbits)
			n = nbits;
		*bmp |= (smdb_u32) (((1UL << n) - 1) << bitno);
		nbits -= n;
		bmp++;
	}
	if (nbits > 0) {
		if ((n = nbits / 32) > 0) {
			smdb_memset(bmp, 0xff, (int) n * 4);
			bmp += n;
		}
		nbits %= 32;
		if (nbits > 0)
			*bmp |= (smdb_u32) ((1UL << nbits) - 1);
	}
}

void smdb_bits_clear(smdb_u32 *bmp, unsigned long start_bit, unsigned long nbits)
{
	unsigned long n, bitno;

	bmp += start_bit / 32;
	bitno = start_bit % 32;
	if (bitno > 0) {
		if ((n = 32 - bitno) > nbits)
			n = nbits;
		*bmp &= ~(smdb_u32) (((1UL << n) - 1) << bitno);
		nbits -= n;
		bmp++;
	}
	if (nbits > 0) {
		if ((n = nbits / 32) > 0) {
			smdb_memset(bmp, 0, (int) n * 4);
			bmp += n;
		}
		nbits %= 32;
		if (nbits > 0)
			*bmp &= (smdb_u32) ~((1UL << nbits) - 1);
	}
}

int smdb_bits_find_clear(smdb_u32 const *bmp, unsigned long start_bit,
			 unsigned long nbits, unsigned long bsize,
			 struct smdb_bits_find_ctx *fctx, unsigned long *pbitno)
{
	smdb_u32 bit;
	unsigned long bitno, bccount, bfirst;

	bmp += start_bit / 32;
	bit = (smdb_u32) (1UL << (start_bit % 32));
	bccount = fctx->bccount;
	bfirst = fctx->bfirst;
	for (bitno = start_bit; bitno < nbits; bit <<= 1, bitno++) {
		if (bit == 0) {
			bit = 1;
			bmp++;
			if (*bmp == 0xffffffff) {
				bccount = 0;
				while (bitno + 32 < nbits &&
				       *bmp == 0xffffffff) {
					bitno += 32;
					bmp++;
				}
			} else if (*bmp == 0) {
				if (bccount == 0)
					bfirst = bitno + fctx->base;
				while (bccount + 32 < bsize &&
				       bitno + 32 < nbits && *bmp == 0) {
					bitno += 32;
					bmp++;
					bccount += 32;
				}
			}
		}

		if ((*bmp & bit) == 0) {
			if (bccount == 0)
				bfirst = bitno + fctx->base;
			bccount++;
			if (bccount == bsize) {
				*pbitno = bfirst;
				return 1;
			}
		} else
			bccount = 0;
	}
	fctx->bccount = bccount;
	fctx->bfirst = bfirst;

	return 0;
}

int smdb_get_order(unsigned long count)
{
	int i;
	unsigned long bit;

	for (i = 0, bit = 1; bit < count; bit <<= 1, i++);

	return i;
}

unsigned long smdb_get_hash(void const *data, unsigned long size,
                            unsigned long hashv)
{
	smdb_u8 const *udata;

	udata = (smdb_u8 const *) data;
        while (size > 0) {
                --size;
                hashv += *udata++;
                hashv += (hashv << 10);
                hashv ^= (hashv >> 6);
        }
        hashv += (hashv << 3);
        hashv ^= (hashv >> 11);
        hashv += (hashv << 15);

        return hashv;
}

int smdb_off_read(struct smdbxi_file *file, smdb_offset_t offset,
		  void *data, int size)
{
	if (SMDBXI_FL_SEEK(file, offset, SMDBXI_FL_SEEKSET) != offset)
		return -1;

	return SMDBXI_FL_READ(file, data, size);
}

int smdb_off_write(struct smdbxi_file *file, smdb_offset_t offset,
		   void const *data, int size)
{
	if (SMDBXI_FL_SEEK(file, offset, SMDBXI_FL_SEEKSET) != offset)
		return -1;

	return SMDBXI_FL_WRITE(file, data, size);
}

