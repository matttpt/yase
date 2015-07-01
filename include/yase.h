/*
 * yase - Yet Another Sieve of Eratosthenes
 * yase.h: header for all yase source files
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

#ifndef YASE_H
#define YASE_H

#include <stdlib.h>
#include <stdio.h>

/* Include the compile parameters and version headers */
#include <params.h>
#include <version.h>

/**********************************************************************\
 * Wheel structures                                                   *
\**********************************************************************/

/* Number of primes skipped by the primary sieving wheel (mod 210) */
#define WHEEL_PRIMES_SKIPPED (4U)

/*
 * Threshold below which to use the small prime sieve.  (This has
 * pretty much been determined experimentally.)  The threshold is
 * expressed in terms of the adjusted prime value (prime / 30).  Primes
 * with the adjusted value < SMALL_THRESHOLD will be sieved with the
 * small prime sieve.
 */
#define SMALL_THRESHOLD (SEGMENT_BYTES / 64)

/*
 * This is based HEAVILY off the way that the "primesieve" program
 * implements wheel factorization.  However, I have derived the
 * mathematics to generate the tables myself.
 */
struct wheel_elem
{
	unsigned char delta_f; /* Delta factor              */
	unsigned char delta_c; /* Delta correction          */
	unsigned char mask;    /* Bitmask to set bit        */
	  signed char next;    /* Offset to next wheel_elem */
};

/* Exposed wheel tables.  Even though "wheel30", "wheel210" and
   "wheel210_last_idx" are not const (so that wheel_init(void) can
   generate them), obviously don't modify them. */
extern struct wheel_elem   wheel30[64];
extern const unsigned char wheel30_offs[8];
extern const unsigned char wheel30_deltas[8];
extern const unsigned char wheel30_last_idx[30];
extern struct wheel_elem   wheel210[384];
extern const unsigned char wheel210_offs[48];
extern const unsigned char wheel210_deltas[48];
extern unsigned char       wheel210_last_idx[210];

/* Wheel initialization routine */
void wheel_init(void);

/**********************************************************************\
 * Population count                                                   *
\**********************************************************************/

/*
 * Take heed, this population count table is actually the number of
 * *unset* bits, because primes are unset.  You've been warned.
 *
 * Just like the "wheel" table above, "popcnt" is not const but
 * obviously don't modify it!
 */
extern unsigned char popcnt[256];

/* Initializes the population count table */
void popcnt_init(void);

/**********************************************************************\
 * Sieves                                                             *
\**********************************************************************/

/* We need to declare this for sieve_seed() and sieve_segment() */
struct prime_set;

/* Structure to hold information about a sieving prime, and which
   multiple needs to be marked next */
struct prime
{
	unsigned long next_byte; /* Next byte to mark                */
	unsigned long prime_adj; /* Prime divided by 30              */
	unsigned int  wheel_idx; /* Current index in the wheel table */
};

/* Finds the sieving primes */
void sieve_seed(
		unsigned long end_byte,
		unsigned long end_bit,
		unsigned long * count,
		struct prime_set * set);

/* Sieves a segment */
void sieve_segment(
		unsigned long start,
		unsigned long end,
		unsigned long end_bit,
		struct prime_set * set,
		unsigned long * count);

/**********************************************************************\
 * Pre-sieve mechanism                                                *
\**********************************************************************/

void presieve_init(void);
void presieve_cleanup(void);
void presieve_copy(
		unsigned char * sieve,
		unsigned long start,
		unsigned long end);

/**********************************************************************\
 * Storage of sieving primes                                          *
\**********************************************************************/

/* Number of primes to fit in a bucket */
#define BUCKET_PRIMES (1024)

/* Bucket structure - contains a bunch of sieving primes at once */
struct bucket
{
	unsigned long count;  /* Number of primes stored in the bucket */
	struct bucket * next; /* Next bucket in the list               */
	struct prime primes[BUCKET_PRIMES]; /* Prime storage           */
};

/* Set structure - contains sieving primes stored to sieve a particular
   interval */
struct prime_set
{
	unsigned long start;          /* Start byte of the interval      */
	unsigned long end;            /* End byte of the interval        */
	unsigned long end_segment;    /* Number of segs. in the interval */
	unsigned long current;        /* Current segment being sieved    */
	struct bucket * small;        /* List of small sieving primes    */
	struct bucket * inactive;     /* List of inactive sieving primes */
	struct bucket * inactive_end; /* Last node, for fast insertion   */
	struct bucket * finished;     /* List of finished sieving primes */
	struct bucket * pool;         /* Pool of unused buckets          */
	struct bucket ** lists;       /* List for each seg. in interval  */
};

/* Initializes a set of primes */
void prime_set_init(
		struct prime_set * set,
		unsigned long start,
		unsigned long end);

/* Adds a sieving prime to a prime set */
void prime_set_add(struct prime_set * set,
		unsigned long prime_adj,
		unsigned long next_byte,
		unsigned int wheel_idx);

/* Sets up the set/lists to sieve the next segment */
void prime_set_advance(struct prime_set * set);

/* Frees any memory allocated for the prime set, including the primes it
   contains */
void prime_set_cleanup(struct prime_set * set);


/**********************************************************************\
 * Inline routines                                                    *
\**********************************************************************/

/* Marks a multiple of a prime and updates wheel values - mod 30
   version */
static inline void mark_multiple_30(
		unsigned char * sieve,
		unsigned long prime_adj,
		unsigned long * byte,
		unsigned int  * wheel_idx)
{
	sieve[*byte] |= wheel30[*wheel_idx].mask;
	*byte += wheel30[*wheel_idx].delta_f * prime_adj;
	*byte += wheel30[*wheel_idx].delta_c;
	*wheel_idx += wheel30[*wheel_idx].next;
}

/* Marks a multiple of a prime and updates wheel values - mod 210
   version */
static inline void mark_multiple_210(
		unsigned char * sieve,
		unsigned long prime_adj,
		unsigned long * byte,
		unsigned int  * wheel_idx)
{
	sieve[*byte] |= wheel210[*wheel_idx].mask;
	*byte += wheel210[*wheel_idx].delta_f * prime_adj;
	*byte += wheel210[*wheel_idx].delta_c;
	*wheel_idx += wheel210[*wheel_idx].next;
}

/* Adds a prime to a bucket.  Returns false/zero if there's no space,
   true/nonzero otherwise. */
static inline int bucket_append(
		struct bucket * node,
		unsigned long prime_adj,
		unsigned long next_byte,
		unsigned int wheel_idx)
{
	unsigned long count = node->count;
	if(count == BUCKET_PRIMES)
	{
		return 0;
	}
	node->primes[count].prime_adj = prime_adj;
	node->primes[count].next_byte = next_byte;
	node->primes[count].wheel_idx = wheel_idx;
	node->count++;
	return 1;
}

/* Allocates a bucket for a set, drawing on the existing pool if
   possible */
static inline struct bucket * prime_set_bucket_init(
		struct prime_set * set,
		struct bucket * next)
{
	struct bucket * node;
	if(set->pool == NULL)
	{
		/* None left in pool.  Allocate one from scratch. */
		node = malloc(sizeof(struct bucket));
		if(node == NULL)
		{
			perror("malloc");
			abort();
		}
	}
	else
	{
		/* Use one from the set's pool */
		node = set->pool;
		set->pool = node->next;
	}
	node->count = 0UL;
	node->next = next;
	return node;
}

/* Inserts a prime into a list */
static inline void prime_set_list_append(
		struct prime_set * set,
		struct bucket ** list,
		unsigned long prime_adj,
		unsigned long next_byte,
		unsigned int wheel_idx)
{
	/* If the list is empty of the first bucket is full, allocate a new
	   bucket.  Otherwise, just add to the first bucket. */
	if(*list == NULL ||
	   !bucket_append(*list, prime_adj, next_byte, wheel_idx))
	{
		struct bucket * node = prime_set_bucket_init(set, *list);
		bucket_append(node, prime_adj, next_byte, wheel_idx);
		*list = node;
	}
}

/* Returns a bucket to a pool for later use */
static inline void prime_set_bucket_return(
		struct prime_set * set,
		struct bucket * bucket)
{
	bucket->next = set->pool;
	set->pool = bucket;
}

/* Returns the small primes stored with a set */
static inline struct bucket * prime_set_small(struct prime_set * set)
{
	return set->small;
}

/* Returns the list of primes needed for the current segment */
static inline struct bucket * prime_set_current(struct prime_set * set)
{
	return set->lists[set->current];
}

/* Saves a processed prime into its next list.  This is only used for
   large sieving primes.  Small sieving primes always remain in the
   small list. */
static inline void prime_set_save(
		struct prime_set * set,
		unsigned long prime_adj,
		unsigned long byte,
		unsigned int wheel_idx)
{
	unsigned long next_seg;

	/* Figure out the next segment in which this prime will be marked */
	next_seg = set->current + byte / SEGMENT_BYTES;

	/*
	 * Distribute as appropriate.  If the next segment is out of range
	 * or byte < SEGMENT_BYTES (indicating that this is the last
	 * segment), put it away.  Otherwise, put it into the next list it
	 * needs to be in.
	 */
	if(next_seg >= set->end_segment || byte < SEGMENT_BYTES)
	{
		/* The byte is stored as absolute in the finished list */
		byte += set->start + set->current * SEGMENT_BYTES;

		/* Prime goes into the finished list */
		prime_set_list_append(set, &set->finished, prime_adj, byte,
		                      wheel_idx);
	}
	else
	{
		/* Prime goes into a "real" list */
		struct bucket ** list = &set->lists[next_seg];
		byte %= SEGMENT_BYTES;
		prime_set_list_append(set, list, prime_adj, byte, wheel_idx);
	}
}

#endif /* !YASE_H */
