/*
 * yase - Yet Another Sieve of Eratosthenes
 * sieve.c: sieves the interval requested
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

#include <string.h>
#include <yase.h>

/* Sieve bit array */
static unsigned char sieve[SEGMENT_BYTES];

/*
 * process_small_prime() marks the multiples of a single small sieving
 * prime using a highly-optimized set of mod 30 marking loops.
 *
 * This code is based closely on the unbelievably fast prime-sieving
 * program "primesieve," which employs a set of loops like this to mark
 * multiples of small primes.  Thank you, primesieve!
 *
 * Before looking at this, make sure you understand how the "regular"
 * method of marking primes on a wheel works.  This is fundamentally the
 * same algorithm turned into a ridiculous bunch of loops.
 *
 * The loops essentially encode the information from the wheel30 table
 * into the generated assembly, making it much faster provided that the
 * sieving prime has enough multiples to mark on the segment to outweigh
 * the overhead.  (This is why it is only used for smaller sieving
 * primes.)
 *
 * The switch statement is used to jump (a la Duff's Device) into the
 * correct spot in the correct loop for whatever "cycle" of the wheel
 * the prime is in, based on the next multiple's wheel_idx.  The program
 * flow then takes care of (metaphorically) "updating the wheel_idx"
 * from then on.
 *
 * Normally, the current byte value must be checked before marking each
 * multiple, to ensure that the byte to mark is still in the segment
 * being sieved.  However, once the wheel index hits the beginning of
 * the cycle, the program enters a loop that marks 8 multiples per
 * iteration without checking the byte value until there are no longer
 * 8 multiples remaining on the segment to be sieved.  The program
 * then returns to checking the byte value before marking each multiple
 * until it reaches the end of the segment.
 *
 * The following table and macros are used to eliminate the need to
 * write each loop out manually.  Because all of the inputs to these
 * expressions are known constants, the compiler will evaluate them
 * into constants in the generated assembly.
 */

/* Turns wheel offsets into bitmasks for marking multiples.  Only the
   non-zero entries will actually be needed. */
static const unsigned char offs_to_mask[30] =
	{ 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
	  0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x10, 0x00, 0x20,
	  0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80 };

/* Macro to calculate the wheel delta correction */
#define DC(df, i, j) (((i * (j + df)) / 30) - ((i * j) / 30))

/* Macro to calculate the marking bitmask */
#define MASK(i, j) (offs_to_mask[i * j % 30])

/* Generates the code to "check and step" through the cycle, making
   sure not to run over the end byte limit */
#define BUILD_CHECK_AND_MARK(n, df, i, j)        \
	if(byte >= end - start) {                    \
		prime->next_byte = byte - (end - start); \
		prime->wheel_idx = n;                    \
		return;                                  \
	}                                            \
	sieve[byte] |= MASK(i, j);                   \
	byte += (adj * df) + DC(df, i, j);

/* Generates the code to handle a cycle of 8. n = the starting wheel
   index, i = the wheel offset associated with the prime/cycle */
#define BUILD_LOOP(n, i)                                          \
	for(;;) {                                                     \
	case n:                                                       \
		while(byte + adj * 28 + i < end - start) {                \
			sieve[byte                          ] |= MASK(i,  1); \
			sieve[byte + adj * 6  + DC( 6, i, 1)] |= MASK(i,  7); \
			sieve[byte + adj * 10 + DC(10, i, 1)] |= MASK(i, 11); \
			sieve[byte + adj * 12 + DC(12, i, 1)] |= MASK(i, 13); \
			sieve[byte + adj * 16 + DC(16, i, 1)] |= MASK(i, 17); \
			sieve[byte + adj * 18 + DC(18, i, 1)] |= MASK(i, 19); \
			sieve[byte + adj * 22 + DC(22, i, 1)] |= MASK(i, 23); \
			sieve[byte + adj * 28 + DC(28, i, 1)] |= MASK(i, 29); \
			byte += adj * 30 + i;                                 \
		}                                                         \
	            BUILD_CHECK_AND_MARK(n    , 6, i,  1)             \
	case n + 1: BUILD_CHECK_AND_MARK(n + 1, 4, i,  7)             \
	case n + 2: BUILD_CHECK_AND_MARK(n + 2, 2, i, 11)             \
	case n + 3: BUILD_CHECK_AND_MARK(n + 3, 4, i, 13)             \
	case n + 4: BUILD_CHECK_AND_MARK(n + 4, 2, i, 17)             \
	case n + 5: BUILD_CHECK_AND_MARK(n + 5, 4, i, 19)             \
	case n + 6: BUILD_CHECK_AND_MARK(n + 6, 6, i, 23)             \
	case n + 7: BUILD_CHECK_AND_MARK(n + 7, 2, i, 29)             \
	}

/* process_small_prime() itself - but all of the real code is in the
   macros */
static inline void process_small_prime(
		unsigned long start,
		unsigned long end,
		struct prime * prime)
{
	/* From prime structure */
	unsigned long byte = prime->next_byte;
	unsigned long adj  = prime->prime_adj;

	/* Jump to the correct spot */
	switch(prime->wheel_idx)
	{
		/* One loop per wheel cycle */
		BUILD_LOOP( 0,  1)
		BUILD_LOOP( 8,  7)
		BUILD_LOOP(16, 11)
		BUILD_LOOP(24, 13)
		BUILD_LOOP(32, 17)
		BUILD_LOOP(40, 19)
		BUILD_LOOP(48, 23)
		BUILD_LOOP(56, 29)
	}
}

/* Processes small sieving primes using the very fast mod 30 loop */
static inline void process_small_primes(
		unsigned long start,
		unsigned long end,
		struct prime_set * set)
{
	struct prime * primes = prime_set_small(set);
	while(primes != NULL)
	{
		process_small_prime(start, end, primes);
		primes = primes->next;
	}
}

/* Processes large sieving primes, marking multiples of two at a time
   if possible to leverage instruction-level parallelism */
static inline void process_large_primes(
		unsigned long start,
		unsigned long end,
		struct prime_set * set)
{
	struct prime * primes, * p1, * p2, * save_p1, * save_p2;
	unsigned long byte1, byte2, adj1, adj2, lim;
	unsigned int wi1, wi2;

	/* Fetch the list we need */
	primes = prime_set_current(set);

	/* If there are no large primes, return */
	if(primes == NULL)
	{
		return;
	}

	/* Find the sieve byte limit */
	lim = end - start;

	/* Mark multiples, attempting to process two primes at once to
	   leverage ILP.  (This idea is taken from primesieve.) */
	p1 = primes;
	p2 = primes->next;
	while(p1 != NULL && p2 != NULL)
	{
		/* Load primes */
		byte1 = p1->next_byte;
		adj1  = p1->prime_adj;
		wi1   = p1->wheel_idx;
		byte2 = p2->next_byte;
		adj2  = p2->prime_adj;
		wi2   = p2->wheel_idx;

		/* For as long as possible, do both together */
		while(byte1 < lim && byte2 < lim)
		{
			mark_multiple_210(sieve, adj1, &byte1, &wi1);
			mark_multiple_210(sieve, adj2, &byte2, &wi2);
		}

		/* Finish first if necessary */
		while(byte1 < lim)
		{
			mark_multiple_210(sieve, adj1, &byte1, &wi1);
		}

		/* Finish second if necessary */
		while(byte2 < lim)
		{
			mark_multiple_210(sieve, adj2, &byte2, &wi2);
		}

		/* Remember old two primes */
		save_p1 = p1;
		save_p2 = p2;

		/* Fetch two more primes */
		p1 = p2->next;
		p2 = (p1 == NULL ? NULL : p1->next);

		/* Save old two back to the set */
		prime_set_save(set, save_p1, byte1, wi1);
		prime_set_save(set, save_p2, byte2, wi2);
	}

	/* If there are an odd number of primes, finish the last one now */
	if(p1 != NULL)
	{
		byte1 = p1->next_byte;
		adj1  = p1->prime_adj;
		wi1   = p1->wheel_idx;
		while(byte1 < lim)
		{
			mark_multiple_210(sieve, adj1, &byte1, &wi1);
		}
		prime_set_save(set, p1, byte1, wi1);
	}
}

/* Sieves a segment.  start and end are in bytes, and end_bit is the
   the bit after the final bit of the last byte checked that is needed.
   If end_bit == 0, the entire final byte checked is needed. */
void sieve_segment(
		unsigned long start,
		unsigned long end,
		unsigned long end_bit,
		struct prime_set * set,
		unsigned long * count)
{
	unsigned long i;

	/* Copy in pre-sieve data */
	presieve_copy(sieve, start, end);

	/* Mark multiples of each sieving prime */
	process_small_primes(start, end, set);
	process_large_primes(start, end, set);

	/* Count primes */
	for(i = 0; i < end - start; i++)
	{
		(*count) += popcnt[sieve[i]];
	}

	/* Prune any extra bits we didn't need in the last byte */
	if(end_bit != 0)
	{
		unsigned char mask = (unsigned char) ~(0xFF << end_bit);
		(*count) -= popcnt[sieve[end - start - 1] | mask];
	}
}
