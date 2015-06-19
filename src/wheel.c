/*
 * yase - Yet Another Sieve of Eratosthenes
 * wheel.c: wheel factorization tables
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

/* mod 30 wheel data */
struct wheel_elem wheel[64];

/* Tables of offsets, deltas, and last wheel index before a number */
const unsigned char wheel_offs[8]   = { 1, 7, 11, 13, 17, 19, 23, 29 };
const unsigned char wheel_deltas[8] = { 6, 4,  2,  4,  2,  4,  6,  2 };
const unsigned char wheel_last_idx[30] =
	{ 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
	  1, 2, 2, 3, 3, 3, 3, 4, 4, 5,
	  5, 5, 5, 6, 6, 6, 6, 6, 6, 7 };

/* Routine to construct the wheel table */
void wheel_init(void)
{
	unsigned int i, j;

	/* Each cycle of 8 is for one initial prime offset */
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			unsigned char offs_p, offs_f, delta, bit_offs;

			/* Find the spoke offset of the prime, the spoke offset of the
			   other multiple factor, and the delta to the next
			   multiple factor */
			offs_p = wheel_offs[i];
			offs_f = wheel_offs[j];
			delta  = wheel_deltas[j];

			/* Record the delta factor */
			wheel[i * 8 + j].delta_f = delta;

			/* Record the delta correction */
			wheel[i * 8 + j].delta_c =
				  ((offs_p * (offs_f + delta)) / 30)
				- ((offs_p * offs_f)           / 30);

			/* Record the bitmask to set the appropriate bit */
			bit_offs = wheel_last_idx[(offs_p * offs_f) % 30];
			wheel[i * 8 + j].mask = (unsigned char) (1U << bit_offs);

			/* Record the delta to the next table element */
			if(j == 7)
			{
				wheel[i * 8 + j].next = -7;
			}
			else
			{
				wheel[i * 8 + j].next = 1;
			}
		}
	}
}
