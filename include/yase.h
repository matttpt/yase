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

/* Number of primes skipped by the primary sieving wheel (mod 210) */
#define WHEEL_PRIMES_SKIPPED (4U)

/*
 * Threshold below which to use the small prime sieve.  (This has
 * pretty much been determined experimentally.)  The threshold is
 * expressed in terms of the adjusted prime value (prime / 30).  Primes
 * with the adjusted value < SMALL_THRESHOLD will be sieved with the
 * small prime sieve.
 */
#define SMALL_THRESHOLD (SEGMENT_BYTES / 64)

/*
 * This is based HEAVILY off the way that the "primesieve" program
 * implements wheel factorization.  However, I have derived the
 * mathematics to generate the tables myself.
 */
struct wheel_elem
{
	unsigned char delta_f; /* Delta factor              */
	unsigned char delta_c; /* Delta correction          */
	unsigned char mask;    /* Bitmask to set bit        */
	  signed char next;    /* Offset to next wheel_elem */
};

/* Exposed wheel tables.  Even though "wheel30", "wheel210" and
   "wheel210_last_idx" are not const (so that wheel_init(void) can
   generate them), obviously don't modify them. */
extern struct wheel_elem   wheel30[64];
extern const unsigned char wheel30_offs[8];
extern const unsigned char wheel30_deltas[8];
extern const unsigned char wheel30_last_idx[30];
extern struct wheel_elem   wheel210[384];
extern const unsigned char wheel210_offs[48];
extern const unsigned char wheel210_deltas[48];
extern unsigned char       wheel210_last_idx[210];

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

/* Structure to hold information about a sieving prime, and which
   multiple needs to be marked next */
struct prime
{
	unsigned long  next_byte; /* Next byte to mark                */
	unsigned long  prime_adj; /* Prime divided by 30              */
	struct prime * next;      /* Next sieving prime in list       */
	unsigned int   wheel_idx; /* Current index in the wheel table */
};

/* Finds the sieving primes */
void sieve_seed(
		unsigned long max,
		unsigned long * count,
		unsigned long * next_byte,
		struct prime ** small_primes_out,
		struct prime ** large_primes_out);
void sieve_segment(
		unsigned long start,
		unsigned long end,
		unsigned long end_bit,
		struct prime * small_primes,
		struct prime * large_primes,
		unsigned long * count);

/**********************************************************************\
 * Pre-sieve mechanism                                                *
\**********************************************************************/

void presieve_init(void);
void presieve_cleanup(void);
void presieve_copy(
		unsigned char * sieve,
		unsigned long start,
		unsigned long end);

#endif /* !YASE_H */
