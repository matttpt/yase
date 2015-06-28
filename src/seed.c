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
void sieve_seed(
		unsigned long max,
		unsigned long * count,
		unsigned long * next_byte,
		struct prime ** small_primes_out,
		struct prime ** large_primes_out)
{
	unsigned long i, max_seed, final_byte, final_bit;
	struct prime * small_primes = NULL;
	struct prime * large_primes = NULL;
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

			/*
			 * If the prime is under the "small threshold," its
			 * multiples are marked with a mod 30 wheel and the prime is
			 * placed in the small_primes list.  Otherwise, it is sieved
			 * with a mod 210 wheel and placed in the large_primes list.
			 */
			if(prime_adj < SMALL_THRESHOLD)
			{
				wheel_idx = (i % 8) * 9;
				while(byte <= final_byte)
				{
					mark_multiple_30(seed_sieve, 0UL, prime_adj,
					                 &byte, &wheel_idx);
				}
			}
			else
			{
				wheel_idx = (i % 8) * 48 + wheel210_last_idx[prime % 210];
				while(byte <= final_byte)
				{
					mark_multiple_210(seed_sieve, 0UL, prime_adj,
					                  &byte, &wheel_idx);
				}
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
				if(prime_adj < SMALL_THRESHOLD)
				{
					prime_s->next = small_primes;
					small_primes = prime_s;
				}
				else
				{
					prime_s->next = large_primes;
					large_primes = prime_s;
				}
			}
		}
	}

	/* Clean up and write pointers to lists */
	free(seed_sieve);
	*small_primes_out = small_primes;
	*large_primes_out = large_primes;
}
