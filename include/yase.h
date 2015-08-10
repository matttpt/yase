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
#include <stdint.h>
#include <inttypes.h>

/* Include the compile parameters and version headers */
#include <params.h>
#include <version.h>

/* We save the program name to this global variable, so that it can be
   accessed for error messages.  Kind of messy, but there's no other
   good, portable way.  The pointer should be considered immutable. */
extern const char * yase_program_name;

/* Better perror() macro */
#define YASE_PERROR(str) \
	do { fprintf(stderr, "%s: ", yase_program_name); \
	     perror(str); } while(0)

/**********************************************************************\
 * Wheel structures                                                   *
\**********************************************************************/

/* Number of primes skipped by the primary sieving wheel (mod 210) */
#define WHEEL_PRIMES_SKIPPED (4U)

/*
 * Threshold below which to use the small prime sieve.  Primes smaller
 * than SMALL_THRESHOLD will be sieved with the small prime sieve, which
 * is much faster for primes with many multiples per segment.  This
 * is configured by SMALL_THRESHOLD_FACTOR in config.mk.
 */
#define SMALL_THRESHOLD \
	((uint64_t) (SEGMENT_BYTES * SMALL_THRESHOLD_FACTOR))

/*
 * This is based HEAVILY off the way that the "primesieve" program
 * implements wheel factorization.  However, I have derived the
 * mathematics to generate the tables myself.
 */
struct wheel_elem
{
	uint8_t delta_f; /* Delta factor              */
	uint8_t delta_c; /* Delta correction          */
	uint8_t mask;    /* Bitmask to set bit        */
	int8_t  next;    /* Offset to next wheel_elem */
};

/* Exposed wheel tables.  Even though "wheel30", "wheel210",
   "wheel210_last_idx" and "wheel210_find_idx" are not const (so that
   wheel_init(void) can generate them), obviously don't modify them. */
extern struct wheel_elem wheel30[64];
extern const uint8_t     wheel30_offs[8];
extern const uint8_t     wheel30_deltas[8];
extern const uint8_t     wheel30_last_idx[30];
extern const uint8_t     wheel30_find_idx[30];
extern struct wheel_elem wheel210[384];
extern const uint8_t     wheel210_offs[48];
extern const uint8_t     wheel210_deltas[48];
extern uint8_t           wheel210_last_idx[210];
extern uint8_t           wheel210_find_idx[210];

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
extern uint8_t popcnt[256];

/* Initializes the population count table */
void popcnt_init(void);

/**********************************************************************\
 * Sieves                                                             *
\**********************************************************************/

/* We need to declare this for sieve_seed() and sieve_segment() */
struct prime_set;

/*
 * This structure describes a sieving interval.  start_byte and end_byte
 * are the first byte checked and first byte not checked, respectively.
 * start_bit is the first bit of start_byte checked.  end_bit is the
 * first byte of (end_byte - 1) not checked, or 0 if the entire last
 * byte must be checked.
 */
struct interval
{
	uint64_t start_byte;
	uint64_t end_byte;
	unsigned int start_bit;
	unsigned int end_bit;
};

/* These calculate the bit/byte intervals needed for sieving */
void calculate_seed_interval(
		uint64_t max,
		uint64_t * seed_end_byte,
		unsigned int * seed_end_bit);
void calculate_interval(
		uint64_t start,
		uint64_t max,
		struct interval * inter);

/* Structure to hold information about a sieving prime, and which
   multiple needs to be marked next */
struct prime
{
	uint64_t next_byte; /* Next byte to mark                */
	uint32_t prime_adj; /* Prime divided by 30              */
	uint32_t wheel_idx; /* Current index in the wheel table */
};

/* Finds the sieving primes */
void sieve_seed(
		uint64_t end_byte,
		unsigned int end_bit,
		struct prime_set * set);

/* Sieves a segment */
void sieve_segment(
		uint64_t start,
		unsigned int start_bit,
		uint64_t end,
		unsigned int end_bit,
		struct prime_set * set,
		uint64_t * count);

/* Sieves an interval */
void sieve_interval(
		const struct interval * inter,
		struct prime_set * set,
		uint64_t * count);

/**********************************************************************\
 * Pre-sieve mechanism                                                *
\**********************************************************************/

void presieve_init(void);
void presieve_cleanup(void);
void presieve_copy(
		uint8_t * sieve,
		uint64_t start,
		uint64_t end);

/**********************************************************************\
 * Argument processing                                                *
\**********************************************************************/

/* Enumeration indicating what to do, based on arguments */
enum args_action
{
	ACTION_FAIL,
	ACTION_HELP,
	ACTION_VERSION,
	ACTION_SIEVE
};

/* Processes arguments, writing back the maximum value given on the
   command line if the action to take is ACTION_SIEVE */
enum args_action process_args(
		int argc,
		char * argv[],
		uint64_t * min,
		uint64_t * max);

/**********************************************************************\
 * Evaluation of mathematical expressions, e.g. for command line      *
\**********************************************************************/

int evaluate(const char * expr, uint64_t * result);

/**********************************************************************\
 * Storage of sieving primes                                          *
\**********************************************************************/

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
	uint64_t start;               /* Start byte of the interval      */
	uint64_t end;                 /* End byte of the interval        */
	uint64_t end_segment;         /* Number of segs. in the interval */
	uint64_t current;             /* Current segment being sieved    */
	unsigned long lists_alloc;    /* Number of list ptrs allocated   */
	struct bucket * small;        /* List of small sieving primes    */
	struct bucket * inactive;     /* List of inactive sieving primes */
	struct bucket * inactive_end; /* Last node, for fast insertion   */
	struct bucket * unused;       /* List of unused sieving primes   */
	struct bucket * pool;         /* Pool of unused buckets          */
	struct bucket ** lists;       /* List for each seg. in interval  */
};

/* Initializes a set of primes */
void prime_set_init(
		struct prime_set * set,
		const struct interval * inter);

/* Adds a sieving prime to a prime set */
void prime_set_add(struct prime_set * set,
		uint64_t prime,
		uint64_t next_byte,
		uint32_t wheel_idx);

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
		uint8_t *  sieve,
		uint32_t   prime_adj,
		uint32_t * byte,
		uint32_t * wheel_idx)
{
	sieve[*byte] |= wheel30[*wheel_idx].mask;
	*byte += wheel30[*wheel_idx].delta_f * prime_adj;
	*byte += wheel30[*wheel_idx].delta_c;
	*wheel_idx += wheel30[*wheel_idx].next;
}

/* Marks a multiple of a prime and updates wheel values - mod 210
   version */
static inline void mark_multiple_210(
		uint8_t *  sieve,
		uint32_t   prime_adj,
		uint32_t * byte,
		uint32_t * wheel_idx)
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
		uint32_t prime_adj,
		uint64_t next_byte,
		uint32_t wheel_idx)
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
			YASE_PERROR("malloc");
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
		uint32_t prime_adj,
		uint64_t next_byte,
		uint32_t wheel_idx)
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

/* Saves a processed prime into its next list.  This is only used for
   large sieving primes.  Small sieving primes always remain in the
   small list. */
static inline void prime_set_save(
		struct prime_set * set,
		uint32_t prime_adj,
		uint64_t byte,
		uint32_t wheel_idx)
{
	struct bucket ** list;
	uint64_t next_seg;

	/* Figure out the next segment in which this prime will be marked,
	   and place it in the appropriate list */
	next_seg = byte / SEGMENT_BYTES;
	list = &set->lists[next_seg];
	byte %= SEGMENT_BYTES;
	prime_set_list_append(set, list, prime_adj, byte, wheel_idx);
}

#endif /* !YASE_H */
