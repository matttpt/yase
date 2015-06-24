Change Log
==========

All notable changes to this project will be documented in this file.

## 0.2.0 - 2015-06-24
### Added
 - Pre-sieving of multiples of small sieving primes.  Set
   `PRESIEVE_PRIMES` in `config.mk` to control this new behavior.

### Changed
 - Use a modulo 210 wheel instead of a modulo 30 wheel during sieving
   for improved performance.
 - Adjust multiple marking for slightly improved performance.
 - Build system fixes and improvements:
    - Fix `make clean` by using `rm -rf` instead of `rmdir`.
    - Make C sources, `params.h`, and yase binary depend on `config.mk`
      in `Makefile` so configuration changes trigger appropriate
      recompilation.
 - Initialize sieve bit array to zeros (was previously using
   uninitialized memory!).
 - Explicitly use `signed char` in appropriate spots to fix yase on ARM.

## 0.1.0 - 2015-06-16
### Added
 - Initial version.  Current features (from 0.1.0 `README`):
   - Efficient implementation of modulo 30 wheel factorization
   - Segmented sieve that fits in CPU L1 data cache
   - Use of an efficient lookup table to count primes after sieving
   - Processing of two sieving primes at once, to leverage
     instruction-level parallelism (adopted from primesieve)
