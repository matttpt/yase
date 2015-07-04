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

/*
 * A prime set uses linked lists of buckets, which can contain up to a
 * fixed number (BUCKET_PRIMES) of sieving primes.  There is a list for
 * every segment to be sieved, a small primes list containing the small
 * sieving primes to be processed specially, a list of inactive primes
 * whose first segment to mark has not yet been reached, and a list of
 * finished primes which no longer have multiples on the interval.  By
 * placing primes into lists associated with the segment of their next
 * multiple, the overhead of having many sieivng primes with no
 * multiples on a segment is greatly reduced.  This idea comes from
 * Tomás Oliveira e Silva.  The algorithm is described at
 * http://sweet.ua.pt/tos/software/prime_sieve.html.
 *
 * There are also a bunch of inline prime set/bucket routines in yase.h,
 * so make sure to check those out too.
 *
 * The logic in prime_set_add() assumes that primes are added IN ORDER,
 * so be careful!  (This assumption makes it a lot easier to create the
 * list of inactive primes, which must be sorted so that the first prime
 * to activate is first, etc.)  Currently, this is not a problem, as
 * sieve_seed() discovers and submits the sieving primes in order.
 *
 * It is also important to note that when a prime is submitted to the
 * set, it is assumed that the next_byte of the prime is in absolute
 * terms, i.e. if one massive, unsegmented sieving bit array were used.
 */

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
	set->lists = calloc(n_lists, sizeof(struct bucket *));
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
	 * Set the "special" lists to NULL to start.  We don't have to worry
	 * avout the regular lists because calloc() zeroes the memory before
	 * returning the pointer to it.
	 */
	set->small        = NULL;
	set->inactive     = NULL;
	set->inactive_end = NULL;
	set->finished     = NULL;

	/* Start with no buckets allocated */
	set->pool = NULL;
}

/* Adds a prime to a set.  This is designed to be used ONLY from
   sieve_seed(), and there are some important pre-conditions for its use.
   See the notes above for information. */
void prime_set_add(struct prime_set * set,
		unsigned long prime,
		unsigned long next_byte,
		unsigned int wheel_idx)
{
	unsigned long prime_adj = prime / 30;

	/* Put the prime into the interval if necessary */
	if(next_byte < set->start)
	{
		/* TODO - recalculate the prime's multiple to fit in the
		   segment.  This currently cannot happen. */
		abort();
	}

	/* Add prime to appropriate list */
	if(prime < SMALL_THRESHOLD)
	{
		next_byte -= set->start;
		prime_set_list_append(set, &set->small, prime_adj, next_byte,
		                      wheel_idx);
	}
	else if(next_byte >= set->end)
	{
		/* We don't need to worry about the prime on this
		   interval */
		prime_set_list_append(set, &set->finished, prime_adj,
		                      next_byte, wheel_idx);
	}
	else
	{
		/* Adjust the next byte */
		next_byte -= set->start;

		/* If the prime's next segment is the first, place it in that
		   list.  Otherwise, place it in the inactive list. */
		if(next_byte / SEGMENT_BYTES == 0)
		{
			prime_set_list_append(set, &set->lists[0], prime_adj,
			                      next_byte, wheel_idx);
		}
		else if(set->inactive_end == NULL ||
		        !bucket_append(set->inactive_end, prime_adj, next_byte,
		                       wheel_idx))
		{
			struct bucket * node = prime_set_bucket_init(set, NULL);
			bucket_append(node, prime_adj, next_byte, wheel_idx);
			if(set->inactive_end != NULL)
			{
				set->inactive_end->next = node;
				set->inactive_end = node;
			}
			else
			{
				set->inactive     = node;
				set->inactive_end = node;
			}
		}
	}
}

/* Advances to the list for the next segment */
void prime_set_advance(struct prime_set * set)
{
	/* Clear out old pointer, advance current segment */
	set->lists[set->current++] = NULL;

	/*
	 * Activate appropriate inactive primes.  We don't check if the
	 * count of primes in the first bucket is 0, because this should
	 * never happen.  Because the inactive primes are in activation
	 * order, we just keep unloading buckets until the current bucket
	 * contains no primes that must be activated.
	 */
	while(set->inactive != NULL &&
	      set->inactive->primes[0].next_byte / SEGMENT_BYTES
	          <= set->current)
	{
		struct prime * prime, * end;
		struct bucket * to_return;
		struct bucket ** list;

		/* Setup prime and end */
		prime = set->inactive->primes;
		end   = &set->inactive->primes[set->inactive->count];

		/* Unload the whole bucket */
		while(prime < end)
		{
			list = &set->lists[prime->next_byte / SEGMENT_BYTES];
			prime_set_list_append(set, list, prime->prime_adj,
			                      prime->next_byte % SEGMENT_BYTES,
			                      prime->wheel_idx);
			prime++;
		}

		/* Advance to next bucket, returning the emptied bucket to the
		   pool */
		to_return = set->inactive;
		set->inactive = set->inactive->next;
		prime_set_bucket_return(set, to_return);
	}
}

/* Frees all of the primes stored in a set, as well as the list head
   pointer table itself */
void prime_set_cleanup(struct prime_set * set)
{
	unsigned long i;
	struct bucket * bucket, * to_free;

	/* Clean up small primes list */
	bucket = set->small;
	while(bucket != NULL)
	{
		to_free = bucket;
		bucket = bucket->next;
		free(to_free);
	}

	/* Clean up the inactive list */
	bucket = set->inactive;
	while(bucket != NULL)
	{
		to_free = bucket;
		bucket = bucket->next;
		free(to_free);
	}

	/* Clean up the finished list */
	bucket = set->finished;
	while(bucket != NULL)
	{
		to_free = bucket;
		bucket = bucket->next;
		free(to_free);
	}

	/* Clean up each regular list */
	for(i = 0; i < set->end_segment; i++)
	{
		bucket = set->lists[i];
		while(bucket != NULL)
		{
			to_free = bucket;
			bucket = bucket->next;
			free(to_free);
		}
	}

	/* Clean up any pooled empty buckets */
	bucket = set->pool;
	while(bucket != NULL)
	{
		to_free = bucket;
		bucket = bucket->next;
		free(to_free);
	}

	/* Free the array of list head pointers */
	free(set->lists);
}