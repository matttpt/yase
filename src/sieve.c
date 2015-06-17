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
		struct multiple * multiples,
		unsigned long * count)
{
	unsigned long i;

	/* Copy in pre-sieve data */
	presieve_copy(sieve, start, end);

	/* Mark multiples */
	while(multiples != NULL)
	{
		if(multiples->next_byte < end)
		{
			/* Attempt to do two at a time to leverage ILP.  This is
			   taken from primesieve. */
			if(multiples->next != NULL && multiples->next->next_byte < end)
			{
				struct multiple * m1   = multiples;
				struct multiple * m2   = multiples->next;
				unsigned long byte1    = m1->next_byte;
				unsigned long byte2    = m2->next_byte;
				struct wheel_elem * e1 = m1->stored_e;
				struct wheel_elem * e2 = m2->stored_e;
				while(byte1 < end && byte2 < end)
				{
					sieve[byte1 - start] |= e1->mask;
					sieve[byte2 - start] |= e2->mask;
					byte1 += e1->delta_f * m1->prime_adj + e1->delta_c;
					byte2 += e2->delta_f * m2->prime_adj + e2->delta_c;
					e1 += e1->next;
					e2 += e2->next;
				}
				while(byte1 < end)
				{
					sieve[byte1 - start] |= e1->mask;
					byte1 += e1->delta_f * m1->prime_adj + e1->delta_c;
					e1 += e1->next;
				}
				while(byte2 < end)
				{
					sieve[byte2 - start] |= e2->mask;
					byte2 += e2->delta_f * m2->prime_adj + e2->delta_c;
					e2 += e2->next;
				}
				m1->next_byte = byte1;
				m1->stored_e  = e1;
				m2->next_byte = byte2;
				m2->stored_e  = e2;

				/* Advance by one extra */
				multiples = multiples->next;
			}
			else
			{
				unsigned long byte    = multiples->next_byte;
				struct wheel_elem * e = multiples->stored_e;
				while(byte < end)
				{
					sieve[byte - start] |= e->mask;
					byte += e->delta_f * multiples->prime_adj + e->delta_c;
					e += e->next;
				}
				multiples->next_byte = byte;
				multiples->stored_e  = e;
			}
		}
		multiples = multiples->next;
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
