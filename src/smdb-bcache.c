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


static void smdb_bc_free_node(struct smdbxi_mem *mem,
			      struct smdb_bc_node *bcn)
{
	if (bcn != NULL) {
		SMDBXI_MM_FREE(mem, bcn->data);
		SMDBXI_MM_FREE(mem, bcn);
	}
}

static struct smdb_bc_node *smdb_bc_alloc_node(struct smdb_bc_ctx *bctx,
					       smdb_u32 blkno)
{
	struct smdb_bc_node *bcn;

	if ((bcn = OBJALLOC(bctx->mem, struct smdb_bc_node)) == NULL ||
	    (bcn->data = SMDBXI_MM_ALLOC(bctx->mem, bctx->blk_size)) == NULL) {
		SMDBXI_MM_FREE(bctx->mem, bcn);
		return NULL;
	}
	SMDB_INIT_LIST_HEAD(&bcn->lnk);
	SMDB_INIT_LIST_HEAD(&bcn->lrulnk);
	bcn->blkno = blkno;

	return bcn;
}

static int smdb_bc_write_block(struct smdb_bc_ctx *bctx, void const *data)
{
	/*
	 * All the I/O done on the underlying file MUST be done in multiple
	 * of the block size.
	 */
	return SMDBXI_FL_WRITE(bctx->bfile, data,
			       bctx->blk_size) != (int) bctx->blk_size ? -1: 0;
}

static int smdb_bc_read_block(struct smdb_bc_ctx *bctx, void *data)
{
	/*
	 * All the I/O done on the underlying file MUST be done in multiple
	 * of the block size.
	 */
	return SMDBXI_FL_READ(bctx->bfile, data,
			      bctx->blk_size) != (int) bctx->blk_size ? -1: 0;
}

static int smdb_bc_file_grow(struct smdb_bc_ctx *bctx, smdb_offset_t size)
{
	smdb_offset_t csize;
	void *zbuf;

	/*
	 * Current and new size MUST be block-aligned!
	 */
	if ((size % bctx->blk_size) != 0 ||
	    (csize = SMDBXI_FL_SEEK(bctx->bfile, 0, SMDBXI_FL_SEEKEND)) < 0 ||
	    (csize % bctx->blk_size) != 0 ||
	    (zbuf = smdb_zalloc(bctx->mem, bctx->blk_size)) == NULL)
		return -1;

	while (csize < size) {
		if (smdb_bc_write_block(bctx, zbuf) < 0) {
			SMDBXI_MM_FREE(bctx->mem, zbuf);
			return -1;
		}
		csize += bctx->blk_size;
	}
	SMDBXI_MM_FREE(bctx->mem, zbuf);

	return 0;
}

static int smdb_bc_load_node(struct smdb_bc_ctx *bctx,
			     struct smdb_bc_node *bcn)
{
	smdb_offset_t offset;

	offset = (smdb_offset_t) bcn->blkno * bctx->blk_size;
	if (offset + bctx->blk_size > bctx->fsize) {
		if (smdb_bc_file_grow(bctx, offset + bctx->blk_size) < 0)
			return -1;
		bctx->fsize = offset + bctx->blk_size;
	}

	if (SMDBXI_FL_SEEK(bctx->bfile, offset, SMDBXI_FL_SEEKSET) != offset ||
	    smdb_bc_read_block(bctx, bcn->data) < 0)
		return -1;

	return 0;
}

static int smdb_bc_store_node(struct smdb_bc_ctx *bctx,
			      struct smdb_bc_node *bcn)
{
	smdb_offset_t offset;

	offset = (smdb_offset_t) bcn->blkno * bctx->blk_size;
	if (offset + bctx->blk_size > bctx->fsize) {
		if (smdb_bc_file_grow(bctx, offset + bctx->blk_size) < 0)
			return -1;
		bctx->fsize = offset + bctx->blk_size;
	}

	if (SMDBXI_FL_SEEK(bctx->bfile, offset, SMDBXI_FL_SEEKSET) != offset ||
	    smdb_bc_write_block(bctx, bcn->data) < 0)
		return -1;

	return 0;
}

static int smdb_bc_sync_node(struct smdb_bc_ctx *bctx,
			     struct smdb_bc_node *bcn)
{
	int syncd = 0;

	if (bcn->flags & SMDB_BCF_DIRTY) {
		if (smdb_bc_store_node(bctx, bcn) < 0)
			return -1;
		bcn->flags &= ~SMDB_BCF_DIRTY;
		syncd = 1;
	}

	return syncd;
}

static struct smdb_bc_node *smdb_bc_get_node(struct smdb_bc_ctx *bctx,
					     smdb_u32 blkno)
{
	smdb_u32 idx;
	struct smdb_listhead *head, *pos;
	struct smdb_bc_node *bcn;

	idx = (blkno ^ (blkno >> 7)) & bctx->hash_mask;
	head = &bctx->hash[idx];
	SMDB_LIST_FOR_EACH(pos, head) {
		bcn = SMDB_LIST_ENTRY(pos, struct smdb_bc_node, lnk);
		if (bcn->blkno == blkno) {
			SMDB_LIST_DEL(&bcn->lrulnk);
			SMDB_LIST_ADDH(&bcn->lrulnk, &bctx->lru);
			return bcn;
		}
	}
	/*
	 * No luck, we didn't find the block we were looking for.
	 */
	if (bctx->blk_count < bctx->blk_max) {
		/*
		 * Since we are under our quota, we can allocate a new
		 * block and load it up.
		 */
		if ((bcn = smdb_bc_alloc_node(bctx, blkno)) == NULL ||
		    smdb_bc_load_node(bctx, bcn) < 0) {
			smdb_bc_free_node(bctx->mem, bcn);
			return NULL;
		}
		SMDB_LIST_ADDH(&bcn->lrulnk, &bctx->lru);
		SMDB_LIST_ADDT(&bcn->lnk, head);
		bctx->blk_count++;
	} else {
		/*
		 * Pick a block from the LRU list, which is not pinned
		 * by the upper layers.
		 */
		for (pos = SMDB_LIST_LAST(&bctx->lru); pos != NULL;
		     pos = SMDB_LIST_PREV(&bctx->lru, pos)) {
			bcn = SMDB_LIST_ENTRY(pos, struct smdb_bc_node, lrulnk);
			if (bcn->usecnt == 0)
				break;
		}
		/*
		 * If we foudn a valid one, we need to sync it on media before
		 * re-using it.
		 */
		if (pos == NULL ||
		    smdb_bc_sync_node(bctx, bcn) < 0)
			return NULL;
		/*
		 * Assign the LRU-ed block to the new one.
		 */
		bcn->blkno = blkno;
		bcn->flags = 0;
		SMDB_LIST_DEL(&bcn->lrulnk);
		SMDB_LIST_DEL(&bcn->lnk);
		if (smdb_bc_load_node(bctx, bcn) < 0) {
			smdb_bc_free_node(bctx->mem, bcn);
			bctx->blk_count--;
			return NULL;
		}
		SMDB_LIST_ADDH(&bcn->lrulnk, &bctx->lru);
		SMDB_LIST_ADDT(&bcn->lnk, head);
	}

	return bcn;
}

static void smdb_bc_free_hash(struct smdbxi_mem *mem,
			      struct smdb_listhead *hash, smdb_u32 hash_mask)
{
	smdb_u32 i;
	struct smdb_bc_node *bcn;
	struct smdb_listhead *head, *pos;

	for (i = 0; i <= hash_mask; i++) {
		head = &hash[i];
		while ((pos = SMDB_LIST_FIRST(head)) != NULL) {
			bcn = SMDB_LIST_ENTRY(pos, struct smdb_bc_node, lnk);
			SMDB_LIST_DEL(&bcn->lnk);
			smdb_bc_free_node(mem, bcn);
		}
	}
}

int smdb_bc_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		   struct smdb_bc_config const *bcfg,
		   struct smdb_bc_ctx **pbctx)
{
	smdb_u32 i;
	smdb_offset_t fsize;
	struct smdbxi_mem *mem;
	struct smdb_bc_ctx *bctx;

	/*
	 * The underlying file MUST be a multiple of block size in size.
	 */
	if ((fsize = SMDBXI_FL_SEEK(bfile, 0, SMDBXI_FL_SEEKEND)) < 0 ||
	    (fsize % bcfg->blk_size) != 0)
		return -1;

	if ((mem = SMDBXI_FC_MEM(fac)) == NULL ||
	    (bctx = OBJALLOC(mem, struct smdb_bc_ctx)) == NULL) {
		SMDBXI_RELEASE(mem);
		return -1;
	}
	SMDBXI_GET(bfile);
	bctx->mem = mem;
	bctx->bfile = bfile;
	bctx->blk_size = bcfg->blk_size;
	bctx->blk_max = bcfg->blk_max;
	SMDB_INIT_LIST_HEAD(&bctx->lru);
	bctx->fsize = fsize;

	for (i = 1; i <= bcfg->blk_max; i <<= 1);

	if ((bctx->hash = (struct smdb_listhead *)
	     SMDBXI_MM_ALLOC(mem, i * sizeof(struct smdb_listhead))) == NULL) {
		smdb_bc_free(bctx);
		return -1;
	}
	bctx->hash_mask = i - 1;
	for (; i > 0; i--)
		SMDB_INIT_LIST_HEAD(&bctx->hash[i - 1]);

	*pbctx = bctx;

	return 0;
}

void smdb_bc_free(struct smdb_bc_ctx *bctx)
{
	if (bctx != NULL) {
		struct smdbxi_mem *mem = bctx->mem;

		if (bctx->hash != NULL) {
			smdb_bc_free_hash(mem, bctx->hash, bctx->hash_mask);
			SMDBXI_MM_FREE(mem, bctx->hash);
		}
		SMDBXI_RELEASE(bctx->bfile);
		SMDBXI_MM_FREE(mem, bctx);
		SMDBXI_RELEASE(mem);
	}
}

int smdb_bc_sync(struct smdb_bc_ctx *bctx)
{
	struct smdb_bc_node *bcn;
	struct smdb_listhead *pos;

	SMDB_LIST_FOR_EACH(pos, &bctx->lru) {
		bcn = SMDB_LIST_ENTRY(pos, struct smdb_bc_node, lrulnk);
		if (smdb_bc_sync_node(bctx, bcn) < 0)
			return -1;
	}

	return SMDBXI_FL_SYNC(bctx->bfile);
}

struct smdb_bc_node *smdb_bc_get_block(struct smdb_bc_ctx *bctx,
				       smdb_u32 blkno, int excl)
{
	struct smdb_bc_node *bcn;

	if ((bcn = smdb_bc_get_node(bctx, blkno)) == NULL)
		return NULL;
	bcn->usecnt++;

	return bcn;
}

void smdb_bc_release_block(struct smdb_bc_ctx *bctx,
			   struct smdb_bc_node *bcn)
{
	bcn->usecnt--;
}

void smdb_bc_set_block_dirty(struct smdb_bc_ctx *bctx,
			     struct smdb_bc_node *bcn)
{
	bcn->flags |= SMDB_BCF_DIRTY;
}

smdb_u32 smdb_bc_block_size(struct smdb_bc_ctx *bctx)
{
	return bctx->blk_size;
}

smdb_offset_t smdb_bc_file_size(struct smdb_bc_ctx *bctx)
{
	return bctx->fsize;
}

void *smdb_bc_get_block_data(struct smdb_bc_node *bcn)
{
	return bcn->data;
}

