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
static unsigned long presieve_primes[7] =
	{ 7, 11, 13, 17, 19, 23, 29 };

/* Pre-sieve initialization */
void presieve_init(void)
{
	unsigned long len;
	unsigned int i;

	/* Find the length of the buffer, and save to presieve_len */
	len = 30;
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

	/* Run pre-sieve */
	for(i = 0; i < PRESIEVE_PRIMES; i++)
	{
		unsigned long prime, prime_adj, byte;
		struct wheel_elem * e;

		/* Mark multiples for the current prime */
		prime     = presieve_primes[i];
		prime_adj = prime / 30;
		byte      = prime / 30;
		e         = &wheel[(i + 1) * 8];
		while(byte < len)
		{
			presieve[byte] |= e->mask;
			byte += e->delta_f * prime_adj + e->delta_c;
			e += e->next;
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
