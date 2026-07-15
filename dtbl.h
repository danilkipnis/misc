// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DTBL_H
#define DTBL_H

#include <stddef.h>

/* lcm[] only has entries for 1..MAX_SERVERS machines */
#define MAX_SERVERS 22

struct dtbl {
	int r;		/* number of replicas, replication factor */
	int n;		/* number of servers in the cluster */
	int **tbl;	/* arrays */
};

struct dtbl *alloc_dtbl(int r, int n);
void free_dtbl(struct dtbl *d);
int gen_tbl(int r, int n, int **tbl, int spread);
int get_srv(size_t b, short r, short n, int **tbl);

/* number of entries in the table for replication factor r and k servers */
int dtbl_size(int r, int k);

#endif
