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


#ifndef _SMDB_JOURNAL_H
#define _SMDB_JOURNAL_H

struct smdb_jbhash_node {
	smdb_u32 offset;
	smdb_u32 joffset;
};

struct smdb_jfile_ctx {
	struct smdbxi_file file_ifc;
	struct smdbxi_mem *mem;
	struct smdbxi_fs *fs;
	struct smdbxi_file *bfile;
	struct smdbxi_file *jfile;
	unsigned int blk_size;
	smdb_offset_t offset;
	smdb_offset_t fsize;
	char *jfpath;
	int enabled;
	int active;
	struct smdb_jbhash_node *bhash;
	unsigned long bhmask;
	unsigned long bhbits;
	unsigned long bcount;
	unsigned long bdeleted;
	unsigned long bfree;
};

EXTC_BEGIN;

int smdb_jf_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		   unsigned int blk_size, struct smdb_jfile_ctx **pjfctx);
void smdb_jf_free(struct smdb_jfile_ctx *jfctx);
struct smdbxi_file *smdb_jf_getfile(struct smdb_jfile_ctx *jfctx);
int smdb_jf_begin(struct smdb_jfile_ctx *jfctx);
int smdb_jf_end(struct smdb_jfile_ctx *jfctx);
int smdb_jf_rollback(struct smdb_jfile_ctx *jfctx);

EXTC_END;

#endif

