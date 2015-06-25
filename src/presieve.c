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
static unsigned char * presieve;
static unsigned long   presieve_len;

/* List of first few primes and their wheel spokes */
static unsigned long presieve_primes[6] =
	{ 11, 13, 17, 19, 23, 29 };

/* Pre-sieve initialization */
void presieve_init(void)
{
	unsigned long len, prime, prime_adj, byte;
	unsigned int i, wheel_idx;

	/* Find the length of the buffer, and save to presieve_len */
	len = 210;
	for(i = 0; i < PRESIEVE_PRIMES; i++)
	{
		len *= presieve_primes[i];
	}
	len /= 30;
	presieve_len = len;

	/* Allocate buffer */
	presieve = malloc(len);
	if(presieve == NULL)
	{
		perror("malloc");
		abort();
	}

	/* Prepare to sieve */
	memset(presieve, 0, len);

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
		presieve[byte] |= wheel30[wheel_idx].mask;
		byte += wheel30[wheel_idx].delta_c;
		wheel_idx += wheel30[wheel_idx].next;
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
			presieve[byte] |= wheel210[wheel_idx].mask;
			byte += wheel210[wheel_idx].delta_f * prime_adj;
			byte += wheel210[wheel_idx].delta_c;
			wheel_idx += wheel210[wheel_idx].next;
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
		unsigned char * sieve,
		unsigned long start,
		unsigned long end)
{
	unsigned long ps_idx, sv_idx, sv_len;

	/* Find the start point in the pre-sieve buffer */
	ps_idx = start % presieve_len;

	/* Copy the presieve data in */
	sv_idx = 0;
	sv_len = end - start;
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
