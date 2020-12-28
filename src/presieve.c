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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <yase.h>

/* Buffer for pre-sieved bits */
static uint8_t *     presieve;
static unsigned long presieve_len;

/* List of first few primes and their wheel spokes */
static unsigned long presieve_primes[6] =
	{ 11, 13, 17, 19, 23, 29 };

/* Pre-sieve initialization */
void presieve_init(void)
{
	unsigned long len, i;
	uint64_t prime;
	uint32_t byte, prime_adj, wheel_idx;

	/* Find the length of the buffer, and save to presieve_len */
	len = 210 / 30;
	for(i = 0; i < PRESIEVE_PRIMES; i++)
	{
		len *= presieve_primes[i];
	}
	presieve_len = len;

	/* Allocate buffer */
	presieve = malloc(len);
	if(presieve == NULL)
	{
		YASE_PERROR("malloc");
		abort();
	}

	/* Prepare to sieve */
	memset(presieve, 0xFF, len);

	/*
	 * Before we do the "requested" pre-sieve, we also pre-sieve 7.
	 * While the mod 210 wheel factorization used in the sieve
	 * eliminates the marking of multiples of 7 (increasing speed), the
	 * bitset is still mod 30 and does not skip over multiples of 7.
	 * Thus, we still need to pre-sieve 7.
	 */
	byte      = 0;
	wheel_idx = 8;
	while(byte < len)
	{
		mark_multiple_30(presieve, 0, &byte, &wheel_idx);
	}

	/* Run pre-sieve */
	for(i = 0; i < PRESIEVE_PRIMES; i++)
	{
		/* Mark multiples for the current prime */
		prime     = presieve_primes[i];
		prime_adj = prime / 30;
		byte      = prime / 30;
		wheel_idx = (i + 2) * 48;
		while(byte < len)
		{
			mark_multiple_210(presieve, prime_adj, &byte, &wheel_idx);
		}
	}
}

/* Pre-sieve cleanup */
void presieve_cleanup(void)
{
	free(presieve);
}

/* Copies pre-sieve data into a sieve buffer */
void presieve_copy(
		uint8_t * sieve,
		uint64_t start,
		uint64_t end)
{
	unsigned long ps_idx, sv_idx, sv_len;

	/* Find the start point in the pre-sieve buffer */
	ps_idx = (unsigned long) (start % presieve_len);

	/* Copy the presieve data in */
	sv_idx = 0;
	sv_len = (unsigned long) (end - start);
	while(sv_idx < sv_len)
	{
		unsigned long len;

		/* Find length to copy */
		len = presieve_len - ps_idx;
		if(len > sv_len - sv_idx)
		{
			len = sv_len - sv_idx;
		}

		/* Perform the copy and update working indices */
		memcpy(&sieve[sv_idx], &presieve[ps_idx], len);
		ps_idx = 0;
		sv_idx += len;
	}
}
