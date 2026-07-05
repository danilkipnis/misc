// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdio.h>
#include <stdlib.h>

static unsigned long n, a, r;

int main(int argc, char *argv[]) {
	unsigned long i, s, x;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <number> [number ...]\n", argv[0]);
		return 1;
	}

	for (i = 1; i < (unsigned long)argc; i++) {
		x = strtoul(argv[i], NULL, 10);
		n++;
		s = a * (n - 1) + r + x;
		a = s / n;
		r = s % n;
	}

	printf("a = %lu + %lu / %lu\n", a, r, n);

	return 0;
}

