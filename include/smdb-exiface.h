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

