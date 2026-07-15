// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "dtbl.h"

/*
 * Least common multiplier of the sequence {1, 2, 3, ... , MAX_SERVERS}.
 * Size of an array needed for number of replicas r and number of servers n
 * is given by lcm(n)/r. I.e. for raid0, r=1.
 */
static int lcm[] = {1, 2, 6, 12, 60, 60, 420, 840, 2520, 2520, 27720, 27720,
		    360360, 360360, 360360, 720720, 12252240, 12252240,
		    232792560, 232792560, 232792560, 232792560};

int dtbl_size(int r, int k)
{
	if (r < 1 || k < 1 || k > MAX_SERVERS)
		return -EINVAL;

	return lcm[k - 1] / r;
}

struct dtbl *alloc_dtbl(int r, int n)
{
	struct dtbl *d;
	int i;

	if (r < 1 || n < r || n > MAX_SERVERS)
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
 * Hand `need[s]` entries from each existing server s over to the new
 * server, one sweep of the table at a time, spreading the picks across
 * the table instead of taking one contiguous run. Returns 1 on success,
 * 0 if the remaining `need[]` can never be satisfied.
 */
static int place_packed(int *tbl, int *need, char *done, int sz_new, int k,
			 int new_bit)
{
	int i, s, left = 0;

	for (s = 0; s < k - 1; s++)
		left += need[s];

	while (left) {
		int progress = 0;

		for (i = 0; i < sz_new; i++) {
			if (done[i])
				continue;

			for (s = 0; s < k - 1; s++) {
				if (!need[s] || !(tbl[i] & (1 << s)))
					continue;

				tbl[i] &= ~(1 << s);
				tbl[i] |= new_bit;
				done[i] = 1;
				need[s]--;
				left--;
				progress = 1;
				break;
			}
		}

		/* a full sweep placed nothing - the remaining `need[]`
		 * can never be satisfied, bail out instead of spinning
		 * forever.
		 */
		if (!progress)
			break;
	}

	return left == 0;
}

/*
 * Same job as place_packed(), but instead of sweeping the table left to
 * right and taking the first eligible entry, each of the D handovers is
 * aimed at an evenly-spaced target position (d * sz_new / D) and only
 * falls back to scanning forward from there when that slot is already
 * taken. Preferring, among the servers still eligible at that slot, the
 * one with the largest remaining need evens out how far through the
 * table each server's handovers end up spread.
 */
static int place_spread(int *tbl, int *need, char *done, int sz_new, int k,
			 int new_bit)
{
	int d, i, s, off, target, best, placed, D = 0;

	for (s = 0; s < k - 1; s++)
		D += need[s];

	for (d = 0; d < D; d++) {
		target = (int)((long long)d * sz_new / D);
		placed = 0;

		for (off = 0; off < sz_new && !placed; off++) {
			i = (target + off) % sz_new;

			if (done[i])
				continue;

			best = -1;
			for (s = 0; s < k - 1; s++) {
				if (need[s] && (tbl[i] & (1 << s)) &&
				    (best < 0 || need[s] > need[best]))
					best = s;
			}

			if (best < 0)
				continue;

			tbl[i] &= ~(1 << best);
			tbl[i] |= new_bit;
			done[i] = 1;
			need[best]--;
			placed = 1;
		}

		if (!placed)
			return 0;
	}

	return 1;
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
 *
 * @spread selects how those handovers are placed within the table:
 * packed (0) takes the first eligible entry on each sweep, spread (1)
 * aims each handover at an evenly-spaced target position instead.
 */
int gen_tbl(int r, int n, int **tbl, int spread)
{
	int i, k;

	if (r < 1 || n < r || n > MAX_SERVERS)
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
		int s, ok;

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

		ok = spread ?
			place_spread(tbl[idx], need, done, sz_new, k, new_bit) :
			place_packed(tbl[idx], need, done, sz_new, k, new_bit);

		free(need);
		free(done);

		if (!ok)
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

/*
 * Print the table for k servers the same way dtbl.md/dtbl.html does: one
 * row per server, one column per table entry, 'x' if that server holds
 * the entry, blank otherwise.
 *
 * With @dots, each row is further extended to the right with a "shadow"
 * of itself (same row, 'x' turned into '.') repeated enough times to
 * reach the size of the next table, then trailing blanks are trimmed.
 * This is what visualizes how the current table tiles into the next one
 * as the cluster grows.
 */
static void print_tbl(struct dtbl *d, int k, int dots)
{
	int sz = dtbl_size(d->r, k);
	int sz_next = (dots && k < d->n) ? dtbl_size(d->r, k + 1) : sz;
	int repeats = sz_next / sz - 1;
	int len = sz * (repeats + 1);
	char *line = malloc(len);
	int s, i, rep, end;

	for (s = 0; s < k; s++) {
		for (i = 0; i < sz; i++)
			line[i] = d->tbl[k - 1][i] & (1 << s) ? 'x' : ' ';

		for (rep = 1; rep <= repeats; rep++)
			for (i = 0; i < sz; i++)
				line[rep * sz + i] = line[i] == 'x' ? '.' : ' ';

		end = len;
		while (end > 0 && line[end - 1] == ' ')
			end--;

		fwrite(line, 1, end, stdout);
		putchar('\n');
	}

	free(line);
}

int main(int argc, char **argv)
{
	int r, n, k, ret, dots = 0, spread = 0;
	struct dtbl *d;
	const char *prog = argv[0];

	while (argc > 1 && argv[1][0] == '-') {
		if (!strcmp(argv[1], "--dots"))
			dots = 1;
		else if (!strcmp(argv[1], "--spread"))
			spread = 1;
		else
			break;

		argv++;
		argc--;
	}

	if (argc != 3) {
		fprintf(stderr,
			"usage: %s [--dots] [--spread] <replicas> <servers>\n",
			prog);
		return -EINVAL;
	}

	r = atoi(argv[1]);
	n = atoi(argv[2]);

	d = alloc_dtbl(r, n);
	if (!d) {
		fprintf(stderr, "alloc_dtbl(%d, %d) failed\n", r, n);
		return -ENOMEM;
	}

	ret = gen_tbl(r, n, d->tbl, spread);
	if (ret) {
		fprintf(stderr, "gen_tbl(%d, %d) failed: %d\n", r, n, ret);
		free_dtbl(d);
		return ret;
	}

	for (k = r; k <= n; k++) {
		print_tbl(d, k, dots);
		putchar('\n');
	}

	free_dtbl(d);

	return 0;
}
