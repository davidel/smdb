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


int smdb_cf_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		   struct smdb_bc_config const *bcfg,
		   struct smdb_cfile_ctx **pcfctx)
{
	struct smdbxi_mem *mem;
	struct smdb_cfile_ctx *cfctx;

	if ((mem = SMDBXI_FC_MEM(fac)) == NULL ||
	    (cfctx = OBJALLOC(mem, struct smdb_cfile_ctx)) == NULL) {
		SMDBXI_RELEASE(mem);
		return -1;
	}
	if (smdb_bc_create(fac, bfile, bcfg, &cfctx->bctx) < 0) {
		SMDBXI_MM_FREE(mem, cfctx);
		SMDBXI_RELEASE(mem);
		return -1;
	}
	SMDBXI_GET(bfile);
	cfctx->mem = mem;
	cfctx->bfile = bfile;

	*pcfctx = cfctx;

	return 0;
}

void smdb_cf_free(struct smdb_cfile_ctx *cfctx)
{
	if (cfctx != NULL) {
		struct smdbxi_mem *mem = cfctx->mem;

		smdb_bc_free(cfctx->bctx);
		SMDBXI_RELEASE(cfctx->bfile);
		SMDBXI_MM_FREE(mem, cfctx);
		SMDBXI_RELEASE(mem);
	}
}

int smdb_cf_sync(struct smdb_cfile_ctx *cfctx)
{
	if (smdb_bc_sync(cfctx->bctx) < 0)
		return -1;

	return 0;
}

smdb_offset_t smdb_cf_file_size(struct smdb_cfile_ctx *cfctx)
{
	return smdb_bc_file_size(cfctx->bctx);
}

smdb_u32 smdb_cf_block_size(struct smdb_cfile_ctx *cfctx)
{
	return smdb_bc_block_size(cfctx->bctx);
}

int smdb_cf_read(struct smdb_cfile_ctx *cfctx, smdb_offset_t offset,
		 void *data, unsigned long size)
{
	smdb_u32 blk_size, blkno, blkoff;
	unsigned long csize, count;
	smdb_offset_t fsize;
	struct smdb_bc_node *bcn;

	fsize = smdb_bc_file_size(cfctx->bctx);
	if (offset >= fsize)
		return 0;
	if (offset + (smdb_offset_t) size > fsize)
		size = (unsigned long) (fsize - offset);

	blk_size = smdb_bc_block_size(cfctx->bctx);
	blkno = (smdb_u32) (offset / blk_size);
	blkoff = (smdb_u32) (offset % blk_size);
	for (csize = 0; csize < size; blkoff = 0, blkno++) {
		if ((bcn = smdb_bc_get_block(cfctx->bctx, blkno, 0)) == NULL)
			return -1;
		count = size - csize;
		if (count > (unsigned long) (blk_size - blkoff))
			count = (unsigned long) (blk_size - blkoff);

		smdb_memcpy(data, (char *) smdb_bc_get_block_data(bcn) + blkoff,
			    count);

		smdb_bc_release_block(cfctx->bctx, bcn);
		csize += count;
		data = (char *) data + count;
	}

	return csize;
}

int smdb_cf_write(struct smdb_cfile_ctx *cfctx, smdb_offset_t offset,
		  void const *data, unsigned long size)
{
	smdb_u32 blk_size, blkno, blkoff;
	unsigned long csize, count;
	struct smdb_bc_node *bcn;

	blk_size = smdb_bc_block_size(cfctx->bctx);
	blkno = (smdb_u32) (offset / blk_size);
	blkoff = (smdb_u32) (offset % blk_size);
	for (csize = 0; csize < size; blkoff = 0, blkno++) {
		if ((bcn = smdb_bc_get_block(cfctx->bctx, blkno, 1)) == NULL)
			return -1;
		count = size - csize;
		if (count > (unsigned long) (blk_size - blkoff))
			count = (unsigned long) (blk_size - blkoff);

		smdb_memcpy((char *) smdb_bc_get_block_data(bcn) + blkoff, data,
			    count);

		smdb_bc_set_block_dirty(cfctx->bctx, bcn);
		smdb_bc_release_block(cfctx->bctx, bcn);
		csize += count;
		data = (char const *) data + count;
	}

	return csize;
}

struct smdb_bc_node *smdb_cf_get_block(struct smdb_cfile_ctx *cfctx,
				       smdb_u32 blkno, int excl)
{
	return smdb_bc_get_block(cfctx->bctx, blkno, excl);
}

void smdb_cf_release_block(struct smdb_cfile_ctx *cfctx,
			   struct smdb_bc_node *bcn)
{
	smdb_bc_release_block(cfctx->bctx, bcn);
}

void smdb_cf_set_block_dirty(struct smdb_cfile_ctx *cfctx,
			     struct smdb_bc_node *bcn)
{
	smdb_bc_set_block_dirty(cfctx->bctx, bcn);
}

int smdb_cf_copy(struct smdb_cfile_ctx *cfctx, smdb_u32 bdest, smdb_u32 bsrc,
		 smdb_u32 nblocks)
{
	smdb_u32 i, blk_size;
	struct smdb_bc_node *bcnd, *bcns;

	blk_size = smdb_bc_block_size(cfctx->bctx);
	for (i = 0; i < nblocks; i++) {
		if ((bcnd = smdb_cf_get_block(cfctx, bdest + i, 1)) == NULL)
			return -1;
		if ((bcns = smdb_cf_get_block(cfctx, bsrc + i, 0)) == NULL) {
			smdb_cf_release_block(cfctx, bcnd);
			return -1;
		}

		smdb_memcpy(smdb_bc_get_block_data(bcnd),
			    smdb_bc_get_block_data(bcns),
			    blk_size);

		smdb_cf_release_block(cfctx, bcns);
		smdb_cf_set_block_dirty(cfctx, bcnd);
		smdb_cf_release_block(cfctx, bcnd);
	}

	return 0;
}

int smdb_cf_zero(struct smdb_cfile_ctx *cfctx, smdb_u32 blkno, smdb_u32 nblocks)
{
	smdb_u32 i, blk_size;
	struct smdb_bc_node *bcn;

	blk_size = smdb_bc_block_size(cfctx->bctx);
	for (i = 0; i < nblocks; i++) {
		if ((bcn = smdb_cf_get_block(cfctx, blkno + i, 1)) == NULL)
			return -1;

		smdb_memset(smdb_bc_get_block_data(bcn), 0, blk_size);

		smdb_cf_set_block_dirty(cfctx, bcn);
		smdb_cf_release_block(cfctx, bcn);
	}

	return 0;
}

