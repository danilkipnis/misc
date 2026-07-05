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

	if (r < 1 || n < r)
		return NULL;

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

	/*
	 * Table i (0-based) holds the mapping for i + 1 servers. A table
	 * only makes sense once there are at least r servers to hold r
	 * replicas, so tables for i < r - 1 are left unallocated.
	 */
	for (i = 0; i < n; i++) {
		if (i < r - 1) {
			d->tbl[i] = NULL;
			continue;
		}

		d->tbl[i] = malloc(lcm[i] / r * sizeof(**d->tbl));
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
 *
 * Construction: the table for k servers is built by growing the table
 * for k - 1 servers, so that scaling the cluster up never reshuffles
 * data between two servers that were already in the pool - it only
 * ever moves blocks onto the newly added server. This is what makes
 * redistribution cost 1/k.
 *
 * tbl[k - 1] has lcm[k - 1] / r entries, and since lcm[k - 1] (the LCM
 * of 1..k) is a multiple of lcm[k - 2] (the LCM of 1..k - 1), the new
 * table is an exact integer number of tiled copies of the old one.
 * Tiling alone reproduces the old (balanced) distribution; each of
 * the k - 1 existing servers then has to hand exactly
 * lcm[k - 1] / (k * (k - 1)) of its entries over to the new server so
 * that all k servers end up holding lcm[k - 1] / k entries each -
 * this quantity is always an integer because k and k - 1 are coprime
 * and both divide lcm[k - 1] individually.
 */
int gen_tbl(int r, int n, int **tbl)
{
	int i, k;

	if (r < 1 || n < r)
		return -EINVAL;

	/* base case: with exactly r servers, every server holds every
	 * block, i.e. all r bits are set in every entry.
	 */
	for (i = 0; i < lcm[r - 1] / r; i++)
		tbl[r - 1][i] = (1 << r) - 1;

	/* grow the cluster one server at a time */
	for (k = r + 1; k <= n; k++) {
		int idx = k - 1, prev = k - 2;
		int sz_new = lcm[idx] / r;
		int sz_old = lcm[prev] / r;
		int move = lcm[idx] / (k * (k - 1));
		int new_bit = 1 << (k - 1);
		int *need;
		char *done;
		int s, left;

		need = malloc((k - 1) * sizeof(*need));
		done = calloc(sz_new, sizeof(*done));
		if (!need || !done) {
			free(need);
			free(done);
			return -ENOMEM;
		}

		for (s = 0; s < k - 1; s++)
			need[s] = move;

		/* tile the previous, already-balanced table */
		for (i = 0; i < sz_new; i++)
			tbl[idx][i] = tbl[prev][i % sz_old];

		/* hand `move` entries from every existing server over to
		 * the new one, spreading the picks across the table
		 * instead of taking one contiguous run.
		 */
		left = (k - 1) * move;
		while (left) {
			int progress = 0;

			for (i = 0; i < sz_new; i++) {
				if (done[i])
					continue;

				for (s = 0; s < k - 1; s++) {
					if (!need[s] ||
					    !(tbl[idx][i] & (1 << s)))
						continue;

					tbl[idx][i] &= ~(1 << s);
					tbl[idx][i] |= new_bit;
					done[i] = 1;
					need[s]--;
					left--;
					progress = 1;
					break;
				}
			}

			/* a full sweep placed nothing - the remaining
			 * `need[]` can never be satisfied, bail out
			 * instead of spinning forever.
			 */
			if (!progress)
				break;
		}

		free(need);
		free(done);

		if (left)
			return -EINVAL;
	}

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
