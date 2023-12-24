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

