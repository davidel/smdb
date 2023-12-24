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

