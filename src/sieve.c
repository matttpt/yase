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

/* Sieve structure */
static unsigned char sieve[SEGMENT_BYTES];

/* Sieves a segment.  start and end are in bytes, and end_bit is the
   the bit after the final bit of the last byte checked that is needed.
   If end_bit == 0, the entire final byte checked is needed. */
void sieve_segment(
		unsigned long start,
		unsigned long end,
		unsigned long end_bit,
		struct prime * primes,
		unsigned long * count)
{
	unsigned long i;

	/* Copy in pre-sieve data */
	presieve_copy(sieve, start, end);

	/* Mark multiples of each sieving prime */
	while(primes != NULL)
	{
		if(primes->next_byte < end)
		{
			/* Attempt to do two at a time to leverage ILP.  This is
			   taken from primesieve. */
			if(primes->next != NULL && primes->next->next_byte < end)
			{
				struct prime * p1   = primes;
				struct prime * p2   = primes->next;
				unsigned long byte1 = p1->next_byte;
				unsigned long byte2 = p2->next_byte;
				unsigned int wi1    = p1->wheel_idx;
				unsigned int wi2    = p2->wheel_idx;
				while(byte1 < end && byte2 < end)
				{
					sieve[byte1 - start] |= wheel210[wi1].mask;
					byte1 += wheel210[wi1].delta_f * p1->prime_adj;
					byte1 += wheel210[wi1].delta_c;
					wi1 += wheel210[wi1].next;
					sieve[byte2 - start] |= wheel210[wi2].mask;
					byte2 += wheel210[wi2].delta_f * p2->prime_adj;
					byte2 += wheel210[wi2].delta_c;
					wi2 += wheel210[wi2].next;
				}
				while(byte1 < end)
				{
					sieve[byte1 - start] |= wheel210[wi1].mask;
					byte1 += wheel210[wi1].delta_f * p1->prime_adj;
					byte1 += wheel210[wi1].delta_c;
					wi1 += wheel210[wi1].next;
				}
				while(byte2 < end)
				{
					sieve[byte2 - start] |= wheel210[wi2].mask;
					byte2 += wheel210[wi2].delta_f * p2->prime_adj;
					byte2 += wheel210[wi2].delta_c;
					wi2 += wheel210[wi2].next;
				}
				p1->next_byte = byte1;
				p1->wheel_idx = wi1;
				p2->next_byte = byte2;
				p2->wheel_idx = wi2;

				/* Advance by one extra */
				primes = primes->next;
			}
			else
			{
				unsigned long byte     = primes->next_byte;
				unsigned int wheel_idx = primes->wheel_idx;
				while(byte < end)
				{
					sieve[byte - start] |= wheel210[wheel_idx].mask;
					byte += wheel210[wheel_idx].delta_f * primes->prime_adj;
					byte += wheel210[wheel_idx].delta_c;
					wheel_idx += wheel210[wheel_idx].next;
				}
				primes->next_byte = byte;
				primes->wheel_idx = wheel_idx;
			}
		}
		primes = primes->next;
	}

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
