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

#ifndef _SMDB_TYPES_H
#define _SMDB_TYPES_H

struct smdb_db_key {
	void *data;
	unsigned long size;
};

struct smdb_db_ckey {
	void const *data;
	unsigned long size;
};

struct smdb_db_data {
	void *data;
	unsigned long size;
};

struct smdb_db_cdata {
	void const *data;
	unsigned long size;
};

struct smdb_db_record {
	void *record;
	struct smdb_db_key key;
	struct smdb_db_data data;
};

#endif

