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

#ifndef _SMDB_BCACHE_H
#define _SMDB_BCACHE_H

#define SMDB_BCF_DIRTY (1 << 0)

struct smdb_bc_config {
	smdb_u32 blk_size;
	smdb_u32 blk_max;
};

struct smdb_bc_node {
	struct smdb_listhead lnk;
	struct smdb_listhead lrulnk;
	void *data;
	smdb_u32 blkno;
	smdb_u32 flags;
	long usecnt;
};

struct smdb_bc_ctx {
	struct smdbxi_mem *mem;
	struct smdbxi_file *bfile;
	struct smdb_listhead lru;
	smdb_u32 blk_size;
	smdb_u32 blk_count;
	smdb_u32 blk_max;
	smdb_u32 hash_mask;
	struct smdb_listhead *hash;
	smdb_offset_t fsize;
};

EXTC_BEGIN;

int smdb_bc_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		   struct smdb_bc_config const *bcfg,
		   struct smdb_bc_ctx **pbctx);
void smdb_bc_free(struct smdb_bc_ctx *bctx);
int smdb_bc_sync(struct smdb_bc_ctx *bctx);
smdb_u32 smdb_bc_block_size(struct smdb_bc_ctx *bctx);
smdb_offset_t smdb_bc_file_size(struct smdb_bc_ctx *bctx);
struct smdb_bc_node *smdb_bc_get_block(struct smdb_bc_ctx *bctx,
				       smdb_u32 blkno, int excl);
void smdb_bc_release_block(struct smdb_bc_ctx *bctx,
			   struct smdb_bc_node *bcn);
void smdb_bc_set_block_dirty(struct smdb_bc_ctx *bctx,
			     struct smdb_bc_node *bcn);
void *smdb_bc_get_block_data(struct smdb_bc_node *bcn);

EXTC_END;

#endif

