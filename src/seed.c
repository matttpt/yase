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

/* Sieves for the sieving primes */
struct multiple * sieve_seed(
		unsigned long max,
		unsigned long * count,
		unsigned long * next_byte)
{
	unsigned long i, max_seed, final_byte, final_bit;
	struct multiple * multiples = NULL;
	unsigned char * seed_sieve;

	/* This is the largest value such that value * value <= max.
	   To avoid floating-point arithmetic we just increase the value
	   until the condition is no longer met. */
	max_seed = 0;
	while(max_seed * max_seed <= max)
	{
		max_seed++;
	}
	max_seed--;

	/* Find the byte of and bit of/before the maximum seed value */
	final_byte = max_seed / 30;
	*next_byte = final_byte + 1;
	
	/* Find the last bit up to which we need sieving primes for */
	final_bit = final_byte * 8 + wheel_last_idx[max_seed % 30];

	/* We don't bother to segment for this process.  We allocate the
	   sieve segment manually. */
	seed_sieve = malloc(final_byte + 1);
	if(seed_sieve == NULL)
	{
		perror("malloc");
		abort();
	}

	/* Zero the sieve */
	memset(seed_sieve, 0, final_byte + 1);

	/* Run the sieve */
	for(i = 1; i < (final_byte + 1) * 8; i++)
	{
		if((seed_sieve[i / 8] & ((unsigned char) 1U << (i % 8))) == 0)
		{
			unsigned long prime, mult, prime_adj, byte;
			struct wheel_elem * e;
			struct multiple * mult_s;

			/* Count the prime */
			(*count)++;

			/* Mark multiples */
			prime     = (i / 8) * 30 + wheel_offs[i % 8];
			mult      = prime * prime;
			prime_adj = prime / 30;
			byte      = mult / 30;
			e         = &wheel[(i % 8) * 8];
			while(byte <= final_byte)
			{
				seed_sieve[byte] |= e->mask;
				byte += e->delta_f * prime_adj + e->delta_c;
				e += e->next;
			}

			/* If this prime is in the range that we need sieving primes,
			   record it. */
			if(i <= final_bit)
			{
				/* Allocate new multiple structure */
				mult_s = malloc(sizeof(struct multiple));
				if(mult_s == NULL)
				{
					perror("malloc");
					free(seed_sieve);
					abort();
				}
				mult_s->next_byte = byte;
				mult_s->prime_adj = prime_adj;
				mult_s->stored_e  = e;
				mult_s->next = multiples;
				multiples = mult_s;
			}
		}
	}

	/* Clean up and return the list of multiples */
	free(seed_sieve);
	return multiples;
}
