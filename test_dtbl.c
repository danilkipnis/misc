// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdio.h>
#include <stdlib.h>

#include "dtbl.h"

static int popcount(int x)
{
	int c = 0;

	while (x) {
		c += x & 1;
		x >>= 1;
	}
	return c;
}

static int check(int r, int n, int spread)
{
	struct dtbl *d;
	int k, i, s, ret, fail = 0;

	d = alloc_dtbl(r, n);
	if (!d) {
		printf("r=%d n=%d spread=%d: alloc failed\n", r, n, spread);
		return 1;
	}

	ret = gen_tbl(r, n, d->tbl, spread);
	if (ret) {
		printf("r=%d n=%d spread=%d: gen_tbl failed: %d\n",
		       r, n, spread, ret);
		free_dtbl(d);
		return 1;
	}

	for (k = r; k <= n; k++) {
		int idx = k - 1;
		int sz = dtbl_size(r, k);
		int *cnt = calloc(k, sizeof(*cnt));

		for (i = 0; i < sz; i++) {
			if (popcount(d->tbl[idx][i]) != r) {
				printf("r=%d n=%d spread=%d k=%d entry %d has %d bits, want %d\n",
				       r, n, spread, k, i, popcount(d->tbl[idx][i]), r);
				fail = 1;
			}
			for (s = 0; s < k; s++)
				if (d->tbl[idx][i] & (1 << s))
					cnt[s]++;
		}

		for (s = 0; s < k; s++) {
			int target = sz * r / k;

			if (cnt[s] != target) {
				printf("r=%d n=%d spread=%d k=%d server %d holds %d, want %d\n",
				       r, n, spread, k, s, cnt[s], target);
				fail = 1;
			}
		}

		/* redistribution check: going from k-1 to k servers must
		 * never move an entry between two servers that both
		 * already existed at k-1 servers.
		 */
		if (k > r) {
			int prev_idx = k - 2;
			int sz_old = dtbl_size(r, k - 1);
			int moved = 0, moved_between_old = 0;

			for (i = 0; i < sz; i++) {
				int old = d->tbl[prev_idx][i % sz_old];
				int new = d->tbl[idx][i];
				int new_masked = new & ~(1 << (k - 1));

				if (new_masked != old) {
					moved++;
					if (!(new & (1 << (k - 1))))
						moved_between_old++;
				}
			}

			if (moved_between_old) {
				printf("r=%d n=%d spread=%d k=%d: %d entries moved between old servers (should be 0)\n",
				       r, n, spread, k, moved_between_old);
				fail = 1;
			}
			printf("r=%d n=%d spread=%d k=%d: %d/%d entries touched (%.1f%%, ideal ~%.1f%%)\n",
			       r, n, spread, k, moved, sz, 100.0 * moved / sz, 100.0 / k);
		}

		free(cnt);
	}

	free_dtbl(d);
	return fail;
}

int main(void)
{
	int fail = 0, spread;

	for (spread = 0; spread <= 1; spread++) {
		fail |= check(1, 8, spread);
		fail |= check(2, 8, spread);
		fail |= check(3, 9, spread);
		fail |= check(4, 10, spread);
	}

	printf(fail ? "FAIL\n" : "OK\n");
	return fail;
}
