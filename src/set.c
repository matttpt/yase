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
#include <math.h>
#include <yase.h>

/*
 * A prime set uses linked lists of buckets, which can contain up to a
 * fixed number (BUCKET_PRIMES) of sieving primes.  There is a list for
 * every segment to be sieved, an array of lists of small sieving primes
 * (categorized by next wheel_idx) to be processed specially, a list of
 * inactive primes whose first segment to mark has not yet been reached,
 * and a list of finished primes which no longer have multiples on the
 * interval.  By placing primes into lists associated with the segment
 * of their next multiple, the overhead of having many sieivng primes
 * with no multiples on a segment is greatly reduced.  This idea comes
 * from Tomás Oliveira e Silva.  The algorithm is described at
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

/* Determines how many list head pointers to allocate, based on the
   maximum number of active lists that should be needed at any given
   point */
static unsigned long find_lists_needed(uint64_t end)
{
	double max_multiple_delta;
	unsigned long max_segment_delta, lists_needed;

	/* We first find a cap on the largest value a sieving prime could
	   take for the interval being sieved, and multiply that by 10
	   (as the greatest gap between multiples needing to be marked on a
	   mod 210 wheel is 10 * prime) */
	max_multiple_delta = sqrt((double) (end * 30)) * 10;

	/* Now we determine how many segments that delta is, and add one to
	   ensure that we round up */
	max_segment_delta = (max_multiple_delta / (SEGMENT_BYTES * 30)) + 1;

	/* The above value should reflect how many segments forward we will
	   ever have to keep track of.  In addition, we will have to keep
	   track of the current segment, so the number of lists needed is
	   one greater. */
	lists_needed = max_segment_delta + 1;
	return lists_needed;
}

/* Adjusts a prime's information so that the next multiple to be sieved
   is above a given byte */
static void adjust_up(uint64_t prime, uint64_t start,
                      uint64_t * next_byte, uint32_t * wheel_idx)
{
	uint64_t divisor;
	unsigned int div_mod, new_wheel_idx;

	/* Find starting divisor */
	divisor = (start * 30) / prime;
	if((start * 30) % prime != 0)
	{
		divisor++;
	}

	/* Find the next value for divisor that is on the wheel being used
	   to sieve the prime's multiples */
	if(prime < SMALL_THRESHOLD)
	{
		div_mod = divisor % 30;
		new_wheel_idx = wheel30_find_idx[div_mod];
		divisor -= div_mod;
		divisor += wheel30_offs[new_wheel_idx];
		*wheel_idx = wheel30_last_idx[prime % 30] * 8 + new_wheel_idx;
	}
	else
	{
		div_mod = divisor % 210;
		new_wheel_idx = wheel210_find_idx[div_mod];
		divisor -= div_mod;
		divisor += wheel210_offs[new_wheel_idx];
		*wheel_idx = wheel30_last_idx[prime % 30] * 48 + new_wheel_idx;
	}

	/* Calculate next byte */
	*next_byte = (prime * divisor) / 30;
}

/* Allocates and initializes an empty set of sieving primes, for use
   with the interval [start, end) */
void prime_set_init(
		struct prime_set * set,
		const struct interval * inter)
{
	uint64_t n_segs;
	unsigned long lists_alloc;

	/* Determine how many segments there are */
	n_segs = (inter->end_byte - inter->start_byte + SEGMENT_BYTES - 1) /
	         SEGMENT_BYTES;

	/* Allocate the list head pointers - as many as will be needed at
	   one time */
	lists_alloc = find_lists_needed(inter->end_byte);
	set->lists = calloc(lists_alloc, sizeof(struct bucket *));
	if(set->lists == NULL)
	{
		YASE_PERROR("calloc");
		abort();
	}
	set->lists_alloc = lists_alloc;

	/* Set up set metadata */
	set->start       = inter->start_byte;
	set->end         = inter->end_byte;
	set->end_segment = n_segs;
	set->current     = 0;

	/*
	 * Set the "special" lists to NULL to start.  We don't have to worry
	 * avout the regular lists because calloc() zeroes the memory before
	 * returning the pointer to it.
	 */
	memset(set->small, 0, sizeof(set->small));
	set->inactive     = NULL;
	set->inactive_end = NULL;
	set->unused       = NULL;

	/* Start with no buckets allocated */
	set->pool = NULL;
}

/* Adds a prime to a set.  This is designed to be used ONLY from
   sieve_seed(), and there are some important pre-conditions for its use.
   See the notes above for information. */
void prime_set_add(struct prime_set * set,
		uint64_t prime,
		uint64_t next_byte,
		uint32_t wheel_idx)
{
	uint32_t prime_adj = (uint32_t) (prime / 30);

	/* Put the prime into the interval if necessary */
	if(next_byte < set->start)
	{
		adjust_up(prime, set->start, &next_byte, &wheel_idx);
	}

	/* Add prime to appropriate list */
	if(prime < SMALL_THRESHOLD)
	{
		next_byte -= set->start;
		prime_set_list_append(set, &set->small[wheel_idx], prime_adj,
		                      next_byte, wheel_idx);
	}
	else if(next_byte >= set->end)
	{
		/* We don't need to worry about the prime on this
		   interval */
		prime_set_list_append(set, &set->unused, prime_adj,
		                      next_byte, wheel_idx);
	}
	else
	{
		/* Adjust the next byte */
		next_byte -= set->start;

		/* If the prime's next segment is the active range, place it in
		   an active list.  Otherwise, place it in the inactive list. */
		if(next_byte / SEGMENT_BYTES < set->lists_alloc)
		{
			prime_set_list_append(set,
			                      &set->lists[next_byte / SEGMENT_BYTES],
			                      prime_adj,
			                      next_byte % SEGMENT_BYTES,
			                      wheel_idx);
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
	/* Shift list pointers, update current segment */
	memmove(set->lists, set->lists + 1,
	        (set->lists_alloc - 1) * sizeof(struct bucket *));
	set->lists[set->lists_alloc - 1] = NULL;
	set->current++;

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
		struct bucket ** list;

		/* Setup prime and end */
		prime = set->inactive->primes;
		end   = &set->inactive->primes[set->inactive->count];

		/* Unload the portion of the bucket that needs to be
		   activated */
		while(prime < end &&
		      prime->next_byte / SEGMENT_BYTES <= set->current)
		{
			uint64_t next_seg;

			/* Calculate next segment index */
			next_seg = prime->next_byte / SEGMENT_BYTES;
			next_seg -= set->current;

			/* Add prime to list */
			list = &set->lists[next_seg];
			prime_set_list_append(set, list, prime->prime_adj,
			                      prime->next_byte % SEGMENT_BYTES,
			                      prime->wheel_idx);
			prime++;
		}

		/* Shift bucket contents or, if the bucket is empty, return it
		   to the pool */
		if(prime < end)
		{
			/* Shift contents if primes remain in the bucket */
			memmove(set->inactive->primes, prime,
			        (end - prime) * sizeof(struct prime));
			set->inactive->count = end - prime;
		}
		else
		{
			/* Advance to next bucket, returning the emptied bucket to
			   the pool */
			struct bucket * to_return = set->inactive;
			set->inactive = set->inactive->next;
			prime_set_bucket_return(set, to_return);
		}
	}
}

/* Frees all of the primes stored in a set, as well as the list head
   pointer table itself */
void prime_set_cleanup(struct prime_set * set)
{
	unsigned long i;
	struct bucket * bucket, * to_free;

	/* Clean up small primes list */
	for(i = 0; i < 64; i++)
	{
		bucket = set->small[i];
		while(bucket != NULL)
		{
			to_free = bucket;
			bucket = bucket->next;
			free(to_free);
		}
	}

	/* Clean up the inactive list */
	bucket = set->inactive;
	while(bucket != NULL)
	{
		to_free = bucket;
		bucket = bucket->next;
		free(to_free);
	}

	/* Clean up the unused list */
	bucket = set->unused;
	while(bucket != NULL)
	{
		to_free = bucket;
		bucket = bucket->next;
		free(to_free);
	}

	/* Clean up each regular list */
	for(i = 0; i < set->lists_alloc; i++)
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
