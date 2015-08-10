/*
 * yase - Yet Another Sieve of Eratosthenes
 * interval.c: handles sieving of an entire interval
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <yase.h>

/* Calculates the end bytes and bits for the seed sieve */
void calculate_seed_interval(
		uint64_t max,
		uint64_t * seed_end_byte,
		unsigned int * seed_end_bit)
{
	uint64_t seed_max;

	/*
	 * Calculate the max value to check when finding sieving primes.
	 * This is the largest value such that value * value <= max.
	 */
	seed_max = (uint64_t) sqrt((double) max);

	/* Find the end byte (the first byte that is not touched) for the
	   seed sieve.  This "magic expression" is explained in
	   interval_calculate(). */
	*seed_end_byte = ((seed_max + 1) + 28) / 30;

	/* Find the first bit of byte (seed_end_byte - 1) that we don't need
	   to check.  This will be 0 if we need the entire last byte. */
	if(seed_max % 30 == 0)
	{
		*seed_end_bit = 0;
	}
	else
	{
		*seed_end_bit += wheel30_last_idx[seed_max % 30] + 1;
	}
}

/* Calculates interval parameters given a start number and maxmium
   number to check */
void calculate_interval(
		uint64_t start,
		uint64_t max,
		struct interval * inter)
{
	/* Calculate the start byte and bit of the interval */
	if(start == 0 || start == 1)
	{
		/* Skip over 1 */
		inter->start_byte = 0;
		inter->start_bit  = 1;
	}
	else
	{
		inter->start_byte = start / 30;
		inter->start_bit  = wheel30_find_idx[start % 30];
	}

	/*
	 * Calculate the end byte (the first byte that is not touched).
	 * This expression is essentially a variation on the idea that to
	 * find p/q rounding up, you can round down (p+q-1)/q.  28 is used
	 * instead of 29 because the first bit in each byte is 30k+1.  Thus,
	 * 30k should still round down.  Try it with a few numbers--you'll
	 * see that it works.
	 */
	inter->end_byte = ((max + 1) + 28) / 30;

	/* Calculate the end bit of the interval */
	if(max % 30 != 0)
	{
		inter->end_bit = (wheel30_last_idx[max % 30] + 1) % 8;
	}
	else
	{
		inter->end_bit = 0;
	}
}

/* Sieves an entire interval, breaking it into segments.  The prime
   set must be initialized for the interval specified. */
void sieve_interval(
		const struct interval * inter,
		struct prime_set * set,
		uint64_t * count)
{
	unsigned int percent = 0;
	uint64_t next_byte = inter->start_byte;

	printf("Sieving . . . %u%%", 0);
	fflush(stdout);
	while(next_byte < inter->end_byte)
	{
		uint64_t seg_end_byte = next_byte + SEGMENT_BYTES;
		unsigned int seg_start_bit = 0, seg_end_bit = 0;
		unsigned int new_percent;

		/* If this is the first segment, load the right start bit */
		if(next_byte == inter->start_byte)
		{
			seg_start_bit = inter->start_bit;
		}

		/* If segment is longer than necessary, trim it and load the
		   right end bit */
		if(seg_end_byte > inter->end_byte)
		{ 
			seg_end_byte = inter->end_byte;
			seg_end_bit  = inter->end_bit;
		}

		/* Run the sieve on the segment */
		sieve_segment(next_byte,
		              seg_start_bit,
		              seg_end_byte,
		              seg_end_bit,
		              set,
		              count);

		/* Move forward */
		next_byte = seg_end_byte;
		prime_set_advance(set);

		/* Update the progress counter if the percentage has changed */
		new_percent = (unsigned int)
		              ((next_byte - inter->start_byte) * 100 /
		               (inter->end_byte - inter->start_byte));
		if(new_percent != percent)
		{
			percent = new_percent;
			printf("\rSieving . . . %u%%", percent);
			fflush(stdout);
		}
	}
	putchar('\n');
}
