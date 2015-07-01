/*
 * yase - Yet Another Sieve of Eratosthenes
 * seed.c: sieves the "seed primes" needed to sieve the entire interval
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
#include <string.h>
#include <yase.h>

/*
 * Sieves for the sieving primes.  end_byte is the first byte not
 * to check; end_bit is the first bit for which we don't need sieving
 * primes.  The long integer pointed to by count is increased with every
 * prime found.  Sieving primes found are added to the prime set given.
 */
void sieve_seed(
		unsigned long end_byte,
		unsigned long end_bit,
		unsigned long * count,
		struct prime_set * set)
{
	unsigned long i;
	unsigned char * seed_sieve;
	
	/* We don't bother to segment for this process.  We allocate the
	   sieve segment manually. */
	seed_sieve = malloc(end_byte);
	if(seed_sieve == NULL)
	{
		perror("malloc");
		abort();
	}

	/* Copy in pre-sieve data */
	presieve_copy(seed_sieve, 0, end_byte);

	/* Run the sieve */
	for(i = PRESIEVE_PRIMES + 2; i < end_byte * 8; i++)
	{
		if((seed_sieve[i / 8] & ((unsigned char) 1U << (i % 8))) == 0)
		{
			unsigned long prime, mult, prime_adj, byte;
			unsigned int wheel_idx;

			/* Count the prime */
			(*count)++;

			/* Mark multiples */
			prime     = (i / 8) * 30 + wheel30_offs[i % 8];
			mult      = prime * prime;
			prime_adj = prime / 30;
			byte      = mult / 30;

			/* If the prime is under the "small threshold," its
			   multiples are marked with a mod 30 wheel.  Otherwise, it
			   is sieved with a mod 210 wheel. */
			if(prime_adj < SMALL_THRESHOLD)
			{
				wheel_idx = (i % 8) * 9;
				while(byte < end_byte)
				{
					mark_multiple_30(seed_sieve, prime_adj, &byte,
					                 &wheel_idx);
				}
			}
			else
			{
				wheel_idx = (i % 8) * 48 + wheel210_last_idx[prime % 210];
				while(byte < end_byte)
				{
					mark_multiple_210(seed_sieve, prime_adj, &byte,
					                  &wheel_idx);
				}
			}

			/* If this prime is in the range that we need sieving primes,
			   record it. */
			if(i < end_bit)
			{
				/* Submit to the prime set */
				prime_set_add(set, prime_adj, byte, wheel_idx);
			}
		}
	}

	/* Clean up */
	free(seed_sieve);
}
