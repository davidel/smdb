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

#ifndef _SMDB_CFILE_H
#define _SMDB_CFILE_H

struct smdb_cfile_ctx {
	struct smdbxi_mem *mem;
	struct smdbxi_file *bfile;
	struct smdb_bc_ctx *bctx;
};

EXTC_BEGIN;

int smdb_cf_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		   struct smdb_bc_config const *bcfg,
		   struct smdb_cfile_ctx **pcfctx);
void smdb_cf_free(struct smdb_cfile_ctx *cfctx);
int smdb_cf_sync(struct smdb_cfile_ctx *cfctx);
smdb_offset_t smdb_cf_file_size(struct smdb_cfile_ctx *cfctx);
smdb_u32 smdb_cf_block_size(struct smdb_cfile_ctx *cfctx);
int smdb_cf_read(struct smdb_cfile_ctx *cfctx, smdb_offset_t offset,
		 void *data, unsigned long size);
int smdb_cf_write(struct smdb_cfile_ctx *cfctx, smdb_offset_t offset,
		  void const *data, unsigned long size);
struct smdb_bc_node *smdb_cf_get_block(struct smdb_cfile_ctx *cfctx,
				       smdb_u32 blkno, int excl);
void smdb_cf_release_block(struct smdb_cfile_ctx *cfctx,
			   struct smdb_bc_node *bcn);
void smdb_cf_set_block_dirty(struct smdb_cfile_ctx *cfctx,
			     struct smdb_bc_node *bcn);
int smdb_cf_copy(struct smdb_cfile_ctx *cfctx, smdb_u32 bdest, smdb_u32 bsrc,
		 smdb_u32 nblocks);
int smdb_cf_zero(struct smdb_cfile_ctx *cfctx, smdb_u32 blkno, smdb_u32 nblocks);

EXTC_END;

#endif

