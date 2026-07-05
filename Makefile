# SPDX-License-Identifier: GPL-2.0-or-later
CFLAGS ?= -Wall -Wextra -O2

.PHONY: all test clean

all: dtbl

dtbl: dtbl.c dtbl.h
	$(CC) $(CFLAGS) -o $@ dtbl.c

test: test_dtbl
	./test_dtbl

test_dtbl: dtbl.o test_dtbl.c dtbl.h
	$(CC) $(CFLAGS) -c dtbl.c -o dtbl_lib.o -Dmain=unused_dtbl_main
	$(CC) $(CFLAGS) -c test_dtbl.c -o test_dtbl.o
	$(CC) dtbl_lib.o test_dtbl.o -o $@

dtbl.o: dtbl.c dtbl.h
	$(CC) $(CFLAGS) -c dtbl.c -o $@

clean:
	rm -f dtbl test_dtbl *.o
