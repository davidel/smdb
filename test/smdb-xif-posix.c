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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "smdb-incl.h"
#include "smdb-xif-posix.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>

#define O_RDONLY _O_RDONLY
#define O_CREAT _O_CREAT
#define O_RDWR _O_RDWR
#define O_TRUNC _O_TRUNC

#define open(f, m, p) _open(f, (m) | _O_BINARY)
#define read(f, d, n) _read(f, d, n)
#define write(f, d, n) _write(f, d, n)
#define lseek(f, o, w) _lseek(f, o, w)
#define fsync(f) _commit(f)
#define mkdir(n, p) _mkdir(n)
#define rmdir(n) _rmdir(n)

#endif


struct smdbxi_mem_px {
	struct smdbxi_mem ifc;
	long usecnt;
};

struct smdbxi_file_px {
	struct smdbxi_file ifc;
	long usecnt;
	int fd;
	int closefd;
	char *filename;
	int unlinkfile;
};

struct smdbxi_fs_px {
	struct smdbxi_fs ifc;
	long usecnt;
};

struct smdbxi_factory_px {
	struct smdbxi_factory ifc;
	long usecnt;
	long seqf;
};


#if defined(WIN32)

static int ftruncate(int fd, smdb_offset_t size)
{
	HANDLE hfile;
	LARGE_INTEGER cpos, lsize;

	hfile = (HANDLE) _get_osfhandle(fd);
	lsize.QuadPart = 0;
	if (!SetFilePointerEx(hfile, lsize, &cpos, FILE_CURRENT))
		return -1;
	lsize.QuadPart = size;
	if (!SetFilePointerEx(hfile, lsize, &cpos, FILE_BEGIN))
		return -1;
	if (!SetEndOfFile(hfile)) {
		lsize = cpos;
		SetFilePointerEx(hfile, lsize, &cpos, FILE_BEGIN);
		return -1;
	}
	if (cpos.QuadPart > size)
		cpos.QuadPart = size;
	SetFilePointerEx(hfile, cpos, NULL, FILE_BEGIN);

	return 0;
}

#endif

static int smdb_xif_mem__get(void *priv)
{
	struct smdbxi_mem_px *pif = (struct smdbxi_mem_px *) priv;

	pif->usecnt++;

	return 0;
}

static int smdb_xif_mem__release(void *priv)
{
	struct smdbxi_mem_px *pif = (struct smdbxi_mem_px *) priv;

	if (!--pif->usecnt) {

		free(pif);
	}

	return 0;
}

static void *smdb_xif_mem__alloc(void *priv, int size)
{
	return malloc(size);
}

static void smdb_xif_mem__free(void *priv, void *data)
{
	free(data);
}

static struct smdbxi_mem *smdb_xif_mem(void)
{
	struct smdbxi_mem_px *pif;

	if ((pif = (struct smdbxi_mem_px *)
	     malloc(sizeof(struct smdbxi_mem_px))) == NULL)
		return NULL;
	pif->ifc.priv = pif;
	pif->ifc.get = smdb_xif_mem__get;
	pif->ifc.release = smdb_xif_mem__release;
	pif->ifc.alloc = smdb_xif_mem__alloc;
	pif->ifc.free = smdb_xif_mem__free;
	pif->usecnt = 1;

	return &pif->ifc;
}

static int smdb_xif_file__get(void *priv)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	pif->usecnt++;

	return 0;
}

static int smdb_xif_file__release(void *priv)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	if (!--pif->usecnt) {
		if (pif->closefd)
			close(pif->fd);
		if (pif->filename != NULL) {
			if (pif->unlinkfile)
				remove(pif->filename);
			free(pif->filename);
		}
		free(pif);
	}

	return 0;
}

static smdb_offset_t smdb_xif_file__seek(void *priv, smdb_offset_t off, int whence)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	return lseek(pif->fd, (off_t) off, whence);
}

static int smdb_xif_file__read(void *priv, void *buf, int n)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	return read(pif->fd, buf, n);
}

static int smdb_xif_file__write(void *priv, void const *buf, int n)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	return write(pif->fd, buf, n);
}

static int smdb_xif_file__truncate(void *priv, smdb_offset_t size)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	return ftruncate(pif->fd, (off_t) size);
}

static int smdb_xif_file__sync(void *priv)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	return fsync(pif->fd);
}

static char const *smdb_xif_file__path(void *priv)
{
	struct smdbxi_file_px *pif = (struct smdbxi_file_px *) priv;

	return pif->filename;
}

struct smdbxi_file *smdb_xif_file(int fd, int closefd, char const *filename,
				  int flags, int unlinkfile)
{
	int lfd = -1;
	struct smdbxi_file_px *pif;

	if (fd < 0) {
		if (filename == NULL)
			return NULL;
		switch (flags)
		{
		case SMDBXI_FL_ROPEN:
			flags = O_RDONLY;
			break;
		case SMDBXI_FL_RWOPEN:
			flags = O_RDWR;
			break;
		case SMDBXI_FL_CREATE:
			flags = O_RDWR | O_CREAT;
			break;
		case SMDBXI_FL_CREATENEW:
			flags = O_RDWR | O_CREAT | O_TRUNC;
			break;
		default:
			return NULL;
		}
		if ((fd = lfd = open(filename, flags, 0644)) == -1)
			return NULL;
		closefd = 1;
	}
	if ((pif = (struct smdbxi_file_px *)
	     malloc(sizeof(struct smdbxi_file_px))) == NULL) {
		if (lfd != -1) {
			close(lfd);
			if (unlinkfile)
				remove(filename);
		}
		return NULL;
	}
	pif->ifc.priv = pif;
	pif->ifc.get = smdb_xif_file__get;
	pif->ifc.release = smdb_xif_file__release;
	pif->ifc.seek = smdb_xif_file__seek;
	pif->ifc.read = smdb_xif_file__read;
	pif->ifc.write = smdb_xif_file__write;
	pif->ifc.truncate = smdb_xif_file__truncate;
	pif->ifc.sync = smdb_xif_file__sync;
	pif->ifc.path = smdb_xif_file__path;
	pif->usecnt = 1;
	pif->fd = fd;
	pif->closefd = closefd;
	pif->filename = filename != NULL ? strdup(filename): NULL;
	pif->unlinkfile = unlinkfile;

	return &pif->ifc;
}

static int smdb_xif_fs__get(void *priv)
{
	struct smdbxi_fs_px *pif = (struct smdbxi_fs_px *) priv;

	pif->usecnt++;

	return 0;
}

static int smdb_xif_fs__release(void *priv)
{
	struct smdbxi_fs_px *pif = (struct smdbxi_fs_px *) priv;

	if (!--pif->usecnt) {

		free(pif);
	}

	return 0;
}

static struct smdbxi_file *smdb_xif_fs__open(void *priv, char const *path,
					     int flags)
{
	return smdb_xif_file(-1, 0, path, flags, 0);
}

static int smdb_xif_fs__remove(void *priv, char const *path)
{
	return remove(path);
}

static int smdb_xif_fs__rename(void *priv, char const *opath, char const *npath)
{
	return rename(opath, npath);
}

static int smdb_xif_fs__mkdir(void *priv, char const *path)
{
	return mkdir(path, 0775);
}

static int smdb_xif_fs__rmdir(void *priv, char const *path)
{
	return rmdir(path);
}

static struct smdbxi_fs *smdb_xif_fs(void)
{
	struct smdbxi_fs_px *pif;

	if ((pif = (struct smdbxi_fs_px *)
	     malloc(sizeof(struct smdbxi_fs_px))) == NULL)
		return NULL;
	pif->ifc.priv = pif;
	pif->ifc.get = smdb_xif_fs__get;
	pif->ifc.release = smdb_xif_fs__release;
	pif->ifc.open = smdb_xif_fs__open;
	pif->ifc.remove = smdb_xif_fs__remove;
	pif->ifc.rename = smdb_xif_fs__rename;
	pif->ifc.mkdir = smdb_xif_fs__mkdir;
	pif->ifc.rmdir = smdb_xif_fs__rmdir;
	pif->usecnt = 1;

	return &pif->ifc;
}

static int smdb_xif_factory__get(void *priv)
{
	struct smdbxi_factory_px *pif = (struct smdbxi_factory_px *) priv;

	pif->usecnt++;

	return 0;
}

static int smdb_xif_factory__release(void *priv)
{
	struct smdbxi_factory_px *pif = (struct smdbxi_factory_px *) priv;

	if (!--pif->usecnt) {

		free(pif);
	}

	return 0;
}

static struct smdbxi_mem *smdb_xif_factory__mem(void *priv)
{
	return smdb_xif_mem();
}

static struct smdbxi_fs *smdb_xif_factory__fs(void *priv)
{
	return smdb_xif_fs();
}

static struct smdbxi_file *smdb_xif_factory__file(void *priv)
{
	struct smdbxi_factory_px *pif = (struct smdbxi_factory_px *) priv;
	char filename[256];

	sprintf(filename, "%p-%ld.tmp", priv, pif->seqf++);

	return smdb_xif_file(-1, 1, filename, O_CREAT | O_RDWR | O_TRUNC, 1);
}

struct smdbxi_factory *smdb_xif_factory(void)
{
	struct smdbxi_factory_px *pif;

	if ((pif = (struct smdbxi_factory_px *)
	     malloc(sizeof(struct smdbxi_factory_px))) == NULL)
		return NULL;
	pif->ifc.priv = pif;
	pif->ifc.get = smdb_xif_factory__get;
	pif->ifc.release = smdb_xif_factory__release;
	pif->ifc.mem = smdb_xif_factory__mem;
	pif->ifc.file = smdb_xif_factory__file;
	pif->ifc.fs = smdb_xif_factory__fs;
	pif->usecnt = 1;
	pif->seqf = 0;

	return &pif->ifc;
}

