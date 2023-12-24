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

