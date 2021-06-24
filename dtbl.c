// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdlib.h>
#include <errno.h>

/*
 * Least common multiplier of the sequence {1, 2, 3, ... , 22}.
 * Size of an array needed for number of replicas r and number of servers n
 * is given by lcm(n)/r. I.e. for raid0, r=1.
 */
static int lcm[] = {1, 2, 6, 12, 60, 60, 420, 840, 2520, 2520, 27720, 27720,
		    360360, 360360, 360360, 720720, 12252240, 12252240,
		    232792560, 232792560, 232792560, 232792560};


struct dtbl {
	int r;		/* number of replicas, replication factor */
	int n;		/* number of servers in the cluster */
	int **tbl;	/* arrays */
};

struct dtbl *alloc_dtbl(int r, int n)
{
	struct dtbl *d;
	int i;

	d = malloc(sizeof(struct dtbl));
	if (!d)
		return NULL;

	d->r = r;
	d->n = n;

	d->tbl = malloc(n * sizeof(*d->tbl));
	if (!d->tbl) {
		free(d);
		return NULL;
	}

	for (i = 0; i < n; i++) {
		d->tbl[i] = malloc(lcm[n] / r * sizeof(**d->tbl));
		if (!d->tbl[i]) {
			while (i--)
				free(d->tbl[i]);

			free(d->tbl);
			free(d);

			return NULL;
		}
	}

	return d;
}

void free_dtbl(struct dtbl *d)
{
	int i;

	for (i = 0; i < d->n; i++)
		free(d->tbl[i]);

	free(d->tbl);

	free(d);
}

/*
 * Generate distribution tables needed for r replicas
 * over r, r + 1, ..., n servers.
 *
 * n starts at 0, i.e, 0 means 1 server, 1 means 2, etc.
 * tbl contains n elements - each an array of int
 */
int gen_tbl(int r, int n, int **tbl)
{
	int i;

	if (r < 1 || n < 1)
		return -EINVAL;

	return 0;
}

/*
 * Get bitmap saying to which servers does block @b belong given
 * the number of replicas @r and the overall number of servers @n
 * Each bit in the returned bitmap corresponds to one server.
 * If the bit is set in the returned bitmap, then the block has
 * to be sent to this server.
 * @b - virtual block number
 * @r - number of replicas (0 - raid0, 1 - raid1 on two legs, etc)
 * @n - overall number of machines in the pool minus 1
 * @returns bitmap of servers
 */
int get_srv(size_t b, short r, short n, int **tbl)
{
	return tbl[n][b % (lcm[n] / r)];
}

int main(int argc, char **argv)
{
	int **tbl;
	int ret;
	struct dtbl *d;

	d = alloc_dtbl(1, 1);
	if (!d)
		return -ENOMEM;

	free_dtbl(d);

	return 0;
}
