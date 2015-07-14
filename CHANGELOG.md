Change Log
==========

All notable changes to this project will be documented in this file.

## 0.3.0 - 2015-07-14
### Added
 - Introduce a highly-optimized sieve for small sieving primes.  The
   threshold between small and large sieving primes is configurable
   via the new setting `SMALL_THRESHOLD_FACTOR`.
 - Sort large sieving primes based on the index of their next multiple
   for a significant performance boost.  How many primes are stored per
   bucket is configurable via the new setting `BUCKET_PRIMES`.
 - Store sieving primes in linked lists of "buckets," each containing
   many sieving primes, for improved performance.
 - Add `--help` and `--version` flags, with appropriate informational
   messages.
 - Allow mathematical expressions to be used for the values specified on
   the command line.  Supported operators are addition (`+`),
   subtraction (`-`), multiplication (`*`), and exponentiation (`**` or
   `^`).  "E-notatation" for scientific notation (e.g. `1e10`) is also
   supported.

### Changed
 - Convert build system to CMake for increased portability:
   - `config.mk` is now `config.cmake` and written in CMake format.
     `config.mk.default` is similarly renamed.  You will have to convert
      your configuration before compiling.  If you choose to perform an
      out-of-source build (now possible!), `config.cmake` is placed in
      the build directory, not the source distribution.
   - There is no longer a `make source-dist`, but you can now make both
     source and binary distributions with CPack.
 - Use fixed-width integer types, allowing yase to sieve much higher
   than 2^32 on 32-bit systems, albeit somewhat slower than on 64-bit
   machines.
 - Various tweaks for improved performance.
 - De-duplicate multiple marking code, move argument processing to
   `src/args.c`, and other code cleanup.
 - Show (invoked) program name in all error messages.

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
