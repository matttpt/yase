/*
 * yase - Yet Another Sieve of Eratosthenes
 * popcnt.c: "population count" (i.e. count of unset bits) mechanism
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

#if defined(__GNUC__) || defined(__clang__)

/**********************************************************************\
 * GCC/Clang: use __builtin_popcount and friends, which will          *
 * translate to specialized CPU instructions (e.g. x86 popcnt) when   *
 * available.                                                         *
\**********************************************************************/

/* We don't have a population count table to initialize, so this is a
   no-op */
void popcnt_init(void) { }

/* Performs a population count on the provided sieve segment. The
   start_bit and end_bit work the same as in sieve_segment; see the
   documentation there. */
uint64_t popcnt(
		const uint8_t * sieve,
		unsigned int start_bit,
		unsigned long end,
		unsigned int end_bit)
{
	unsigned long i;
	uint64_t count = 0;

	/* Go in 64-bit chunks as long as possible; then go byte-by-byte */
	for(i = 0; i < (end & ~0x7UL); i += 8)
	{
		uint64_t bits;
		memcpy(&bits, &sieve[i], sizeof(uint64_t));
		count += __builtin_popcountll(bits);
	}
	for(i = end & ~0x7UL; i < end; i++)
	{
		count += __builtin_popcount(sieve[i]);
	}

	/* Prune any extra bits we didn't need in the first or last byte */
	if(start_bit != 0)
	{
		uint8_t mask = (uint8_t) ~(0xFFU << start_bit);
		count -= __builtin_popcount(sieve[0] & mask);
	}
	if(end_bit != 0)
	{
		uint8_t mask = (uint8_t) (0xFFU << end_bit);
		count -= __builtin_popcount(sieve[end - 1] & mask);
	}

	return count;
}

#else

/**********************************************************************\
 * FALLBACK IMPLEMENTATION: use a lookup table                         *
\**********************************************************************/

/* Population count table */
static uint8_t popcnt_lookup[256];

/* Initializes the population count table */
void popcnt_init(void)
{
	unsigned int i;
	for(i = 0; i < 256; i++)
	{
		uint8_t set_count = 0;
		unsigned int mask;
		for(mask = 1; mask < 256; mask <<= 1)
		{
			if((i & (uint8_t) mask) != 0)
			{
				set_count++;
			}
		}
		popcnt_lookup[i] = set_count;
	}
}

/* Performs a population count on the provided sieve segment. The
   start_bit and end_bit work the same as in sieve_segment; see the
   documentation there. */
uint64_t popcnt(
		const uint8_t * sieve,
		unsigned int start_bit,
		unsigned long end,
		unsigned int end_bit)
{
	unsigned long i;
	uint64_t count = 0;

	for(i = 0; i < end; i++)
	{
		count += popcnt_lookup[sieve[i]];
	}

	/* Prune any extra bits we didn't need in the first or last byte */
	if(start_bit != 0)
	{
		uint8_t mask = (uint8_t) ~(0xFFU << start_bit);
		count -= popcnt_lookup[sieve[0] & mask];
	}
	if(end_bit != 0)
	{
		uint8_t mask = (uint8_t) (0xFFU << end_bit);
		count -= popcnt_lookup[sieve[end - 1] & mask];
	}

	return count;
}

#endif
