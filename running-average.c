// SPDX-License-Identifier: GPL-2.0-or-later
#include <stdio.h>

static unsigned long n, a, r;

int main(int argc, char *argv[]) {
	unsigned long i, s, x[] = {1, 2, 3, 4, 5, 6};

	for (i = 0; i < sizeof(x) / sizeof(x[0]); i++) {
		n++;
		s = a * (n - 1) + r + x[i];
		a = s / n;
		r = s % n;
	}

	printf("a = %lu + %lu / %lu\n", a, r, n);

	return 0;
}

