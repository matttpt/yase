/*
 * yase - Yet Another Sieve of Eratosthenes
 * set.c: code to store sieving primes
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

/* Allocates and initializes an empty set of sieving primes, for use
   with the interval [start, end) */
void prime_set_init(
		struct prime_set * set,
		unsigned long start,
		unsigned long end)
{
	unsigned long n_lists;

	/* Allocate the list head pointers */
	n_lists = (end - start + SEGMENT_BYTES - 1) / SEGMENT_BYTES;
	set->lists = calloc(n_lists, sizeof(struct prime *));
	if(set->lists == NULL)
	{
		perror("calloc");
		abort();
	}

	/* Set up set metadata */
	set->start       = start;
	set->end         = end;
	set->end_segment = n_lists;
	set->current     = 0UL;

	/*
	 * Set the small list and finished list to NULL to start.  We don't
	 * have to worry about the regular lists because calloc() zeroes the
	 * the memory before returning the pointer to it.
	 */
	set->small    = NULL;
	set->finished = NULL;
}

/* Adds a prime to a set */
void prime_set_add(struct prime_set * set, const struct prime * prime)
{
	struct prime * new;

	/* Put the prime into the interval if necessary */
	if(prime->next_byte < set->start)
	{
		/* TODO - recalculate the prime's multiple to fit in the
		   segment.  This currently cannot happen. */
		abort();
	}

	/* Allocate memory for the new sieving prime */
	new = malloc(sizeof(struct prime));
	if(new == NULL)
	{
		perror("malloc");
		abort();
	}
	*new = *prime;

	/* Add prime to appropriate list */
	if(prime->prime_adj < SMALL_THRESHOLD)
	{
		new->next_byte -= set->start;
		new->next = set->small;
		set->small = new;
	}
	else if(new->next_byte >= set->end)
	{
		/* We don't need to worry about the prime on this
		   interval */
		new->next = set->finished;
		set->finished = new;
	}
	else
	{
		unsigned long byte_adj, list_idx;

		/* Find the appropriate list */
		byte_adj = new->next_byte - set->start;
		list_idx = byte_adj / SEGMENT_BYTES;

		/* Adjust the next byte */
		new->next_byte = byte_adj % SEGMENT_BYTES;

		/* Place into the list */
		new->next = set->lists[list_idx];
		set->lists[list_idx] = new;
	}
}

/* Frees all of the primes stored in a set, as well as the list head
   pointer table itself */
void prime_set_cleanup(struct prime_set * set)
{
	unsigned long i;
	struct prime * prime, * to_free;

	/* Clean up small primes list */
	prime = set->small;
	while(prime != NULL)
	{
		to_free = prime;
		prime = prime->next;
		free(to_free);
	}
	
	/* Clean up each regular list */
	for(i = 0; i < set->end_segment; i++)
	{
		prime = set->lists[i];
		while(prime != NULL)
		{
			to_free = prime;
			prime = prime->next;
			free(to_free);
		}
	}

	/* Clean up the finished list */
	prime = set->finished;
	while(prime != NULL)
	{
		to_free = prime;
		prime = prime->next;
		free(to_free);
	}

	/* Free the array of list head pointers */
	free(set->lists);
}
