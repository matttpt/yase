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
		uint64_t end_byte,
		unsigned int end_bit,
		struct prime_set * set)
{
	uint64_t i, end_bit_absolute;
	uint8_t * seed_sieve;

	/* Calculate the absolute end bit */
	if(end_bit != 0)
	{
		end_bit_absolute = (end_byte - 1) * 8 + end_bit;
	}
	else
	{
		end_bit_absolute = end_byte * 8;
	}

	/* We don't bother to segment for this process.  We allocate the
	   sieve segment manually. */
	seed_sieve = malloc(end_byte);
	if(seed_sieve == NULL)
	{
		YASE_PERROR("malloc");
		abort();
	}

	/* Copy in pre-sieve data */
	presieve_copy(seed_sieve, 0, end_byte);

	/* Run the sieve */
	for(i = PRESIEVE_PRIMES + 2; i < end_byte * 8; i++)
	{
		if((seed_sieve[i / 8] & ((uint8_t) 1U << (i % 8))) == 0)
		{
			uint64_t prime, mult, byte;
			uint32_t prime_adj, wheel_idx;

			/* Mark multiples */
			prime     = (i / 8) * 30 + wheel30_offs[i % 8];
			mult      = prime * prime;
			prime_adj = (uint32_t) (prime / 30);
			byte      = mult / 30;
			wheel_idx = (i % 8) * 48 + wheel210_last_idx[prime % 210];

			/* If this prime is in the range that we need sieving primes,
			   record it. */
			if(i < end_bit_absolute)
			{
				if(prime < SMALL_THRESHOLD)
				{
					prime_set_add(set, prime, byte, (i % 8) * 9);
				}
				else
				{
					prime_set_add(set, prime, byte, wheel_idx);
				}
			}

			/* Sieve multiples for the purpose of finding more sieving
			   primes */
			while(byte < end_byte)
			{
				mark_multiple_210(seed_sieve, prime_adj,
				                  (uint32_t *) &byte, &wheel_idx);
			}
		}
	}

	/* Clean up */
	free(seed_sieve);
}
