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

#define SMDB_BHASH_MINSIZE 512
#define SMDB_NO_OFFSET 0xffffffff
#define SMDB_JFILE_MAGIC "SMDBJF01"

struct smdb_jfile_trailer {
	smdb_u8 magic[8];
	smdb_offset_t offset;
};

static char *smdb_jf_journal_path(struct smdbxi_mem *mem, char const *path)
{
	int psize, size;
	char *jpath;
	static char const * const jext = ".journal";

	psize = smdb_strlen(path);
	size = psize + smdb_strlen(jext) + 1;
	if ((jpath = (char *) SMDBXI_MM_ALLOC(mem, size)) == NULL)
		return NULL;
	smdb_memcpy(jpath, path, psize);
	smdb_memcpy(jpath + psize, jext, size - psize);

	return jpath;
}

static unsigned long smdb_jf_offset_index(smdb_u32 offset, unsigned long bhbits,
					  unsigned long bhmask)
{
	offset ^= offset >> bhbits;

	return (unsigned long) offset & bhmask;
}

static void smdb_jf_reset_blkhash(struct smdb_jfile_ctx *jfctx)
{
	smdb_memset(jfctx->bhash, 0xff,
		    (jfctx->bhmask + 1) * sizeof(smdb_offset_t));
	jfctx->bcount = 0;
	jfctx->bdeleted = 0;
	jfctx->bfree = jfctx->bhmask + 1;
}

static int smdb_jf_alloc_blkhash(struct smdb_jfile_ctx *jfctx)
{
	unsigned long i, n, hsize;
	smdb_offset_t fsize;

	if ((fsize = SMDBXI_FL_SEEK(jfctx->bfile, 0, SMDBXI_FL_SEEKEND)) < 0)
		return -1;
	hsize = (unsigned long) (fsize / jfctx->blk_size);
	if (hsize < SMDB_BHASH_MINSIZE)
		hsize = SMDB_BHASH_MINSIZE;
	for (i = 1, n = 0; i < hsize; i <<= 1, n++);

	if ((jfctx->bhash = (struct smdb_jbhash_node *)
	     SMDBXI_MM_ALLOC(jfctx->mem, i * sizeof(struct smdb_jbhash_node))) == NULL)
		return -1;
	smdb_memset(jfctx->bhash, 0xff, i * sizeof(struct smdb_jbhash_node));

	jfctx->bhmask = i - 1;
	jfctx->bhbits = n;
	jfctx->bfree = i;

	return 0;
}

static int smdb_jf_resize_blkhash(struct smdb_jfile_ctx *jfctx,
				  unsigned long bhbits)
{
	unsigned long i, idx, bhmask;
	struct smdb_jbhash_node *bhash, *obhash;

	bhmask = (1UL << bhbits) - 1;
	if ((bhash = (struct smdb_jbhash_node *)
	     SMDBXI_MM_ALLOC(jfctx->mem, (bhmask + 1) *
			     sizeof(struct smdb_jbhash_node))) == NULL)
		return -1;
	smdb_memset(bhash, 0xff, (bhmask + 1) * sizeof(struct smdb_jbhash_node));

	/*
	 * Reassign the hashed blocks according to the new hash size.
	 * Yes, we might end up forgetting about some block we could use
	 * due to a truncate/trim-blocks, but it should not be a problem
	 * worth dealing with.
	 */
	for (i = 0, obhash = jfctx->bhash; i <= jfctx->bhmask; i++) {
		if (obhash[i].joffset == SMDB_NO_OFFSET ||
		    obhash[i].offset == SMDB_NO_OFFSET)
			continue;

		idx = smdb_jf_offset_index(obhash[i].offset, bhbits, bhmask);
		for (;;) {
			if (bhash[idx].joffset == SMDB_NO_OFFSET ||
			    bhash[idx].offset == SMDB_NO_OFFSET)
				break;
			if (idx < bhmask)
				idx++;
			else
				idx = 0;
		}
		bhash[idx] = obhash[i];
	}

	SMDBXI_MM_FREE(jfctx->mem, jfctx->bhash);
	jfctx->bhash = bhash;
	jfctx->bhmask = bhmask;
	jfctx->bhbits = bhbits;
	jfctx->bfree = bhmask + 1;
	jfctx->bdeleted = 0;

	return 0;
}

static int smdb_jf_fetch_block(struct smdb_jfile_ctx *jfctx, smdb_offset_t foffset,
			       smdb_offset_t *pjoffset)
{
	unsigned long idx;
	smdb_u32 offset;
	struct smdb_jbhash_node *bhash;

	offset = (smdb_u32) (foffset / jfctx->blk_size);
	idx = smdb_jf_offset_index(offset, jfctx->bhbits, jfctx->bhmask);
	for (bhash = jfctx->bhash;;) {
		if (bhash[idx].joffset == SMDB_NO_OFFSET)
			break;
		if (bhash[idx].offset == offset) {
			*pjoffset = (smdb_offset_t) bhash[idx].joffset *
				jfctx->blk_size;
			return 1;
		}
		if (idx < jfctx->bhmask)
			idx++;
		else
			idx = 0;
	}

	return 0;
}

static int smdb_jf_alloc_block(struct smdb_jfile_ctx *jfctx, smdb_offset_t foffset,
			       smdb_offset_t *pjoffset)
{
	unsigned long i, n, idx;
	smdb_u32 offset;
	smdb_offset_t joffset;
	struct smdb_jbhash_node *bhash;

	/*
	 * Check if we are running out of space inside our hash table.
	 */
	if (6 * jfctx->bcount > 5 * jfctx->bhmask &&
	    smdb_jf_resize_blkhash(jfctx, jfctx->bhbits + 1) < 0)
		return -1;
	/*
	 * Find a usable slot to where to store the new entry.
	 */
	offset = (smdb_u32) (foffset / jfctx->blk_size);
	idx = smdb_jf_offset_index(offset, jfctx->bhbits, jfctx->bhmask);
	for (bhash = jfctx->bhash;;) {
		if (bhash[idx].joffset == SMDB_NO_OFFSET ||
		    bhash[idx].offset == SMDB_NO_OFFSET)
			break;
		if (idx < jfctx->bhmask)
			idx++;
		else
			idx = 0;
	}
	joffset = -1;
	if (jfctx->bfree <= jfctx->bhmask) {
		/*
		 * Try to fetch from the freed blocks previously allocated.
		 */
		for (i = jfctx->bfree, n = jfctx->bhmask; i <= n; i++)
			if (bhash[i].offset == SMDB_NO_OFFSET &&
			    bhash[i].joffset != SMDB_NO_OFFSET)
				break;
		jfctx->bfree = i;
		if (i <= n) {
			joffset = (smdb_offset_t) bhash[i].joffset * jfctx->blk_size;
			jfctx->bdeleted--;
		}
	}
	/*
	 * If no previously allocated blocks were free, alloc from the
	 * end of file.
	 */
	if (joffset == -1 &&
	    (joffset = SMDBXI_FL_SEEK(jfctx->jfile, 0, SMDBXI_FL_SEEKEND)) < 0)
		return -1;

	bhash[idx].offset = offset;
	bhash[idx].joffset = (smdb_u32) (joffset / jfctx->blk_size);

	jfctx->bcount++;

	*pjoffset = joffset;

	return 0;
}

static int smdb_jf_want_block(struct smdb_jfile_ctx *jfctx, smdb_offset_t foffset,
			      smdb_offset_t *pjoffset)
{
	if (smdb_jf_fetch_block(jfctx, foffset, pjoffset) > 0)
		return 0;

	return smdb_jf_alloc_block(jfctx, foffset, pjoffset);
}

static int smdb_jf_trim_blocks(struct smdb_jfile_ctx *jfctx, smdb_offset_t fsize)
{
	unsigned long i;
	smdb_u32 size;
	struct smdb_jbhash_node *bhash;

	/*
	 * Drop all the blocks which fall above the specified size. We mark
	 * those freed blocks in such a way that we can re-use them.
	 * That is, ->offset set to SMDB_NO_OFFSET, and ->joffset left
	 * pointing to the freed block.
	 */
	size = (smdb_u32) (fsize / jfctx->blk_size);
	for (i = 0, bhash = jfctx->bhash; i <= jfctx->bhmask; i++) {
		if (bhash[i].offset == SMDB_NO_OFFSET ||
		    bhash[i].joffset == SMDB_NO_OFFSET)
			continue;
		if (bhash[i].offset >= size) {
			bhash[i].offset = SMDB_NO_OFFSET;
			if (i < jfctx->bfree)
				jfctx->bfree = i;
			jfctx->bcount--;
			jfctx->bdeleted++;
		}
	}

	return 0;
}

static int smdb_jf_finish_journal(struct smdb_jfile_ctx *jfctx)
{
	unsigned long i;
	smdb_offset_t offset;
	struct smdb_jbhash_node *bhash;
	struct smdb_jfile_trailer jft;

	/*
	 * Write the block mapping table at the end of the file.
	 */
	if ((offset = SMDBXI_FL_SEEK(jfctx->jfile, 0, SMDBXI_FL_SEEKEND)) < 0)
		return -1;

	for (i = 0, bhash = jfctx->bhash; i <= jfctx->bhmask; i++) {
		/*
		 * Write only active (not freed) blocks.
		 */
		if (bhash[i].joffset == SMDB_NO_OFFSET ||
		    bhash[i].offset == SMDB_NO_OFFSET)
			continue;
		if (SMDBXI_FL_WRITE(jfctx->jfile, &bhash[i],
				    sizeof(struct smdb_jbhash_node)) !=
		    sizeof(struct smdb_jbhash_node))
			return -1;
	}
	/*
	 * Sync all blocks before (write barrier) ...
	 */
	if (SMDBXI_FL_SYNC(jfctx->jfile) < 0)
		return -1;

	/*
	 * Then write trailer and sync again.  We do this since we do not
	 * want that a re-ordering on the block I/O layer make the trailer
	 * visible while the referenced block are still not on disk.
	 */
	MZERO(jft);
	smdb_memcpy(jft.magic, SMDB_JFILE_MAGIC, sizeof(jft.magic));
	jft.offset = offset;
	if (SMDBXI_FL_WRITE(jfctx->jfile, &jft, sizeof(jft)) != sizeof(jft) ||
	    SMDBXI_FL_SYNC(jfctx->jfile) < 0)
		return -1;

	smdb_jf_reset_blkhash(jfctx);

	return 0;
}

static int smdb_jf_truncate_journal(struct smdb_jfile_ctx *jfctx)
{
	SMDBXI_FL_SEEK(jfctx->jfile, 0, SMDBXI_FL_SEEKSET);
	if (SMDBXI_FL_TRUNCATE(jfctx->jfile, 0) < 0 ||
	    SMDBXI_FL_SYNC(jfctx->jfile) < 0)
		return -1;

	return 0;
}

static int smdb_jf_play_journal(struct smdb_jfile_ctx *jfctx)
{
	smdb_offset_t toffset, foffset, offset, joffset;
	void *blkbuf;
	struct smdb_jfile_trailer jft;
	struct smdb_jbhash_node bhn;

	/*
	 * Check to see if the journal file trailer is there and is valid.
	 */
	if ((toffset = SMDBXI_FL_SEEK(jfctx->jfile,
				      - (long) sizeof(struct smdb_jfile_trailer),
				      SMDBXI_FL_SEEKEND)) < 0 ||
	    SMDBXI_FL_READ(jfctx->jfile, &jft, sizeof(jft)) != sizeof(jft) ||
	    smdb_memcmp(jft.magic, SMDB_JFILE_MAGIC, sizeof(jft.magic)) != 0) {
		/*
		 * If we are dealing with a broken journal, just nuke it and
		 * forget it.
		 */
		return smdb_jf_truncate_journal(jfctx);
	}
	if ((blkbuf = SMDBXI_MM_ALLOC(jfctx->mem, jfctx->blk_size)) == NULL)
		return -1;

	/*
	 * Go through every table entry, and apply the cached blocks into
	 * the DB file.
	 */
	for (foffset = jft.offset; foffset < toffset; foffset += sizeof(bhn)) {
		/*
		 * Seek and read table entry.
		 */
		if (smdb_off_read(jfctx->jfile, foffset, &bhn,
				  sizeof(bhn)) != sizeof(bhn)) {
			SMDBXI_MM_FREE(jfctx->mem, blkbuf);
			return -1;
		}

		offset = (smdb_offset_t) bhn.offset * jfctx->blk_size;
		joffset = (smdb_offset_t) bhn.joffset * jfctx->blk_size;

		/*
		 * Read block from journal file ...
		 */
		if (smdb_off_read(jfctx->jfile, joffset, blkbuf,
				  jfctx->blk_size) != (int) jfctx->blk_size) {
			SMDBXI_MM_FREE(jfctx->mem, blkbuf);
			return -1;
		}
		/*
		 * ... and write it to the underlying DB file.
		 */
		if (smdb_off_write(jfctx->bfile, offset, blkbuf,
				   jfctx->blk_size) != (int) jfctx->blk_size) {
			SMDBXI_MM_FREE(jfctx->mem, blkbuf);
			return -1;
		}
	}
	SMDBXI_MM_FREE(jfctx->mem, blkbuf);

	/*
	 * Be sure that the underlying file content hit the disk, before going
	 * ahead and nuke the journal file.
	 */
	if (SMDBXI_FL_SYNC(jfctx->bfile) < 0)
		return -1;

	/*
	 * Now truncate and sync the journal file.
	 */
	if (smdb_jf_truncate_journal(jfctx) < 0)
		return -1;

	return 1;
}

static int smdb_jf_open_journal(struct smdb_jfile_ctx *jfctx)
{
	if ((jfctx->jfpath =
	     smdb_jf_journal_path(jfctx->mem,
				  SMDBXI_FL_PATH(jfctx->bfile))) == NULL)
		return -1;

	if ((jfctx->jfile = SMDBXI_FS_OPEN(jfctx->fs, jfctx->jfpath,
					   SMDBXI_FL_RWOPEN)) == NULL) {
		if ((jfctx->jfile = SMDBXI_FS_OPEN(jfctx->fs, jfctx->jfpath,
						   SMDBXI_FL_CREATENEW)) == NULL)
			return -1;
	} else {
		if (smdb_jf_play_journal(jfctx) < 0)
			return -1;
	}

	return 0;
}

static int smdb_jf_file__get(void *priv)
{
	/*
	 * Nothing here.  This file lifetime is controlled by the journaling
	 * context data structure, and freed with smdb_jf_free().
	 */
	return 0;
}

static int smdb_jf_file__release(void *priv)
{
	/*
	 * Nothing here.  This file lifetime is controlled by the journaling
	 * context data structure, and freed with smdb_jf_free().
	 */
	return 0;
}

static smdb_offset_t smdb_jf_file__seek(void *priv, smdb_offset_t off, int whence)
{
	struct smdb_jfile_ctx *jfctx = (struct smdb_jfile_ctx *) priv;

	/*
	 * Go directly on file if journal is not enabled.
	 */
	if (!jfctx->enabled)
		return SMDBXI_FL_SEEK(jfctx->bfile, off, whence);

	switch (whence) {
	case SMDBXI_FL_SEEKSET:
		break;

	case SMDBXI_FL_SEEKEND:
		off = jfctx->fsize + off;
		break;

	case SMDBXI_FL_SEEKCUR:
		off = jfctx->offset + off;
		break;

	default:
		return -1;
	}
	if (off < 0 || off > jfctx->fsize)
		return -1;
	jfctx->offset = off;

	return jfctx->offset;
}

static int smdb_jf_file__read(void *priv, void *buf, int n)
{
	struct smdb_jfile_ctx *jfctx = (struct smdb_jfile_ctx *) priv;
	int count;
	smdb_offset_t joffset;

	/*
	 * Go directly on file if journal is not enabled.
	 */
	if (!jfctx->enabled)
		return SMDBXI_FL_READ(jfctx->bfile, buf, n);

	if (jfctx->offset >= jfctx->fsize)
		return 0;

	if (jfctx->offset + n > jfctx->fsize)
		n = (int) (jfctx->fsize - jfctx->offset);

	/*
	 * The upper layer is supposed to be a block layer, which
	 * read/write at block offset, in block sized chunks.
	 */
	if ((n % jfctx->blk_size) != 0 ||
	    (jfctx->offset % jfctx->blk_size) != 0)
		return -1;

	for (count = 0; count < n;
	     count += jfctx->blk_size, jfctx->offset += jfctx->blk_size) {
		if (smdb_jf_fetch_block(jfctx, jfctx->offset, &joffset) > 0) {
			if (smdb_off_read(jfctx->jfile, joffset,
					  (char *) buf + count,
					  jfctx->blk_size) != (int) jfctx->blk_size)
				break;
		} else {
			if (smdb_off_read(jfctx->bfile, jfctx->offset,
					  (char *) buf + count,
					  jfctx->blk_size) != (int) jfctx->blk_size)
				break;
		}
	}

	return count;
}

static int smdb_jf_file__write(void *priv, void const *buf, int n)
{
	struct smdb_jfile_ctx *jfctx = (struct smdb_jfile_ctx *) priv;
	int count;
	smdb_offset_t joffset;

	/*
	 * Go directly on file if journal is not enabled.
	 */
	if (!jfctx->enabled)
		return SMDBXI_FL_WRITE(jfctx->bfile, buf, n);

	/*
	 * The upper layer is supposed to be a block layer, which
	 * read/write at block offset, in block sized chunks.
	 */
	if ((n % jfctx->blk_size) != 0 ||
	    (jfctx->offset % jfctx->blk_size) != 0)
		return -1;

	for (count = 0; count < n;
	     count += jfctx->blk_size, jfctx->offset += jfctx->blk_size) {
		if (smdb_jf_want_block(jfctx, jfctx->offset, &joffset) < 0 ||
		    smdb_off_write(jfctx->jfile, joffset, (char const *) buf + count,
				   jfctx->blk_size) != (int) jfctx->blk_size)
			break;
		jfctx->active = 1;
	}
	if (jfctx->offset > jfctx->fsize)
		jfctx->fsize = jfctx->offset;

	return count;
}

static int smdb_jf_file__truncate(void *priv, smdb_offset_t size)
{
	struct smdb_jfile_ctx *jfctx = (struct smdb_jfile_ctx *) priv;

	/*
	 * Go directly on file if journal is not enabled.
	 */
	if (!jfctx->enabled)
		return SMDBXI_FL_TRUNCATE(jfctx->bfile, size);

	/*
	 * The upper layer is supposed to be a block layer, which
	 * resizes the DB file at multiple of block offset.
	 */
	if ((size % jfctx->blk_size) != 0)
		return -1;

	/*
	 * Free all the blocks whose offset is greater than size.
	 */
	if (smdb_jf_trim_blocks(jfctx, size) < 0)
		return -1;

	jfctx->fsize = size;

	return 0;
}

static int smdb_jf_file__sync(void *priv)
{
	struct smdb_jfile_ctx *jfctx = (struct smdb_jfile_ctx *) priv;

	return jfctx->enabled ? 0: SMDBXI_FL_SYNC(jfctx->bfile);
}

static char const *smdb_jf_file__path(void *priv)
{
	struct smdb_jfile_ctx *jfctx = (struct smdb_jfile_ctx *) priv;

	return SMDBXI_FL_PATH(jfctx->bfile);
}

int smdb_jf_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		   unsigned int blk_size, struct smdb_jfile_ctx **pjfctx)
{
	struct smdbxi_mem *mem;
	struct smdbxi_fs *fs;
	struct smdb_jfile_ctx *jfctx;

	mem = SMDBXI_FC_MEM(fac);
	fs = SMDBXI_FC_FS(fac);
	if (mem == NULL || fs == NULL ||
	    (jfctx = OBJALLOC(mem, struct smdb_jfile_ctx)) == NULL) {
		SMDBXI_RELEASE(fs);
		SMDBXI_RELEASE(mem);
		return -1;
	}
	SMDBXI_GET(bfile);
	jfctx->mem = mem;
	jfctx->fs = fs;
	jfctx->bfile = bfile;
	jfctx->blk_size = blk_size;

	jfctx->file_ifc.priv = jfctx;
	jfctx->file_ifc.get = smdb_jf_file__get;
	jfctx->file_ifc.release = smdb_jf_file__release;
	jfctx->file_ifc.seek = smdb_jf_file__seek;
	jfctx->file_ifc.read = smdb_jf_file__read;
	jfctx->file_ifc.write = smdb_jf_file__write;
	jfctx->file_ifc.truncate = smdb_jf_file__truncate;
	jfctx->file_ifc.sync = smdb_jf_file__sync;
	jfctx->file_ifc.path = smdb_jf_file__path;
	if (smdb_jf_alloc_blkhash(jfctx) < 0 ||
	    smdb_jf_open_journal(jfctx) < 0) {
		smdb_jf_free(jfctx);
		return -1;
	}

	*pjfctx = jfctx;

	return 0;
}

void smdb_jf_free(struct smdb_jfile_ctx *jfctx)
{
	if (jfctx != NULL) {
		struct smdbxi_mem *mem = jfctx->mem;

		SMDBXI_RELEASE(jfctx->jfile);
		if (jfctx->jfpath != NULL) {
			SMDBXI_FS_REMOVE(jfctx->fs, jfctx->jfpath);
			SMDBXI_MM_FREE(mem, jfctx->jfpath);
		}
		SMDBXI_RELEASE(jfctx->bfile);
		SMDBXI_RELEASE(jfctx->fs);
		SMDBXI_MM_FREE(mem, jfctx->bhash);
		SMDBXI_MM_FREE(mem, jfctx);
		SMDBXI_RELEASE(mem);
	}
}

struct smdbxi_file *smdb_jf_getfile(struct smdb_jfile_ctx *jfctx)
{
	return &jfctx->file_ifc;
}

int smdb_jf_begin(struct smdb_jfile_ctx *jfctx)
{
	/*
	 * Do not allow nested transactions.
	 */
	if (jfctx->enabled)
		return -1;

	/*
	 * The caller might have previously changed the file without the
	 * journal enabled, so we want to be sure that everything done
	 * before is actually on disk, before proceeding.
	 */
	if (SMDBXI_FL_SYNC(jfctx->bfile) < 0 ||
	    (jfctx->fsize = SMDBXI_FL_SEEK(jfctx->bfile, 0,
					   SMDBXI_FL_SEEKEND)) < 0)
		return -1;
	SMDBXI_FL_SEEK(jfctx->bfile, 0, SMDBXI_FL_SEEKSET);
	jfctx->enabled = 1;
	jfctx->active = 0;
	jfctx->offset = 0;

	return 0;
}

int smdb_jf_end(struct smdb_jfile_ctx *jfctx)
{
	jfctx->enabled = 0;
	if (jfctx->active) {
		if (smdb_jf_finish_journal(jfctx) < 0 ||
		    smdb_jf_play_journal(jfctx) < 0)
			return -1;
		jfctx->active = 0;
	}

	return 0;
}

int smdb_jf_rollback(struct smdb_jfile_ctx *jfctx)
{
	jfctx->enabled = 0;
	if (jfctx->active) {
		if (smdb_jf_truncate_journal(jfctx) < 0)
			return -1;
		smdb_jf_reset_blkhash(jfctx);
		jfctx->active = 0;
	}

	return 0;
}

