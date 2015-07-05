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
struct wheel_elem wheel30[64];
const uint8_t wheel30_offs[8]   = { 1, 7, 11, 13, 17, 19, 23, 29 };
const uint8_t wheel30_deltas[8] = { 6, 4,  2,  4,  2,  4,  6,  2 };
const uint8_t wheel30_last_idx[30] =
	{ 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
	  1, 2, 2, 3, 3, 3, 3, 4, 4, 5,
	  5, 5, 5, 6, 6, 6, 6, 6, 6, 7 };

/* mod 210 wheel data */
struct wheel_elem wheel210[384];
const uint8_t wheel210_offs[48] =
	{   1,  11,  13,  17,  19,  23,  29,  31,
	   37,  41,  43,  47,  53,  59,  61,  67,
	   71,  73,  79,  83,  89,  97, 101, 103,
	  107, 109, 113, 121, 127, 131, 137, 139,
	  143, 149, 151, 157, 163, 167, 169, 173,
	  179, 181, 187, 191, 193, 197, 199, 209 };
const uint8_t wheel210_deltas[48] =
	{ 10,  2,  4,  2,  4,  6,  2,  6,  4,  2,  4,  6,
	   6,  2,  6,  4,  2,  6,  4,  6,  8,  4,  2,  4,
	   2,  4,  8,  6,  4,  6,  2,  4,  6,  2,  6,  6,
	   4,  2,  4,  6,  2,  6,  4,  2,  4,  2, 10,  2 };
uint8_t wheel210_last_idx[210];

/* Routine to construct the wheel tables - mod 30 */
static void wheel30_init(void)
{
	unsigned int i, j;

	/* Each cycle of 8 is for one initial prime offset in the (mod 30)
	   bit array */
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 8; j++)
		{
			uint8_t offs_p, offs_f, delta, bit_offs;

			/* Find the spoke offset of the prime, the spoke offset of the
			   other multiple factor, and the delta to the next
			   multiple factor */
			offs_p = wheel30_offs[i];
			offs_f = wheel30_offs[j];
			delta  = wheel30_deltas[j];

			/* Record the delta factor */
			wheel30[i * 8 + j].delta_f = delta;

			/* Record the delta correction */
			wheel30[i * 8 + j].delta_c =
				  ((offs_p * (offs_f + delta)) / 30)
				- ((offs_p * offs_f)           / 30);

			/* Record the bitmask to set the appropriate bit */
			bit_offs = wheel30_last_idx[(offs_p * offs_f) % 30];
			wheel30[i * 8 + j].mask = (uint8_t) (1U << bit_offs);

			/* Record the delta to the next table element */
			if(j == 7)
			{
				wheel30[i * 8 + j].next = -7;
			}
			else
			{
				wheel30[i * 8 + j].next = 1;
			}
		}
	}
}

/* Routine to construct the wheel tables - mod 210 */
static void wheel210_init(void)
{
	unsigned int i, j, last_idx;

	/* Setup wheel210_last_idx */
	j        = 0;
	last_idx = 0;
	for(i = 0; i < 210; i++)
	{
		if(wheel210_offs[j] == i)
		{
			last_idx = j;
			j++;
		}
		wheel210_last_idx[i] = (uint8_t) last_idx;
	}

	/* Setup the wheel table.  Each cycle of 48 is for one initial
	   prime offset in the (mod 30) bit array. */
	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 48; j++)
		{
			uint8_t offs_p, offs_f, delta, bit_offs;

			/* Find the spoke offset of the prime, the spoke offset of the
			   other multiple factor, and the delta to the next
			   multiple factor */
			offs_p = wheel30_offs[i];
			offs_f = wheel210_offs[j];
			delta  = wheel210_deltas[j];

			/* Record the delta factor */
			wheel210[i * 48 + j].delta_f = delta;

			/* Record the delta correction */
			wheel210[i * 48 + j].delta_c =
				  ((offs_p * (offs_f + delta)) / 30)
				- ((offs_p * offs_f)           / 30);

			/* Record the bitmask to set the appropriate bit */
			bit_offs = wheel30_last_idx[(offs_p * offs_f) % 30];
			wheel210[i * 48 + j].mask = (uint8_t) (1U << bit_offs);

			/* Record the delta to the next table element */
			if(j == 47)
			{
				wheel210[i * 48 + j].next = -47;
			}
			else
			{
				wheel210[i * 48 + j].next = 1;
			}
		}
	}
}

/* Routine to construct wheel tables - both */
void wheel_init(void)
{
	wheel30_init();
	wheel210_init();
}
