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
#include <string.h>
#include "smdb-incl.h"
#include "smdb-xif-posix.h"

#define MODE_PUT 1
#define MODE_GET 2
#define MODE_ERASE 3
#define MODE_CMP 4
#define MODE_DUMP 5
#define MODE_RMTABLE 6
#define MODE_MKTABLE 7

static void *load_file(char const *path, long *pfsize)
{
	long fsize;
	void *fdata;
	FILE *file;

	if ((file = fopen(path, "rb")) == NULL) {
		perror(path);
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	fsize = ftell(file);
	rewind(file);
	if ((fdata = malloc(fsize + 1)) == NULL) {
		perror("allocating file data");
		fclose(file);
		return NULL;
	}
	if (fread(fdata, 1, (size_t) fsize, file) != (size_t) fsize) {
		perror(path);
		free(fdata);
		fclose(file);
		return NULL;
	}
	fclose(file);
	((char *) fdata)[fsize] = 0;
	*pfsize = fsize;

	return fdata;
}

static char **get_flist(char **ifiles, int infiles, char const *flpath,
			int *pnfiles)
{
	int i, nalloc;
	long fsize;
	char *fdata, *tok;
	char **flist, **nflist;

	if (flpath == NULL) {
		nalloc = infiles + 1;
		if ((flist = (char **) malloc(nalloc * sizeof(char *))) == NULL)
			return NULL;
		for (i = 0; i < infiles; i++)
			flist[i] = strdup(ifiles[i]);
		*pnfiles = infiles;
		return flist;
	}
	if ((fdata = (char *) load_file(flpath, &fsize)) == NULL) {
		perror(flpath);
		return NULL;
	}
	nalloc = infiles + (int) (fsize / 16) + 1;
	if ((flist = (char **) malloc(nalloc * sizeof(char *))) == NULL) {
		free(fdata);
		return NULL;
	}
	for (i = 0; i < infiles; i++)
		flist[i] = strdup(ifiles[i]);
	for (tok = strtok(fdata, "\r\n"); tok != NULL;
	     tok = strtok(NULL, "\r\n")) {
		if (i >= nalloc) {
			nalloc = nalloc * 2 + 32;
			if ((nflist = (char **)
			     realloc(flist, nalloc * sizeof(char *))) == NULL) {
				for (i--; i >= 0; i--)
					free(flist[i]);
				free(flist);
				free(fdata);
				return NULL;
			}
		}
		flist[i++] = strdup(tok);
	}
	free(fdata);
	*pnfiles = i;

	return flist;
}

static void free_flist(char **flist, int n)
{
	if (flist != NULL) {
		for (; n > 0; n--)
			free(flist[n - 1]);
	}
	free(flist);
}

int main(int ac, char **av)
{
	int i, error, nfiles, mode = MODE_PUT, journal = 0;
	unsigned int tblsize = 16000, tblid = 0;
	long fsize, rcount;
	void *fdata;
	char **files;
	char const *path = NULL, *flpath = NULL;
	struct smdbxi_factory *fac;
	struct smdbxi_file *file;
	struct smdb_dbfile_ctx *dfctx;
	struct smdb_db_config dbcfg;
	struct smdb_db_ckey ckey;
	struct smdb_db_cdata cdata;
	struct smdb_db_record rec;
	struct smdb_db_kenum ken;

	MZERO(dbcfg);
	dbcfg.blk_size = 1024;
	dbcfg.blk_count = 100 * 1024;
	dbcfg.cache_size = 64 * 1024;
	dbcfg.num_tables = 16;
	for (i = 1; i < ac; i++) {
		if (strcmp(av[i], "-f") == 0) {
			if (++i < ac)
				path = av[i];
		} else if (strcmp(av[i], "-l") == 0) {
			if (++i < ac)
				flpath = av[i];
		} else if (strcmp(av[i], "-b") == 0) {
			if (++i < ac)
				dbcfg.blk_size = strtoul(av[i], NULL, 0);
		} else if (strcmp(av[i], "-c") == 0) {
			if (++i < ac)
				dbcfg.blk_count = strtoul(av[i], NULL, 0);
		} else if (strcmp(av[i], "-s") == 0) {
			if (++i < ac)
				dbcfg.cache_size = strtoul(av[i], NULL, 0);
		} else if (strcmp(av[i], "-x") == 0) {
			if (++i < ac)
				tblsize = strtoul(av[i], NULL, 0);
		} else if (strcmp(av[i], "-T") == 0) {
			if (++i < ac)
				tblid = strtoul(av[i], NULL, 0);
		} else if (strcmp(av[i], "-g") == 0)
			mode = MODE_GET;
		else if (strcmp(av[i], "-e") == 0)
			mode = MODE_ERASE;
		else if (strcmp(av[i], "-m") == 0)
			mode = MODE_CMP;
		else if (strcmp(av[i], "-d") == 0)
			mode = MODE_DUMP;
		else if (strcmp(av[i], "-R") == 0)
			mode = MODE_RMTABLE;
		else if (strcmp(av[i], "-M") == 0)
			mode = MODE_MKTABLE;
		else if (strcmp(av[i], "-j") == 0)
			journal = 1;
		else
			break;
	}
	if (path == NULL)
		return 1;

	if ((fac = smdb_xif_factory()) == NULL)
		return 2;
	if ((file = smdb_xif_file(-1, 0, path, SMDBXI_FL_RWOPEN, 0)) == NULL) {
		if ((file = smdb_xif_file(-1, 0, path, SMDBXI_FL_CREATENEW,
					  0)) == NULL)
			return 3;
		if (smdb_dbf_create(fac, file, &dbcfg, &dfctx) < 0)
			return 4;
		if (smdb_dbf_create_table(dfctx, tblid, tblsize) < 0)
			return 5;
	} else {
		if (smdb_dbf_open(fac, file, &dbcfg, &dfctx) < 0)
			return 4;
	}

	if ((files = get_flist(&av[i], ac - i, flpath, &nfiles)) == NULL)
		return 6;

	fprintf(stdout, "Number of files: %d\n", nfiles);

	if (journal && smdb_dbf_begin(dfctx) < 0)
		return 7;

	if (mode == MODE_GET) {
		for (i = 0; i < nfiles; i++) {
			ckey.data = files[i];
			ckey.size = strlen(files[i]);
			if (smdb_dbf_get(dfctx, tblid, &ckey, &rec, &ken) > 0) {

				smdb_dbf_free_record(dfctx, &rec);
			} else {
				fprintf(stderr, "Record not found: '%s'\n", files[i]);
			}
		}
	} else if (mode == MODE_ERASE) {
		for (i = 0; i < nfiles; i++) {
			ckey.data = files[i];
			ckey.size = strlen(files[i]);
			if (smdb_dbf_erase(dfctx, tblid, &ckey, NULL) > 0) {

			} else {
				fprintf(stderr, "Record not found: '%s'\n", files[i]);
			}
		}
	} else if (mode == MODE_PUT) {
		for (i = 0; i < nfiles; i++) {
			if ((fdata = load_file(files[i], &fsize)) == NULL) {
				free_flist(files, nfiles);
				return 8;
			}

			ckey.data = files[i];
			ckey.size = strlen(files[i]);
			cdata.data = fdata;
			cdata.size = fsize;
			if (smdb_dbf_put(dfctx, tblid, &ckey, &cdata) < 0) {
				fprintf(stderr, "DD insert failed: '%s'\n", files[i]);
				free_flist(files, nfiles);
				return 9;
			}
			free(fdata);
		}
	} else if (mode == MODE_CMP) {
		for (i = 0; i < nfiles; i++) {
			if ((fdata = load_file(files[i], &fsize)) == NULL) {
				free_flist(files, nfiles);
				return 8;
			}

			ckey.data = files[i];
			ckey.size = strlen(files[i]);
			if (smdb_dbf_get(dfctx, tblid, &ckey, &rec, &ken) > 0) {
				if (rec.data.size != (unsigned long) fsize ||
				    memcmp(rec.data.data, fdata, fsize) != 0)
					fprintf(stderr, "Record in DB differs: '%s'\n",
						files[i]);

				smdb_dbf_free_record(dfctx, &rec);
			} else {
				fprintf(stderr, "Record not found: '%s'\n", files[i]);
			}
			free(fdata);
		}
	} else if (mode == MODE_DUMP) {
		rcount = 0;
		if ((error = smdb_dbf_first(dfctx, tblid, &rec, &ken)) > 0) {
			do {
				fprintf(stdout, "KEY (%ld)='", rec.key.size);
				fwrite(rec.key.data, 1, rec.key.size, stdout);
				fprintf(stdout, "'\n");

				fprintf(stdout, "\tDATA (%ld)\n\n", rec.data.size);

				smdb_dbf_free_record(dfctx, &rec);
				rcount++;
			} while ((error = smdb_dbf_next(dfctx, &rec, &ken)) > 0);
		}
		if (error < 0) {
			fprintf(stderr, "Record enumeration failed!\n");
			free_flist(files, nfiles);
			return 9;
		}
		fprintf(stdout, "\nFound %ld records.\n", rcount);
	} else if (mode == MODE_RMTABLE) {
		if (smdb_dbf_free_table(dfctx, tblid) < 0) {
			fprintf(stderr, "Table remove failed!\n");
			return 11;
		}
	} else if (mode == MODE_MKTABLE) {
		if (smdb_dbf_create_table(dfctx, tblid, tblsize) < 0) {
			fprintf(stderr, "Table create failed!\n");
			return 11;
		}
	}

	free_flist(files, nfiles);
	if (journal && smdb_dbf_end(dfctx) < 0)
		return 12;

	smdb_dbf_free(dfctx);
	SMDBXI_RELEASE(file);
	SMDBXI_RELEASE(fac);

	return 0;
}

