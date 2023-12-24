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


#ifndef _SMDB_DBFILE_H
#define _SMDB_DBFILE_H

struct smdb_db_config {
	smdb_u32 blk_size;
	smdb_u32 blk_count;
	smdb_u32 cache_size;
	smdb_u32 num_tables;
};

struct smdb_db_kenum {
	smdb_u32 tblid;
	smdb_u32 hashv;
	smdb_u32 hsize;
	smdb_u32 idx;
};

struct smdb_dbfile_ctx {
	struct smdbxi_mem *mem;
	struct smdbxi_fs *fs;
	struct smdbxi_file *bfile;
	struct smdb_cfile_ctx *cfctx;
	struct smdb_jfile_ctx *jfctx;
};

EXTC_BEGIN;

int smdb_dbf_create(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		    struct smdb_db_config const *dbcfg,
		    struct smdb_dbfile_ctx **pdfctx);
int smdb_dbf_open(struct smdbxi_factory *fac, struct smdbxi_file *bfile,
		  struct smdb_db_config const *dbcfg,
		  struct smdb_dbfile_ctx **pdfctx);
void smdb_dbf_free(struct smdb_dbfile_ctx *dfctx);
int smdb_dbf_create_table(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
			  unsigned int tblsize);
int smdb_dbf_free_table(struct smdb_dbfile_ctx *dfctx, unsigned int tblid);
int smdb_dbf_sync(struct smdb_dbfile_ctx *dfctx);
int smdb_dbf_begin(struct smdb_dbfile_ctx *dfctx);
int smdb_dbf_end(struct smdb_dbfile_ctx *dfctx);
int smdb_dbf_rollback(struct smdb_dbfile_ctx *dfctx);
int smdb_dbf_get(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		 struct smdb_db_ckey const *key, struct smdb_db_record *rec,
		 struct smdb_db_kenum *ken);
int smdb_dbf_get_next(struct smdb_dbfile_ctx *dfctx,
		      struct smdb_db_ckey const *key,
		      struct smdb_db_record *rec, struct smdb_db_kenum *ken);
int smdb_dbf_first(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		   struct smdb_db_record *rec, struct smdb_db_kenum *ken);
int smdb_dbf_next(struct smdb_dbfile_ctx *dfctx, struct smdb_db_record *rec,
		  struct smdb_db_kenum *ken);
void smdb_dbf_free_record(struct smdb_dbfile_ctx *dfctx,
			  struct smdb_db_record *rec);
int smdb_dbf_put(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		 struct smdb_db_ckey const *key, struct smdb_db_cdata *data);
int smdb_dbf_erase(struct smdb_dbfile_ctx *dfctx, unsigned int tblid,
		   struct smdb_db_ckey const *key, struct smdb_db_cdata const *data);

EXTC_END;

#endif

