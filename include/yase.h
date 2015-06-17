/*
 * yase - Yet Another Sieve of Eratosthenes
 * yase.h: header for all yase source files
 *
 * Copyright (c) 2015 Matthew Ingwersen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef YASE_H
#define YASE_H

/* Include the compile parameters and version headers */
#include <params.h>
#include <version.h>

/**********************************************************************\
 * Wheel structures                                                   *
\**********************************************************************/

/* Number of primes skipped by the wheel */
#define WHEEL_PRIMES_SKIPPED (3U)

/*
 * This is based HEAVILY off the way that the "primesieve" program
 * implements wheel factorization.  However, I have derived the
 * mathematics to generate the tables myself.
 */
struct wheel_elem
{
	unsigned char delta_f; /* Delta factor              */
	unsigned char delta_c; /* Delta correction          */
	unsigned char mask;    /* Bitmask to set bit */
	         char next;    /* Offset to next wheel_elem */
};

/* Exposed wheel tables.  Even though "wheel" is not const (so that
   wheel_init(void) can generate it), obviously don't modify it. */
extern struct wheel_elem    wheel[64];
extern const unsigned char wheel_offs[8];
extern const unsigned char wheel_deltas[8];
extern const unsigned char wheel_last_idx[30];

/* Wheel initialization routine */
void wheel_init(void);

/**********************************************************************\
 * Population count                                                   *
\**********************************************************************/

/*
 * Take heed, this population count table is actually the number of
 * *unset* bits, because primes are unset.  You've been warned.
 *
 * Just like the "wheel" table above, "popcnt" is not const but
 * obviously don't modify it!
 */
extern unsigned char popcnt[256];

/* Initializes the population count table */
void popcnt_init(void);

/**********************************************************************\
 * Sieves                                                             *
\**********************************************************************/

/* Structure to hold information about a multiple of a sieving prime */
struct multiple
{
	unsigned long       next_byte; /* Next byte to mark     */
	unsigned long       prime_adj; /* Prime divided by 30   */
	struct wheel_elem * stored_e;  /* Next wheel_elem       */
	struct multiple *   next;      /* Next multiple in list */
};

/* Finds the sieving primes */
struct multiple * sieve_seed(
		unsigned long max,
		unsigned long * count,
		unsigned long * next_byte);
void sieve_segment(
		unsigned long start,
		unsigned long end,
		unsigned long end_bit,
		struct multiple * multiples,
		unsigned long * count);

/**********************************************************************\
 * Pre-sieve mechanism                                                *
\**********************************************************************/

void presieve_init(void);
unsigned long presieve_max_prime(void);
void presieve_copy(
		unsigned char * sieve,
		unsigned long start,
		unsigned long end);

#endif /* !YASE_H */
