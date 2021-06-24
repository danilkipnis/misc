dtbl
====

Generate explicit distribution tables for virtual blocks depending on the
overall number of servers available in the system and the desired number of
replicas in the system. This is consistent hashing without hashing.

For a given virtual block number _b_ and a desired replication factor (number
of replicas in the system) _f_ and an overall number of machines in the system
_n_, one would like to know the list of servers the block has to be sent to in
such a way, that the redistribution in case of scaling the system up or down is
strictly optimal (i.e. _1/n_).

Each element of table _t(f, n)_ for number of replicas _f_ and number of machines
_n_ is a bitmap, where one bit corresponds to one machine in the system. If the
table has _s_ elements, then block _b_ has to be sent to machines _t[f, n, b%s]_.

The strictly optimal redistribution at virtually zero computational cost comes
with high memory costs.

The size of table (actually number of elements in the array) _t(f,n)_ is equal to
_LCM{1,2..n}(n)/f_ sequence.

Examples
--------

Mirror with a single copy (raid0) in a cluster with 1,2,3... machines::

	1 x.

	1  x . .
	2 x . .

	1    x x   . .
	2   x x   . .
	3 xx    ..

	1      x   x x     .   . .     .   . .     .   . .     .   . .
	2     x   x x     .   . .     .   . .     .   . .     .   . .
	3  x    xx     .    ..     .    ..     .    ..     .    ..
	4 x xx        . ..        . ..        . ..        . ..

	1                  x   x x     x   x x     x   x x     x   x x
	2                 x   x x     x   x x     x   x x     x   x x
	3              x    xx     x    xx     x    xx     x    xx
	4             x xx        x xx        x xx        x xx
	5 xxxxxxxxxxxx

	1                        x     x   x x     x   x x     x   x x
	2                       x     x   x x     x   x x     x   x x
	3                    x     x    xx     x    xx     x    xx
	4                x        x xx        x xx        x xx
	5   xxxxxxxxxx
	6 xx          xxx xxx xx

Mirror with 2 copies (raid1) present in a cluster with 2,3,... nodes::

	1 x..
	2 x..

	1  xx ..
	2 x x. .
	3 xx ..

	1   x xx  . ..  . ..  . ..  . ..
	2   xx x  .. .  .. .  .. .  .. .
	3 xx  x ..  . ..  . ..  . ..  .
	4 xx x  .. .  .. .  .. .  .. .

	1         x xx  x xx  x xx  x xx
	2   x  x     x  xx x  xx x  xx x
	3     x  x  x xx  x xx  x xx  x
	4 xx x  x  x     x  xx x  xx x
	5 xxxxxxxxxx  xx

	1            x  x xx  x xx  x xx
	2            x  xx x  xx x  xx x
	3           x xx  x xx  x xx  x
	4    x  x  x     x  xx x  xx x
	5 xxx xxxxx   xx
	6 xxxxxx xxxx

Three copies in a cluster with n nodes (n>=3)::

	1 x...
	2 x...
	3 x...

	1  xxx ... ... ... ...
	2 x xx. ... ... ... ..
	3 xx x.. ... ... ... .
	4 xxx ... ... ... ...

	1      xxx xxx xxx xxx
	2   xx   xx xxx xxx xx
	3 xx xx    x xxx xxx x
	4 xxx xxx x    xx xxx
	5 xxxxxxxxxxx x

	1        x xxx xxx xxx       . ... ... ...       . ... ... ...       . ... ... ...       . ... ... ...       . ... ... ...       . ... ... ...
	2        xx xxx xxx xx       .. ... ... ..       .. ... ... ..       .. ... ... ..       .. ... ... ..       .. ... ... ..       .. ... ... ..
	3    xx    x xxx xxx x   ..    . ... ... .   ..    . ... ... .   ..    . ... ... .   ..    . ... ... .   ..    . ... ... .   ..    . ... ... .
	4 xxx  xx      xx xxx ...  ..      .. ... ...  ..      .. ... ...  ..      .. ... ...  ..      .. ... ...  ..      .. ... ...  ..      .. ...
	5 xxxxxxx x x x       ....... . . .       ....... . . .       ....... . . .       ....... . . .       ....... . . .       ....... . . .
	6 xxxxxxxxxx          ..........          ..........          ..........          ..........          ..........          ..........

	1                            x xxx xxx xxx       x xxx xxx xxx       x xxx xxx xxx       x xxx xxx xxx       x xxx xxx xxx       x xxx xxx xxx
	2        x  xx  xx  xx                x xx       xx xxx xxx xx       xx xxx xxx xx       xx xxx xxx xx       xx xxx xxx xx       xx xxx xxx xx
	3          x xxx xxx x           xx  x        x    x xxx xxx x   xx    x xxx xxx x   xx    x xxx xxx x   xx    x xxx xxx x   xx    x xxx xxx x
	4              xx xxx              xx xxx xxx  xx      xx xxx xxx  xx      xx xxx xxx  xx      xx xxx xxx  xx      xx xxx xxx  xx      xx xxx
	5 xxxxxxx x x x       xxxxxxx x x x          x                 xxxxxx x x x       xxxxxxx x x x       xxxxxxx x x x       xxxxxxx x x x
	6 xxxxxxxxxx          xxxxxxxxxx          xxxxxxx x           x        x          xxxxxxxxxx          xxxxxxxxxx          xxxxxxxxxx
	7 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxx x       xxxxxxxxx

