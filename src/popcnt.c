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

#include <yase.h>

/* Population count table */
uint8_t popcnt[256];

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
		popcnt[i] = set_count;
	}
}
