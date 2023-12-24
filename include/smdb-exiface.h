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

#ifndef _SMDB_EXIFACE_H
#define _SMDB_EXIFACE_H

#define SMDBXI_GET(p) (*(p)->get)((p)->priv)
#define SMDBXI_RELEASE(p) ((p) != NULL ? (*(p)->release)((p)->priv): 0)

struct smdbxi_mem {
	void *priv;
	int (*get)(void *);
	int (*release)(void *);
	void *(*alloc)(void *, int);
	void (*free)(void *, void *);
};

#define SMDBXI_MM_ALLOC(p, s) (*(p)->alloc)((p)->priv, s)
#define SMDBXI_MM_FREE(p, d) (*(p)->free)((p)->priv, d)

struct smdbxi_lock {
	void *priv;
	int (*get)(void *);
	int (*release)(void *);
	void *(*lock)(void *);
	void (*unlock)(void *);
};

#define SMDBXI_LK_LOCK(p) (*(p)->lock)((p)->priv)
#define SMDBXI_LK_UNLOCK(p) (*(p)->unlock)((p)->priv)

#define SMDBXI_FL_SEEKSET 0
#define SMDBXI_FL_SEEKCUR 1
#define SMDBXI_FL_SEEKEND 2

#define SMDBXI_FL_ROPEN 1
#define SMDBXI_FL_RWOPEN 2
#define SMDBXI_FL_CREATE 3
#define SMDBXI_FL_CREATENEW 4

typedef smdb_s64 smdb_offset_t;

struct smdbxi_file {
	void *priv;
	int (*get)(void *);
	int (*release)(void *);
	smdb_offset_t (*seek)(void *, smdb_offset_t, int);
	int (*read)(void *, void *, int);
	int (*write)(void *, void const *, int);
	int (*truncate)(void *, smdb_offset_t);
	int (*sync)(void *);
	char const *(*path)(void *);
};

#define SMDBXI_FL_SEEK(p, o, w) (*(p)->seek)((p)->priv, o, w)
#define SMDBXI_FL_READ(p, b, n) (*(p)->read)((p)->priv, b, n)
#define SMDBXI_FL_WRITE(p, b, n) (*(p)->write)((p)->priv, b, n)
#define SMDBXI_FL_TRUNCATE(p, s) (*(p)->truncate)((p)->priv, s)
#define SMDBXI_FL_SYNC(p) (*(p)->sync)((p)->priv)
#define SMDBXI_FL_PATH(p) (*(p)->path)((p)->priv)

struct smdbxi_fs {
	void *priv;
	int (*get)(void *);
	int (*release)(void *);
	struct smdbxi_file *(*open)(void *, char const *, int);
	int (*remove)(void *, char const *);
	int (*rename)(void *, char const *, char const *);
	int (*mkdir)(void *, char const *);
	int (*rmdir)(void *, char const *);
};

#define SMDBXI_FS_OPEN(p, n, f) (*(p)->open)((p)->priv, n, f)
#define SMDBXI_FS_REMOVE(p, n) (*(p)->remove)((p)->priv, n)
#define SMDBXI_FS_RENAME(p, o, n) (*(p)->remove)((p)->priv, o, n)
#define SMDBXI_FS_MKDIR(p, n) (*(p)->mkdir)((p)->priv, n)
#define SMDBXI_FS_RMDIR(p, n) (*(p)->remove)((p)->priv, n)

struct smdbxi_factory {
	void *priv;
	int (*get)(void *);
	int (*release)(void *);
	struct smdbxi_mem *(*mem)(void *);
	struct smdbxi_file *(*file)(void *);
	struct smdbxi_fs *(*fs)(void *);
	struct smdbxi_lock *(*lock)(void *);
};

#define SMDBXI_FC_MEM(p) (*(p)->mem)((p)->priv)
#define SMDBXI_FC_FILE(p) (*(p)->file)((p)->priv)
#define SMDBXI_FC_FS(p) (*(p)->fs)((p)->priv)
#define SMDBXI_FC_LOCK(p) (*(p)->lock)((p)->priv)

#endif

