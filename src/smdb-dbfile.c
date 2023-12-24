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

#define SMDB_DBF_MAGIC "SMSIDB01"
#define SMDB_MAX_ORDER 32
#define SMDB_HASHV_INIT 9587
#define SMDB_MIN_BLKSIZE 256

struct smdb_db_file {
	smdb_u32 blkno;
	smdb_u32 size;
};

struct smdb_db_rstorage {
	smdb_u32 hashv;
	smdb_u32 ksize;
	smdb_u32 dsize;
};

struct smdb_db_table {
	struct smdb_db_file hash;
	smdb_u32 num_recs;
};

struct smdb_db_header {
	smdb_u8 magic[8];
	smdb_u32 blk_size;
	smdb_u32 blk_count;
	smdb_u32 blk_alloc;
	smdb_u32 first_free[SMDB_MAX_ORDER];
	struct smdb_db_file bitmap;
	smdb_u32 num_tables;
	struct smdb_db_file tables;
	smdb_u32 num_recs;
};

struct smdb_db_env {
	struct smdb_bc_node *mbcn;
	struct smdb_bc_node *tbcn;
	struct smdb_db_header *hdr;
	struct smdb_db_table *tbl;
};


static int smdb_dbf_expand(struct smdb_cfile_ctx *cfctx,
			   struct smdb_db_header *hdr, smdb_u32 nblocks,
			   smdb_u32 *pblkno)
{
	smdb_u32 i, blkno;
	struct smdb_bc_node *bcn;

	DBGPRINT("Expanding DBF from %lu blocks, %lu blocks\n",
		 (unsigned long) 1 + hdr->blk_count,
		 (unsigned long) nblocks);

	blkno = 1 + hdr->blk_count;
	for (i = 0; i < nblocks; i++) {
		if ((bcn = smdb_cf_get_block(cfctx, blkno + i, 1)) == NULL)
			return -1;
		smdb_cf_set_block_dirty(cfctx, bcn);
		smdb_cf_release_block(cfctx, bcn);
	}
	*pblkno = blkno;

	return 0;
}

static int smdb_dbf_setbmbits(struct smdb_cfile_ctx *cfctx,
			      struct smdb_db_header *hdr, unsigned long start_bit,
			      unsigned long nbits, int set)
{
	unsigned long blkbits, blkno, bitno, bcount, bsize;
	struct smdb_bc_node *bcn;
	smdb_u32 *bmp;

	blkbits = hdr->blk_size * 8;

	blkno = start_bit / blkbits;
	bitno = start_bit % blkbits;

	for (bcount = 0; bcount < nbits; bitno = 0, blkno++) {
		if ((bcn = smdb_cf_get_block(cfctx, hdr->bitmap.blkno + blkno,
					     1)) == NULL)
			return -1;
		bmp = (smdb_u32 *) smdb_bc_get_block_data(bcn);
		if ((bsize = blkbits - bitno) > nbits - bcount)
			bsize = nbits - bcount;
		if (set)
			smdb_bits_set(bmp, bitno, bsize);
		else
			smdb_bits_clear(bmp, bitno, bsize);
		smdb_cf_set_block_dirty(cfctx, bcn);
		smdb_cf_release_block(cfctx, bcn);
		bcount += bsize;
	}

	return 0;
}

static int smdb_dbf_get_free_blocks(struct smdb_cfile_ctx *cfctx,
				    struct smdb_db_header *hdr, smdb_u32 start_block,
				    smdb_u32 nbits, smdb_u32 *pfblkno)
{
	smdb_u32 blkno;
	unsigned long bitno, blkbits;
	struct smdb_bc_node *bcn;
	smdb_u32 *bmp;
	struct smdb_bits_find_ctx fctx;

	DBGPRINT("Looking for %lu blocks, from block %lu\n", nbits,
		 (unsigned long) start_block);

	blkbits = hdr->blk_size * 8;
	blkno = start_block / blkbits;

	MZERO(fctx);
	fctx.base = blkno * blkbits;

	for (blkno = start_block / blkbits; blkno < hdr->bitmap.size; blkno++) {
		if ((bcn = smdb_cf_get_block(cfctx, hdr->bitmap.blkno + blkno,
					     0)) == NULL)
			return -1;
		bmp = (smdb_u32 *) smdb_bc_get_block_data(bcn);

		if (smdb_bits_find_clear(bmp, 0, blkbits, nbits, &fctx, &bitno)) {
			smdb_cf_release_block(cfctx, bcn);
			*pfblkno = (smdb_u32) bitno;

			DBGPRINT("Found start in block %lu\n", bitno);

			return 1;
		}
		fctx.base += blkbits;
		smdb_cf_release_block(cfctx, bcn);
	}

	DBGPRINT("Looking for %lu blocks, from block %lu - FAILED!\n", nbits,
		 (unsigned long) start_block);

	return 0;
}

static int smdb_dbf_grow(struct smdb_cfile_ctx *cfctx,
			 struct smdb_db_header *hdr, smdb_u32 nblocks)
{
	smdb_u32 blk_count, bmpsize, bmp_blkno;
	struct smdb_db_file obitmap;

	obitmap = hdr->bitmap;

	DBGPRINT("Current bitmap at block %lu\n", (unsigned long) obitmap.blkno);

	/*
	 * Calculate new bitmap size.
	 */
	blk_count = hdr->blk_count + nblocks;
	bmpsize = (blk_count / 8) / hdr->blk_size + 1;
	blk_count = bmpsize * hdr->blk_size * 8;

	/*
	 * Grow space for new bitmap.
	 */
	if (smdb_dbf_expand(cfctx, hdr, bmpsize, &bmp_blkno) < 0)
		return -1;

	DBGPRINT("New bitmap at block %lu\n", (unsigned long) bmp_blkno);

	/*
	 * Copy old bitmap into new one.
	 */
	if (smdb_cf_copy(cfctx, bmp_blkno, obitmap.blkno, obitmap.size) < 0)
		return -1;

	/*
	 * Update DB header values for new bitmap position and size.
	 */
	hdr->blk_count = blk_count;
	hdr->bitmap.blkno = bmp_blkno;
	hdr->bitmap.size = bmpsize;

	/*
	 * Clear the bits freed from the old bitmap, and set the ones allocated
	 * for the new one.
	 */
	if (smdb_dbf_setbmbits(cfctx, hdr, obitmap.blkno, obitmap.size, 0) < 0 ||
	    smdb_dbf_setbmbits(cfctx, hdr, bmp_blkno, bmpsize, 1) < 0)
		return -1;

	return 0;
}

static int smdb_dbf_balloc(struct smdb_cfile_ctx *cfctx, smdb_u32 blkcnt,
			   smdb_u32 *pblkno)
{
	smdb_u32 fblkno, order, grow_blkcnt;
	struct smdb_bc_node *mbcn;
	struct smdb_db_header *hdr;

	order = smdb_get_order(blkcnt);
	if (order >= SMDB_MAX_ORDER ||
	    (mbcn = smdb_cf_get_block(cfctx, 0, 1)) == NULL)
		return -1;
	hdr = (struct smdb_db_header *) smdb_bc_get_block_data(mbcn);

	DBGPRINT("Allocating %lu blocks (order = %lu)\n", (unsigned long) blkcnt,
		 (unsigned long) order);

	/*
	 * Try to alloc from the last known free index before.
	 */
	if (!smdb_dbf_get_free_blocks(cfctx, hdr, hdr->first_free[order],
				      blkcnt, &fblkno)) {
		/*
		 * Then try again from the beginning, and grow the DB file
		 * if no more space is available to allocated this block.
		 */
		if (hdr->first_free[order] == 0 ||
		    !smdb_dbf_get_free_blocks(cfctx, hdr, 0,
					      blkcnt, &fblkno)) {
			/*
			 * Calculate the grow size, grow the database
			 * accordingly, and try the allocation for the requested
			 * number of blocks.
			 */
			if ((grow_blkcnt = hdr->blk_count) < blkcnt)
				grow_blkcnt = 2 * blkcnt;
			if (smdb_dbf_grow(cfctx, hdr, grow_blkcnt) < 0) {
				smdb_cf_release_block(cfctx, mbcn);
				return -1;
			}
			smdb_cf_set_block_dirty(cfctx, mbcn);
			/*
			 * Now we should be able to allocated the requested block.
			 */
			if (!smdb_dbf_get_free_blocks(cfctx, hdr, 0,
						      blkcnt, &fblkno)) {
				smdb_cf_release_block(cfctx, mbcn);
				return -1;
			}
		}
	}
	/*
	 * Set the allocation bitmap area for the allocated range.
	 */
	if (smdb_dbf_setbmbits(cfctx, hdr, fblkno, blkcnt, 1) < 0) {
		smdb_cf_release_block(cfctx, mbcn);
		return -1;
	}

	hdr->first_free[order] = fblkno + blkcnt;
	hdr->blk_alloc += blkcnt;

	smdb_cf_set_block_dirty(cfctx, mbcn);
	smdb_cf_release_block(cfctx, mbcn);

	*pblkno = fblkno;

	return 0;
}

static int smdb_dbf_alloc_file(struct smdb_cfile_ctx *cfctx, smdb_u32 blkcnt,
			       struct smdb_db_file *dbf)
{
	smdb_u32 blkno;

	if (smdb_dbf_balloc(cfctx, blkcnt, &blkno) < 0)
		return -1;

	MZERO(*dbf);
	dbf->blkno = blkno;
	dbf->size = blkcnt;

	return 0;
}

static int smdb_dbf_bfree(struct smdb_cfile_ctx *cfctx, smdb_u32 blkno,
			  smdb_u32 blkcnt)
{
	smdb_u32 order;
	struct smdb_bc_node *mbcn;
	struct smdb_db_header *hdr;

	order = smdb_get_order(blkcnt);
	if (order >= SMDB_MAX_ORDER ||
	    (mbcn = smdb_cf_get_block(cfctx, 0, 1)) == NULL)
		return -1;
	hdr = (struct smdb_db_header *) smdb_bc_get_block_data(mbcn);

	if (smdb_dbf_setbmbits(cfctx, hdr, blkno, blkcnt, 0) < 0) {
		smdb_cf_set_block_dirty(cfctx, mbcn);
		smdb_cf_release_block(cfctx, mbcn);
		return -1;
	}
	if (blkno < hdr->first_free[order])
		hdr->first_free[order] = blkno;
	hdr->blk_alloc -= blkcnt;

	smdb_cf_set_block_dirty(cfctx, mbcn);
	smdb_cf_release_block(cfctx, mbcn);

	return 0;
}

static int smdb_dbf_initdb(struct smdb_cfile_ctx *cfctx,
			   struct smdb_db_config const *dbcfg)
{
	smdb_u32 i, tbl_nblocks;
	struct smdb_bc_node *mbcn, *bcn;
	struct smdb_db_header *hdr;

	if ((mbcn = smdb_cf_get_block(cfctx, 0, 1)) == NULL)
		return -1;
	hdr = (struct smdb_db_header *) smdb_bc_get_block_data(mbcn);

	smdb_memcpy(hdr->magic, SMDB_DBF_MAGIC, sizeof(hdr->magic));
	hdr->blk_size = dbcfg->blk_size;
	hdr->bitmap.blkno = 1;
	hdr->bitmap.size = (dbcfg->blk_count / 8) / dbcfg->blk_size + 1;
	if (hdr->bitmap.size < 1)
		hdr->bitmap.size = 1;
	hdr->blk_count = hdr->bitmap.size * dbcfg->blk_size * 8;
	hdr->num_tables = dbcfg->num_tables;

	/*
	 * Initial space for the bitmap. From block 1 ahead ...
	 */
	for (i = 0; i < hdr->bitmap.size; i++) {
		if ((bcn = smdb_cf_get_block(cfctx, i + 1, 1)) == NULL) {
			smdb_cf_release_block(cfctx, mbcn);
			return -1;
		}
		smdb_cf_set_block_dirty(cfctx, bcn);
		smdb_cf_release_block(cfctx, bcn);
	}
	hdr->blk_alloc = 1 + hdr->bitmap.size;

	/*
	 * Mark the first block (DB header) and the initial bitmap
	 * blocks as allocated.
	 * Then allocates the tables files according to the requested size.
	 */
	tbl_nblocks = (dbcfg->num_tables * sizeof(struct smdb_db_table)) /
		dbcfg->blk_size + 1;
	if (smdb_dbf_setbmbits(cfctx, hdr, 0, 1 + hdr->bitmap.size,
			       1) < 0 ||
	    smdb_dbf_alloc_file(cfctx, tbl_nblocks, &hdr->tables) < 0) {
		smdb_cf_set_block_dirty(cfctx, mbcn);
		smdb_cf_release_block(cfctx, mbcn);
		return -1;
	}
	smdb_cf_set_block_dirty(cfctx, mbcn);
	smdb_cf_release_block(cfctx, mbcn);

	return 0;
}

static struct smdb_dbfile_ctx *smdb_dbf_alloc_ctx(struct smdbxi_factory *fac,
						  struct smdbxi_file *bfile,
						  unsigned int blk_size)
{
	struct smdbxi_mem *mem;
	struct smdbxi_fs *fs;
	struct smdb_jfile_ctx *jfctx;
	struct smdb_dbfile_ctx *dfctx;

	if (smdb_jf_create(fac, bfile, blk_size, &jfctx) < 0)
		return NULL;

	mem = SMDBXI_FC_MEM(fac);
	fs = SMDBXI_FC_FS(fac);
	if (mem == NULL || fs == NULL ||
	    (dfctx = OBJALLOC(mem, struct smdb_dbfile_ctx)) == NULL) {
		SMDBXI_RELEASE(fs);
		SMDBXI_RELEASE(mem);
		smdb_jf_free(jfctx);
		return NULL;
	}
	dfctx->mem = mem;
	dfctx->fs = fs;
	dfctx->jfctx = jfctx;
	dfctx->bfile = smdb_jf_getfile(jfctx);
	SMDBXI_GET(dfctx->bfile);

	return dfctx;
}

int smdb_dbf_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		    struct smdb_db_config const *dbcfg,
		    struct smdb_dbfile_ctx **pdfctx)
{
	struct smdb_dbfile_ctx *dfctx;
	struct smdb_bc_config bcfg;

	BUILD_BUG_IF(SMDB_MIN_BLKSIZE < sizeof(struct smdb_db_header));
	if (dbcfg->blk_size < SMDB_MIN_BLKSIZE)
		return -1;

	if ((dfctx = smdb_dbf_alloc_ctx(fac, bfile, dbcfg->blk_size)) == NULL)
		return -1;

	MZERO(bcfg);
	bcfg.blk_size = dbcfg->blk_size;
	bcfg.blk_max = dbcfg->cache_size / bcfg.blk_size + 1;
	if (SMDBXI_FL_TRUNCATE(dfctx->bfile, 0) < 0 ||
	    smdb_cf_create(fac, dfctx->bfile, &bcfg, &dfctx->cfctx) < 0 ||
	    smdb_dbf_initdb(dfctx->cfctx, dbcfg) < 0 ||
	    smdb_dbf_sync(dfctx) < 0) {
		smdb_dbf_free(dfctx);
		return -1;
	}

	*pdfctx = dfctx;

	return 0;
}

static int smdb_dbf_get_header(struct smdbxi_file *bfile,
			       struct smdb_db_header *hdr)
{
	if (smdb_off_read(bfile, 0, hdr, sizeof(*hdr)) != sizeof(*hdr) ||
	    smdb_memcmp(hdr->magic, SMDB_DBF_MAGIC, sizeof(hdr->magic)) != 0)
		return -1;

	return 0;
}

int smdb_dbf_open(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		  struct smdb_db_config const *dbcfg,
		  struct smdb_dbfile_ctx **pdfctx)
{
	struct smdb_dbfile_ctx *dfctx;
	struct smdb_db_header hdr;
	struct smdb_bc_config bcfg;

	if (smdb_dbf_get_header(bfile, &hdr) < 0 ||
	    (dfctx = smdb_dbf_alloc_ctx(fac, bfile, hdr.blk_size)) == NULL)
		return -1;

	MZERO(bcfg);
	bcfg.blk_size = hdr.blk_size;
	bcfg.blk_max = dbcfg->cache_size / hdr.blk_size + 1;
	if (smdb_cf_create(fac, dfctx->bfile, &bcfg, &dfctx->cfctx) < 0) {
		smdb_dbf_free(dfctx);
		return -1;
	}

	*pdfctx = dfctx;

	return 0;
}

void smdb_dbf_free(struct smdb_dbfile_ctx *dfctx)
{
	if (dfctx != NULL) {
		struct smdbxi_mem *mem = dfctx->mem;

		if (dfctx->cfctx != NULL)
			smdb_cf_sync(dfctx->cfctx);
		smdb_cf_free(dfctx->cfctx);
		SMDBXI_RELEASE(dfctx->bfile);
		smdb_jf_free(dfctx->jfctx);
		SMDBXI_RELEASE(dfctx->fs);
		SMDBXI_MM_FREE(mem, dfctx);
		SMDBXI_RELEASE(mem);
	}
}

int smdb_dbf_sync(struct smdb_dbfile_ctx *dfctx)
{
	if (smdb_cf_sync(dfctx->cfctx) < 0)
		return -1;

	return 0;
}

static int smdb_dbf_get_env(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
			    int writep, struct smdb_db_env *env)
{
	smdb_u32 tbl_x_blk, blkno, tblidx;
	struct smdb_bc_node *bcn;

	MZERO(*env);
	if ((env->mbcn = smdb_cf_get_block(dfctx->cfctx, 0, writep)) == NULL)
		return -1;
	env->hdr = (struct smdb_db_header *) smdb_bc_get_block_data(env->mbcn);

	if ((smdb_u32) tblid >= env->hdr->num_tables) {
		smdb_cf_release_block(dfctx->cfctx, env->mbcn);
		return -1;
	}

	tbl_x_blk = env->hdr->blk_size / sizeof(struct smdb_db_table);
	blkno = (smdb_u32) tblid / tbl_x_blk;
	tblidx = (smdb_u32) tblid % tbl_x_blk;
	if ((env->tbcn = smdb_cf_get_block(dfctx->cfctx, env->hdr->tables.blkno +
					   blkno, writep)) == NULL) {
		smdb_cf_release_block(dfctx->cfctx, env->mbcn);
		return -1;
	}
	env->tbl = (struct smdb_db_table *) smdb_bc_get_block_data(env->tbcn) +
		tblidx;

	return 0;
}

static void smdb_dbf_release_env(struct smdb_dbfile_ctx *dfctx,
				 struct smdb_db_env *env)
{
	smdb_cf_release_block(dfctx->cfctx, env->tbcn);
	smdb_cf_release_block(dfctx->cfctx, env->mbcn);
}

static int smdb_dbf_grab_env(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
			     int writep, struct smdb_db_env *env)
{
	if (smdb_dbf_get_env(dfctx, tblid, writep, env) < 0)
		return -1;
	if (env->tbl->hash.size == 0) {
		smdb_dbf_release_env(dfctx, env);
		return -1;
	}

	return 0;
}

static int smdb_dbf_rec_alloc(struct smdb_dbfile_ctx *dfctx,
			      struct smdb_db_header *hdr,
			      struct smdb_db_file const *dbf,
			      struct smdb_db_rstorage *stg,
			      struct smdb_db_record *rec)
{
	smdb_u32 i;
	struct smdb_bc_node *bcn;

	MZERO(*rec);
	if ((rec->record = SMDBXI_MM_ALLOC(dfctx->mem,
					   dbf->size * hdr->blk_size)) == NULL)
		return -1;
	rec->key.data = (char *) rec->record + sizeof(struct smdb_db_rstorage);
	rec->key.size = stg->ksize;
	rec->data.data = (char *) rec->record + sizeof(struct smdb_db_rstorage) +
		stg->ksize;
	rec->data.size = stg->dsize;

	smdb_memcpy(rec->record, stg, hdr->blk_size);

	for (i = 1; i < dbf->size; i++) {
		if ((bcn = smdb_cf_get_block(dfctx->cfctx, dbf->blkno + i,
					     0)) == NULL) {
			smdb_dbf_free_record(dfctx, rec);
			return -1;
		}

		smdb_memcpy((char *) rec->record + i * hdr->blk_size,
			    smdb_bc_get_block_data(bcn), hdr->blk_size);

		smdb_cf_release_block(dfctx->cfctx, bcn);
	}

	return 0;
}

static int smdb_dbf_match_key(struct smdb_dbfile_ctx *dfctx,
			      struct smdb_db_header *hdr, struct smdb_db_file const *dbf,
			      struct smdb_db_ckey const *key,
			      struct smdb_db_cdata const *data,
			      struct smdb_db_record *rec)
{
	smdb_u32 csize;
	struct smdb_bc_node *bcn;
	struct smdb_db_rstorage *stg;

	if ((bcn = smdb_cf_get_block(dfctx->cfctx, dbf->blkno, 0)) == NULL)
		return -1;
	stg = (struct smdb_db_rstorage *) smdb_bc_get_block_data(bcn);

	/*
	 * A few fast checks before loading the record.
	 * If the keys length does not match, we are not at home.
	 */
	if (stg->ksize != key->size ||
	    (data != NULL && stg->dsize != data->size)) {
		smdb_cf_release_block(dfctx->cfctx, bcn);
		return 0;
	}
	/*
	 * Compare the initial key area fitting the first block, and give
	 * up if not matching.
	 */
	csize = hdr->blk_size - sizeof(struct smdb_db_rstorage);
	if (csize > (smdb_u32) key->size)
		csize = (smdb_u32) key->size;
	if (smdb_memcmp(key->data,
			(char *) stg + sizeof(struct smdb_db_rstorage),
			csize) != 0) {
		smdb_cf_release_block(dfctx->cfctx, bcn);
		return 0;
	}
	/*
	 * Alloc and load the whole record at this point.
	 */
	if (smdb_dbf_rec_alloc(dfctx, hdr, dbf, stg, rec) < 0) {
		smdb_cf_release_block(dfctx->cfctx, bcn);
		return -1;
	}
	smdb_cf_release_block(dfctx->cfctx, bcn);

	/*
	 * If the whole keys do not match, free the newly allocated record
	 * and return a no-match.
	 */
	if (smdb_memcmp(key->data, rec->key.data, key->size) != 0 ||
	    (data != NULL && smdb_memcmp(data->data, rec->data.data,
					 data->size) != 0)) {
		smdb_dbf_free_record(dfctx, rec);
		return 0;
	}

	return 1;
}

static int smdb_dbf_file_available(struct smdb_db_file const *dbf)
{
	return dbf->size == 0 || dbf->blkno == 0;
}

static int smdb_dbf_file_empty(struct smdb_db_file const *dbf)
{
	return dbf->size == 0;
}

static int smdb_dbf_file_deleted(struct smdb_db_file const *dbf)
{
	return dbf->size != 0 && dbf->blkno == 0;
}

static void smdb_dbf_file_set_deleted(struct smdb_db_file *dbf)
{
	dbf->blkno = 0;
}

static int smdb_dbf_delete_file(struct smdb_cfile_ctx *cfctx,
				struct smdb_db_file *dbf)
{
	if (smdb_dbf_bfree(cfctx, dbf->blkno, dbf->size) < 0)
		return -1;
	smdb_dbf_file_set_deleted(dbf);

	return 0;
}

int smdb_dbf_create_table(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
			  unsigned int tblsize)
{
	smdb_u32 tbl_x_blk, tbl_nblocks;
	struct smdb_db_file hash;
	struct smdb_db_env env;

	if (smdb_dbf_get_env(dfctx, tblid, 1, &env) < 0)
		return -1;
	/*
	 * Fail if exists!
	 */
	if (env.tbl->hash.size != 0) {
		smdb_dbf_release_env(dfctx, &env);
		return -1;
	}

	tbl_x_blk = env.hdr->blk_size / sizeof(struct smdb_db_table);
	tbl_nblocks = (smdb_u32) tblsize / tbl_x_blk + 1;

	if (smdb_dbf_alloc_file(dfctx->cfctx, tbl_nblocks, &hash) < 0) {
		smdb_dbf_release_env(dfctx, &env);
		return -1;
	}
	/*
	 * Properly init/zero the newly allocated hash.
	 */
	if (smdb_cf_zero(dfctx->cfctx, hash.blkno, hash.size) < 0) {
		smdb_dbf_bfree(dfctx->cfctx, hash.blkno, hash.size);
		smdb_dbf_release_env(dfctx, &env);
		return -1;
	}

	MZERO(*env.tbl);
	env.tbl->hash = hash;

	smdb_cf_set_block_dirty(dfctx->cfctx, env.tbcn);
	smdb_cf_set_block_dirty(dfctx->cfctx, env.mbcn);

	smdb_dbf_release_env(dfctx, &env);

	return 0;
}

int smdb_dbf_free_table(struct smdb_dbfile_ctx *dfctx, unsigned int tblid)
{
	smdb_u32 i, blkno, dbf_x_blk;
	struct smdb_bc_node *bcn;
	struct smdb_db_file *dbf;
	struct smdb_db_env env;

	if (smdb_dbf_get_env(dfctx, tblid, 1, &env) < 0)
		return -1;
	/*
	 * Fail if does not exists!
	 */
	if (env.tbl->hash.size == 0) {
		smdb_dbf_release_env(dfctx, &env);
		return -1;
	}

	/*
	 * Loop through every hash entry and release the allocated storage.
	 */
	dbf_x_blk = env.hdr->blk_size / sizeof(struct smdb_db_file);
	for (blkno = 0; blkno < env.tbl->hash.size; blkno++) {
		if ((bcn = smdb_cf_get_block(dfctx->cfctx, env.tbl->hash.blkno +
					     blkno, 1)) == NULL) {
			smdb_dbf_release_env(dfctx, &env);
			return -1;
		}
		dbf = (struct smdb_db_file *) smdb_bc_get_block_data(bcn);

		for (i = 0; i < dbf_x_blk; i++, dbf++) {
			/*
			 * If this is an empty slot, or we find a deleted item,
			 * we need to continue to scan the chain since we want
			 * to free them all.
			 */
			if (smdb_dbf_file_empty(dbf) || smdb_dbf_file_deleted(dbf))
				continue;

			if (smdb_dbf_bfree(dfctx->cfctx, dbf->blkno,
					   dbf->size) < 0) {
				smdb_dbf_release_env(dfctx, &env);
				return -1;
			}
			smdb_dbf_file_set_deleted(dbf);
		}

		smdb_cf_set_block_dirty(dfctx->cfctx, bcn);
		smdb_cf_release_block(dfctx->cfctx, bcn);
	}

	/*
	 * Release the space allocated for the hash table itself.
	 */
	if (smdb_dbf_bfree(dfctx->cfctx, env.tbl->hash.blkno,
			   env.tbl->hash.size) < 0) {
		smdb_dbf_release_env(dfctx, &env);
		return -1;
	}

	MZERO(*env.tbl);

	smdb_cf_set_block_dirty(dfctx->cfctx, env.tbcn);
	smdb_cf_set_block_dirty(dfctx->cfctx, env.mbcn);

	smdb_dbf_release_env(dfctx, &env);

	return 0;
}

static int smdb_dbf_find_key(struct smdb_dbfile_ctx *dfctx,
			     struct smdb_db_env *env,
			     struct smdb_db_ckey const *key,
			     struct smdb_db_cdata const *data,
			     struct smdb_db_record *rec, struct smdb_db_kenum *ken,
			     int erase)
{
	int match_res;
	smdb_u32 i, idx, dbf_x_blk, istart, blkno;
	struct smdb_bc_node *bcn;
	struct smdb_db_file *dbf;

	dbf_x_blk = env->hdr->blk_size / sizeof(struct smdb_db_file);
	for (idx = ken->idx;;) {
		blkno = idx / dbf_x_blk;
		istart = idx % dbf_x_blk;
		if ((bcn = smdb_cf_get_block(dfctx->cfctx, env->tbl->hash.blkno +
					     blkno, erase != 0)) == NULL)
			return -1;
		dbf = (struct smdb_db_file *) smdb_bc_get_block_data(bcn);

		for (i = istart, dbf += istart; i < dbf_x_blk; i++, dbf++) {
			/*
			 * If this is an empty slot, the chain ends here and
			 * the key was not found.
			 */
			if (smdb_dbf_file_empty(dbf)) {
				smdb_cf_release_block(dfctx->cfctx, bcn);
				return 0;
			}
			/*
			 * When we find a deleted item, we need to continue to
			 * scan the chain.
			 */
			if (smdb_dbf_file_deleted(dbf))
				continue;
			/*
			 * Try to match the current item.
			 */
			if ((match_res = smdb_dbf_match_key(dfctx, env->hdr, dbf,
							    key, data, rec)) < 0) {
				smdb_cf_release_block(dfctx->cfctx, bcn);
				return match_res;
			}
			/*
			 * No luck, try the next one ...
			 */
			if (match_res == 0)
				continue;

			/*
			 * At this point we found it.
			 */
			if (erase) {
				if (smdb_dbf_delete_file(dfctx->cfctx, dbf) < 0)
					match_res = -1;
				else
					smdb_cf_set_block_dirty(dfctx->cfctx, bcn);
			}
			smdb_cf_release_block(dfctx->cfctx, bcn);
			ken->idx = idx + i - istart;
			return match_res;
		}

		smdb_cf_release_block(dfctx->cfctx, bcn);
		idx += dbf_x_blk - istart;
		if (idx == ken->hsize)
			idx = 0;
	}

	return 0;
}

static int smdb_dbf_load_rec(struct smdb_dbfile_ctx *dfctx,
			     struct smdb_db_header *hdr,
			     struct smdb_db_file const *dbf,
			     struct smdb_db_record *rec)
{
	int error;
	struct smdb_bc_node *bcn;
	struct smdb_db_rstorage *stg;

	if ((bcn = smdb_cf_get_block(dfctx->cfctx, dbf->blkno, 0)) == NULL)
		return -1;
	stg = (struct smdb_db_rstorage *) smdb_bc_get_block_data(bcn);

	error = smdb_dbf_rec_alloc(dfctx, hdr, dbf, stg, rec);

	smdb_cf_release_block(dfctx->cfctx, bcn);

	return error;
}

static int smdb_dbf_enum(struct smdb_dbfile_ctx *dfctx, struct smdb_db_env *env,
			 struct smdb_db_record *rec, struct smdb_db_kenum *ken)
{
	smdb_u32 i, idx, dbf_x_blk, istart, blkno;
	struct smdb_bc_node *bcn;
	struct smdb_db_file *dbf;

	dbf_x_blk = env->hdr->blk_size / sizeof(struct smdb_db_file);
	for (idx = ken->idx; idx < ken->hsize;) {
		blkno = idx / dbf_x_blk;
		istart = idx % dbf_x_blk;
		if ((bcn = smdb_cf_get_block(dfctx->cfctx, env->tbl->hash.blkno +
					     blkno, 0)) == NULL)
			return -1;
		dbf = (struct smdb_db_file *) smdb_bc_get_block_data(bcn);

		for (i = istart, dbf += istart; i < dbf_x_blk; i++, dbf++) {
			/*
			 * If this is an empty slot, or we find a deleted item,
			 * we need to continue to scan the chain since we want
			 * to find them all.
			 */
			if (smdb_dbf_file_empty(dbf) || smdb_dbf_file_deleted(dbf))
				continue;

			/*
			 * We got a valid record, load this up.
			 */
			if (smdb_dbf_load_rec(dfctx, env->hdr, dbf, rec) < 0) {
				smdb_cf_release_block(dfctx->cfctx, bcn);
				return -1;
			}
			smdb_cf_release_block(dfctx->cfctx, bcn);
			ken->idx = idx + i - istart;
			return 1;
		}

		smdb_cf_release_block(dfctx->cfctx, bcn);
		idx += dbf_x_blk - istart;
	}
	ken->idx = idx;

	return 0;
}

int smdb_dbf_begin(struct smdb_dbfile_ctx *dfctx)
{
	if (smdb_jf_begin(dfctx->jfctx) < 0)
		return -1;

	return 0;
}

int smdb_dbf_end(struct smdb_dbfile_ctx *dfctx)
{
	if (smdb_cf_sync(dfctx->cfctx) < 0 ||
	    smdb_jf_end(dfctx->jfctx) < 0)
		return -1;

	return 0;
}

int smdb_dbf_rollback(struct smdb_dbfile_ctx *dfctx)
{
	if (smdb_jf_rollback(dfctx->jfctx) < 0)
		return -1;

	return 0;
}

int smdb_dbf_get(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		 struct smdb_db_ckey const *key, struct smdb_db_record *rec,
		 struct smdb_db_kenum *ken)
{
	int match_res;
	struct smdb_db_env env;

	if (smdb_dbf_grab_env(dfctx, tblid, 0, &env) < 0)
		return -1;

	MZERO(*ken);
	ken->tblid = (smdb_u32) tblid;
	ken->hashv = (smdb_u32) smdb_get_hash(key->data, key->size, SMDB_HASHV_INIT);
	ken->hsize = (env.tbl->hash.size * env.hdr->blk_size) / sizeof(struct smdb_db_file);
	ken->idx = ken->hashv % ken->hsize;

	if ((match_res = smdb_dbf_find_key(dfctx, &env, key, NULL,
					   rec, ken, 0)) > 0)
		ken->idx++;

	smdb_dbf_release_env(dfctx, &env);

	return match_res;
}

int smdb_dbf_get_next(struct smdb_dbfile_ctx *dfctx, struct smdb_db_ckey const *key,
		      struct smdb_db_record *rec, struct smdb_db_kenum *ken)
{
	int match_res;
	struct smdb_db_env env;

	if (smdb_dbf_grab_env(dfctx, ken->tblid, 0, &env) < 0)
		return -1;

	if ((match_res = smdb_dbf_find_key(dfctx, &env, key, NULL,
					   rec, ken, 0)) > 0)
		ken->idx++;

	smdb_dbf_release_env(dfctx, &env);

	return match_res;
}

int smdb_dbf_first(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		   struct smdb_db_record *rec, struct smdb_db_kenum *ken)
{
	int match_res;
	struct smdb_db_env env;

	if (smdb_dbf_grab_env(dfctx, tblid, 0, &env) < 0)
		return -1;

	MZERO(*ken);
	ken->tblid = (smdb_u32) tblid;
	ken->hsize = (env.tbl->hash.size * env.hdr->blk_size) /
		sizeof(struct smdb_db_file);

	if ((match_res = smdb_dbf_enum(dfctx, &env, rec, ken)) > 0)
		ken->idx++;

	smdb_dbf_release_env(dfctx, &env);

	return match_res;
}

int smdb_dbf_next(struct smdb_dbfile_ctx *dfctx, struct smdb_db_record *rec,
		  struct smdb_db_kenum *ken)
{
	int match_res;
	struct smdb_db_env env;

	if (smdb_dbf_grab_env(dfctx, ken->tblid, 0, &env) < 0)
		return -1;

	if ((match_res = smdb_dbf_enum(dfctx, &env, rec, ken)) > 0)
		ken->idx++;

	smdb_dbf_release_env(dfctx, &env);

	return match_res;
}

void smdb_dbf_free_record(struct smdb_dbfile_ctx *dfctx,
			  struct smdb_db_record *rec)
{
	if (rec != NULL) {
		/*
		 * This may be called multiple times over the same record,
		 * so nullify the record data pointer afterward.
		 * The memory interface is supposed to be handling the
		 * NULL pointer frees just fine.
		 */
		SMDBXI_MM_FREE(dfctx->mem, rec->record);
		rec->record = NULL;
	}
}

static int smdb_dbf_falloc_rec(struct smdb_dbfile_ctx *dfctx,
			       struct smdb_db_env *env,
			       struct smdb_db_ckey const *key,
			       struct smdb_db_cdata *data,
			       smdb_u32 hashv, struct smdb_db_file *dbf)
{
	smdb_u32 rec_blocks;
	smdb_u64 rsize;
	smdb_offset_t offset;
	struct smdb_bc_node *bcn;
	struct smdb_db_rstorage *stg;

	/*
	 * Calculate the space for the new record, and allocate the necessary
	 * number of blocks on file.
	 */
	rsize = (smdb_u64) sizeof(struct smdb_db_rstorage) + key->size + data->size;
	rec_blocks = (smdb_u32) (rsize / env->hdr->blk_size);
	if ((rsize % env->hdr->blk_size) != 0)
		rec_blocks++;
	if (smdb_dbf_alloc_file(dfctx->cfctx, rec_blocks, dbf) < 0)
		return -1;

	/*
	 * Setup the record storage header.
	 */
	if ((bcn = smdb_cf_get_block(dfctx->cfctx, dbf->blkno,
				     1)) == NULL) {
		smdb_dbf_bfree(dfctx->cfctx, dbf->blkno, dbf->size);
		return -1;
	}
	stg = (struct smdb_db_rstorage *) smdb_bc_get_block_data(bcn);
	stg->hashv = hashv;
	stg->ksize = key->size;
	stg->dsize = data->size;

	smdb_cf_set_block_dirty(dfctx->cfctx, bcn);
	smdb_cf_release_block(dfctx->cfctx, bcn);
	/*
	 * Write key and data into the new space. Key and data follow the record
	 * storage header.
	 */
	offset = (smdb_offset_t) dbf->blkno * env->hdr->blk_size +
		sizeof(struct smdb_db_rstorage);
	if (smdb_cf_write(dfctx->cfctx, offset, key->data,
			  key->size) != (int) key->size ||
	    smdb_cf_write(dfctx->cfctx, offset + key->size, data->data,
			  data->size) != (int) data->size) {
		smdb_dbf_bfree(dfctx->cfctx, dbf->blkno, dbf->size);
		return -1;
	}

	return 0;
}

static int smdb_dbf_put_key(struct smdb_dbfile_ctx *dfctx,
			    struct smdb_db_env *env, smdb_u32 hashv,
			    smdb_u32 hsize, struct smdb_db_ckey const *key,
			    struct smdb_db_cdata *data)
{
	smdb_u32 i, idx, dbf_x_blk, istart, blkno;
	struct smdb_bc_node *bcn;
	struct smdb_db_file *dbf;

	idx = hashv % hsize;
	dbf_x_blk = env->hdr->blk_size / sizeof(struct smdb_db_file);
	for (;;) {
		blkno = idx / dbf_x_blk;
		istart = idx % dbf_x_blk;
		if ((bcn = smdb_cf_get_block(dfctx->cfctx, env->tbl->hash.blkno +
					     blkno, 1)) == NULL)
			return -1;
		dbf = (struct smdb_db_file *) smdb_bc_get_block_data(bcn);

		for (i = istart, dbf += istart; i < dbf_x_blk; i++, dbf++) {
			if (smdb_dbf_file_available(dbf)) {
				if (smdb_dbf_falloc_rec(dfctx, env, key, data,
							hashv, dbf) < 0) {
					smdb_cf_release_block(dfctx->cfctx, bcn);
					return -1;
				}
				smdb_cf_set_block_dirty(dfctx->cfctx, bcn);
				smdb_cf_release_block(dfctx->cfctx, bcn);

				return 1;
			}
		}

		smdb_cf_release_block(dfctx->cfctx, bcn);
		idx += dbf_x_blk - istart;
		if (idx == hsize)
			idx = 0;
	}

	return 0;
}

static int smdb_dbf_get_rec_stg(struct smdb_cfile_ctx *cfctx,
				struct smdb_db_file const *dbf,
				struct smdb_db_rstorage *rstg)
{
	struct smdb_bc_node *bcn;
	struct smdb_db_rstorage *stg;

	if ((bcn = smdb_cf_get_block(cfctx, dbf->blkno, 0)) == NULL)
		return -1;
	stg = (struct smdb_db_rstorage *) smdb_bc_get_block_data(bcn);

	*rstg = *stg;

	smdb_cf_release_block(cfctx, bcn);

	return 0;
}

static int smdb_dbf_set_hash_ent(struct smdb_cfile_ctx *cfctx,
				 struct smdb_db_env *env, smdb_u32 hashv,
				 smdb_u32 hsize, struct smdb_db_file const *rdbf)
{
	smdb_u32 i, idx, dbf_x_blk, istart, blkno;
	struct smdb_bc_node *bcn;
	struct smdb_db_file *dbf;

	idx = hashv % hsize;
	dbf_x_blk = env->hdr->blk_size / sizeof(struct smdb_db_file);
	for (;;) {
		blkno = idx / dbf_x_blk;
		istart = idx % dbf_x_blk;
		if ((bcn = smdb_cf_get_block(cfctx, env->tbl->hash.blkno +
					     blkno, 1)) == NULL)
			return -1;
		dbf = (struct smdb_db_file *) smdb_bc_get_block_data(bcn);

		for (i = istart, dbf += istart; i < dbf_x_blk; i++, dbf++) {
			if (smdb_dbf_file_available(dbf)) {
				*dbf = *rdbf;

				smdb_cf_set_block_dirty(cfctx, bcn);
				smdb_cf_release_block(cfctx, bcn);

				return 1;
			}
		}

		smdb_cf_release_block(cfctx, bcn);
		idx += dbf_x_blk - istart;
		if (idx == hsize)
			idx = 0;
	}

	return 0;
}

static int smdb_dbf_hash_grow(struct smdb_cfile_ctx *cfctx,
			      struct smdb_db_env *env)
{
	smdb_u32 i, blkno, hash_blocks, dbf_x_blk, hsize;
	struct smdb_bc_node *bcn;
	struct smdb_db_file *dbf;
	struct smdb_db_file hash, ohash;
	struct smdb_db_rstorage rstg;

	/*
	 * Double the size of the current hash, and alloc a new space
	 * for it.
	 */
	hash_blocks = env->tbl->hash.size * 2;
	if (smdb_dbf_alloc_file(cfctx, hash_blocks, &hash) < 0)
		return -1;
	/*
	 * Properly init/zero the newly allocated hash.
	 */
	if (smdb_cf_zero(cfctx, hash.blkno, hash.size) < 0) {
		smdb_dbf_bfree(cfctx, hash.blkno, hash.size);
		return -1;
	}

	ohash = env->tbl->hash;
	env->tbl->hash = hash;

	/*
	 * Loop through every old hash entry, and re-insert then into
	 * the new hash position.  We cannot just copy over here, since
	 * the new hash size generates different indexes for the same
	 * hash values.
	 */
	hsize = (hash_blocks * env->hdr->blk_size) / sizeof(struct smdb_db_file);
	dbf_x_blk = env->hdr->blk_size / sizeof(struct smdb_db_file);
	for (blkno = 0; blkno < ohash.size; blkno++) {
		if ((bcn = smdb_cf_get_block(cfctx, ohash.blkno +
					     blkno, 0)) == NULL) {
			smdb_dbf_bfree(cfctx, ohash.blkno, ohash.size);
			return -1;
		}
		dbf = (struct smdb_db_file *) smdb_bc_get_block_data(bcn);

		for (i = 0; i < dbf_x_blk; i++, dbf++) {
			if (dbf->size == 0 || dbf->blkno == 0)
				continue;

			if (smdb_dbf_get_rec_stg(cfctx, dbf, &rstg) < 0 ||
			    smdb_dbf_set_hash_ent(cfctx, env, rstg.hashv,
						  hsize, dbf) < 0) {
				smdb_cf_release_block(cfctx, bcn);
				smdb_dbf_bfree(cfctx, ohash.blkno, ohash.size);
				return -1;
			}
		}

		smdb_cf_release_block(cfctx, bcn);
	}

	smdb_dbf_bfree(cfctx, ohash.blkno, ohash.size);

	return 0;
}

int smdb_dbf_put(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		 struct smdb_db_ckey const *key, struct smdb_db_cdata *data)
{
	int put_res;
	smdb_u32 hashv, hsize;
	struct smdb_db_env env;

	if (smdb_dbf_grab_env(dfctx, tblid, 1, &env) < 0)
		return -1;

	hashv = (smdb_u32) smdb_get_hash(key->data, key->size, SMDB_HASHV_INIT);
	hsize = (env.tbl->hash.size * env.hdr->blk_size) / sizeof(struct smdb_db_file);

	/*
	 * We need to make sure that there is enough free space inside the
	 * hash, so that chain walks can properly terminate.  If we do not
	 * do that, we end up looping endlessly.
	 */
	if (5 * hsize < 6 * env.tbl->num_recs) {
		if (smdb_dbf_hash_grow(dfctx->cfctx, &env) < 0) {
			smdb_dbf_release_env(dfctx, &env);
			return -1;
		}
		smdb_cf_set_block_dirty(dfctx->cfctx, env.tbcn);
		smdb_cf_set_block_dirty(dfctx->cfctx, env.mbcn);
		hsize = (env.tbl->hash.size * env.hdr->blk_size) / sizeof(struct smdb_db_file);
	}
	/*
	 * Store the key+data couple inside the DB.  We allow multiple storage
	 * of the same key, so the user will have to take care of it, if he
	 * needs unicity.
	 */
	if ((put_res = smdb_dbf_put_key(dfctx, &env, hashv, hsize, key,
					data)) > 0) {
		env.tbl->num_recs++;
		smdb_cf_set_block_dirty(dfctx->cfctx, env.tbcn);
	}

	smdb_dbf_release_env(dfctx, &env);

	return put_res;
}

int smdb_dbf_erase(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		   struct smdb_db_ckey const *key,
		   struct smdb_db_cdata const *data)
{
	int match_res;
	struct smdb_db_env env;
	struct smdb_db_kenum ken;
	struct smdb_db_record rec;

	if (smdb_dbf_grab_env(dfctx, tblid, 1, &env) < 0)
		return -1;

	MZERO(ken);
	ken.hashv = (smdb_u32) smdb_get_hash(key->data, key->size, SMDB_HASHV_INIT);
	ken.hsize = (env.tbl->hash.size * env.hdr->blk_size) / sizeof(struct smdb_db_file);
	ken.idx = ken.hashv % ken.hsize;

	if ((match_res = smdb_dbf_find_key(dfctx, &env, key, data, &rec,
					   &ken, 1)) > 0) {
		smdb_dbf_free_record(dfctx, &rec);
		env.tbl->num_recs--;
		smdb_cf_set_block_dirty(dfctx->cfctx, env.tbcn);
	}

	smdb_dbf_release_env(dfctx, &env);

	return match_res;
}

