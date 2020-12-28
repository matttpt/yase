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
static uint8_t sieve[SEGMENT_BYTES];

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
static const uint8_t offs_to_mask[30] =
	{ 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x00,
	  0x00, 0xFB, 0x00, 0xF7, 0x00, 0x00, 0x00, 0xEF, 0x00, 0xDF,
	  0x00, 0x00, 0x00, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F };

/* Macro to calculate the wheel delta correction */
#define DC(df, i, j) (((i * (j + df)) / 30) - ((i * j) / 30))

/* Macro to calculate the marking bitmask */
#define MASK(i, j) (offs_to_mask[i * j % 30])

/* Generates the code to "check and step" through the cycle, making
   sure not to run over the end byte limit */
#define BUILD_CHECK_AND_MARK(n, df, i, j)                \
	if(byte >= lim) {                                    \
		prime->next_byte = (uint64_t) (byte - lim);      \
		prime->wheel_idx = n;                            \
		return;                                          \
	}                                                    \
	*byte &= MASK(i, j);                                 \
	byte += (adj * df) + DC(df, i, j);

/* Generates the code to handle a cycle of 8. n = the starting wheel
   index, i = the wheel offset associated with the prime/cycle */
#define BUILD_LOOP(n, i)                                  \
	for(;;) {                                             \
	case n:                                               \
		while(byte < lim - adj * 28 - i) {                \
			byte[                      0] &= MASK(i,  1); \
			byte[adj * 6  + DC( 6, i, 1)] &= MASK(i,  7); \
			byte[adj * 10 + DC(10, i, 1)] &= MASK(i, 11); \
			byte[adj * 12 + DC(12, i, 1)] &= MASK(i, 13); \
			byte[adj * 16 + DC(16, i, 1)] &= MASK(i, 17); \
			byte[adj * 18 + DC(18, i, 1)] &= MASK(i, 19); \
			byte[adj * 22 + DC(22, i, 1)] &= MASK(i, 23); \
			byte[adj * 28 + DC(28, i, 1)] &= MASK(i, 29); \
			byte += adj * 30 + i;                         \
		}                                                 \
	            BUILD_CHECK_AND_MARK(n    , 6, i,  1)     \
	case n + 1: BUILD_CHECK_AND_MARK(n + 1, 4, i,  7)     \
	case n + 2: BUILD_CHECK_AND_MARK(n + 2, 2, i, 11)     \
	case n + 3: BUILD_CHECK_AND_MARK(n + 3, 4, i, 13)     \
	case n + 4: BUILD_CHECK_AND_MARK(n + 4, 2, i, 17)     \
	case n + 5: BUILD_CHECK_AND_MARK(n + 5, 4, i, 19)     \
	case n + 6: BUILD_CHECK_AND_MARK(n + 6, 6, i, 23)     \
	case n + 7: BUILD_CHECK_AND_MARK(n + 7, 2, i, 29)     \
	}

/* process_small_prime() itself - but all of the real code is in the
   macros */
static inline void process_small_prime(
		struct prime * prime)
{
	/* From prime structure */
	uint8_t * byte = &sieve[prime->next_byte];
	uint8_t * lim  = &sieve[SEGMENT_BYTES];
	uint32_t  adj  = prime->prime_adj;

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
		struct prime_set * set)
{
	struct bucket * bucket = set->small;
	while(bucket != NULL)
	{
		struct prime * prime = bucket->primes;
		struct prime * p_end = &bucket->primes[bucket->count];
		while(prime < p_end)
		{
			process_small_prime(prime);
			prime++;
		}
		bucket = bucket->next;
	}
}

/* Processes one bucket of large sieving primes */
static inline void process_large_prime_bucket(
		struct prime_set * set,
		struct bucket * bucket)
{
	struct prime * next_prime, * end_prime, * p1, * p2;
	uint32_t byte1, byte2, adj1, adj2, wi1, wi2;

	/* If there are no large primes in the bucket, return */
	if(bucket->count == 0)
	{
		return;
	}

	/* Setup next and end primes */
	next_prime = bucket->primes;
	end_prime  = &bucket->primes[bucket->count];

	/* Mark multiples, attempting to process two primes at once to
	   leverage ILP.  (This idea is taken from primesieve.) */
	p1 = next_prime;
	next_prime++;
	p2 = (next_prime < end_prime ? next_prime++ : NULL);
	while(p1 != NULL && p2 != NULL)
	{
		/* Load primes */
		byte1 = (uint32_t) p1->next_byte;
		adj1  = p1->prime_adj;
		wi1   = p1->wheel_idx;
		byte2 = (uint32_t) p2->next_byte;
		adj2  = p2->prime_adj;
		wi2   = p2->wheel_idx;

		/* For as long as possible, do both together */
		while(byte1 < SEGMENT_BYTES && byte2 < SEGMENT_BYTES)
		{
			mark_multiple_210(sieve, adj1, &byte1, &wi1);
			mark_multiple_210(sieve, adj2, &byte2, &wi2);
		}

		/* Finish first if necessary */
		while(byte1 < SEGMENT_BYTES)
		{
			mark_multiple_210(sieve, adj1, &byte1, &wi1);
		}

		/* Finish second if necessary */
		while(byte2 < SEGMENT_BYTES)
		{
			mark_multiple_210(sieve, adj2, &byte2, &wi2);
		}

		/* Save old two back to the set */
		prime_set_save(set, adj1, (uint64_t) byte1, wi1);
		prime_set_save(set, adj2, (uint64_t) byte2, wi2);

		/* Fetch two more primes */
		p1 = (next_prime < end_prime ? next_prime++ : NULL);
		p2 = (next_prime < end_prime ? next_prime++ : NULL);
	}

	/* If there are an odd number of primes, finish the last one now */
	if(p1 != NULL)
	{
		byte1 = p1->next_byte;
		adj1  = p1->prime_adj;
		wi1   = p1->wheel_idx;
		while(byte1 < SEGMENT_BYTES)
		{
			mark_multiple_210(sieve, adj1, &byte1, &wi1);
		}
		prime_set_save(set, adj1, (uint64_t) byte1, wi1);
	}
}

/* Processes large sieving primes, marking multiples of two at a time
   if possible to leverage instruction-level parallelism */
static inline void process_large_primes(
		struct prime_set * set)
{
	struct bucket * bucket, * to_return;

	/* Fetch the list we need */
	bucket = set->lists[0];

	/* Process each bucket */
	while(bucket != NULL)
	{
		process_large_prime_bucket(set, bucket);
		to_return = bucket;
		bucket    = bucket->next;
		prime_set_bucket_return(set, to_return);
	}
}

/* Sieves a segment.  start and end are in bytes, and end_bit is the
   the bit after the final bit of the last byte checked that is needed.
   If end_bit == 0, the entire final byte checked is needed. */
void sieve_segment(
		uint64_t start,
		unsigned int start_bit,
		uint64_t end,
		unsigned int end_bit,
		struct prime_set * set,
		uint64_t * count)
{
	/* Copy in pre-sieve data */
	presieve_copy(sieve, start, end);

	/* Mark multiples of each sieving prime */
	process_small_primes(set);
	process_large_primes(set);

	/* Count primes */
	(*count) += popcnt(sieve, start_bit, (unsigned long) (end - start),
	                   end_bit);
}
