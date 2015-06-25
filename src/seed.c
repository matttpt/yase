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
struct prime * sieve_seed(
		unsigned long max,
		unsigned long * count,
		unsigned long * next_byte)
{
	unsigned long i, max_seed, final_byte, final_bit;
	struct prime * primes = NULL;
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
	final_bit = final_byte * 8 + wheel30_last_idx[max_seed % 30];

	/* We don't bother to segment for this process.  We allocate the
	   sieve segment manually. */
	seed_sieve = malloc(final_byte + 1);
	if(seed_sieve == NULL)
	{
		perror("malloc");
		abort();
	}

	/* Copy in pre-sieve data */
	presieve_copy(seed_sieve, 0, final_byte + 1);

	/* Run the sieve */
	for(i = PRESIEVE_PRIMES + 2; i < (final_byte + 1) * 8; i++)
	{
		if((seed_sieve[i / 8] & ((unsigned char) 1U << (i % 8))) == 0)
		{
			unsigned long prime, mult, prime_adj, byte;
			unsigned int wheel_idx;
			struct prime * prime_s;

			/* Count the prime */
			(*count)++;

			/* Mark multiples */
			prime     = (i / 8) * 30 + wheel30_offs[i % 8];
			mult      = prime * prime;
			prime_adj = prime / 30;
			byte      = mult / 30;
			wheel_idx = (i % 8) * 48 + wheel210_last_idx[prime % 210];
			while(byte <= final_byte)
			{
				seed_sieve[byte] |= wheel210[wheel_idx].mask;
				byte += wheel210[wheel_idx].delta_f * prime_adj;
			  	byte += wheel210[wheel_idx].delta_c;
				wheel_idx += wheel210[wheel_idx].next;
			}

			/* If this prime is in the range that we need sieving primes,
			   record it. */
			if(i <= final_bit)
			{
				/* Allocate new sieving prime structure */
				prime_s = malloc(sizeof(struct prime));
				if(prime_s == NULL)
				{
					perror("malloc");
					free(seed_sieve);
					abort();
				}
				prime_s->next_byte = byte;
				prime_s->prime_adj = prime_adj;
				prime_s->wheel_idx = wheel_idx;
				prime_s->next = primes;
				primes = prime_s;
			}
		}
	}

	/* Clean up and return the list of multiples */
	free(seed_sieve);
	return primes;
}
