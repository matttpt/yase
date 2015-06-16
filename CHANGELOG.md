Change Log
==========

All notable changes to this project will be documented in this file.

## 0.1.0
### Added
 - Initial version.  Current features (from 0.1.0 `README`):
   - Efficient implementation of modulo 30 wheel factorization
   - Segmented sieve that fits in CPU L1 data cache
   - Use of an efficient lookup table to count primes after sieving
   - Processing of two sieving primes at once, to leverage
     instruction-level parallelism (adopted from primesieve)
