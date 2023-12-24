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

