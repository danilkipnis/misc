running-average.c
-----------------

Calculate running average while taking rest of division into account.

dtbl.c
------

See [dtbl.md](https://github.com/danilkipnis/misc/blob/develop/dtbl.md).

Build and print the tables for a given replication factor and server count::

	make
	./dtbl <replicas> <servers>

Run the invariant checks (replica count per entry, load balance, 1/n
redistribution) with::

	make test

Limited to at most 22 servers - the precomputed LCM table (`lcm[]` in
dtbl.c) only covers `lcm(1..22)`.

