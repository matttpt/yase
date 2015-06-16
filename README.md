yase - Yet Another Sieve of Eratosthenes
========================================

yase is a Sieve of Eratosthenes-based single-threaded prime finding
program.  It is invoked with a single argument, which gives the highest
number to be checked for primality.  It currently employs the following
methods to speed up its computations:

 - Efficient implementation of modulo 30 wheel factorization
 - Segmented sieve that fits in CPU L1 data cache
 - Use of an efficient lookup table to count primes after sieving
 - Processing of two sieving primes at once, to leverage
   instruction-level parallelism

Additionally, each byte of the bit array used to sieve for primes
covers a range of 30 numbers.  With a 32 KB sieve (fitting a common CPU
L1 data cache size), this gives a total range of 983,040 numbers checked
per segment sieved.

Many of the ideas in this program are taken from other prime-sieving
programs, especially [primesieve] (http://primesieve.org/).

yase is built with a `Makefile`.  Before you build, copy
`config.mk.default` to `config.mk` and make any edits you desire.
Then:

 - `make all` builds the yase executable (same as `make yase`)
 - `make yase` builds the yase executable (same as `make all`)
 - `make clean` cleans up after a build, deleting the object files and
   generated executable
 - `make source-dist` takes the current set of source files and produces
    a `.tar.gz` for publication/redistribution

The build options/parameters for yase can be changed by editing
`config.mk`.  `config.mk.default` contains reasonable default values for
each variable and a description of what each controls.  If you want to
change the C compiler, compilation flags, or sieve segment size, take a
look here.
