/*
 * yase - Yet Another Sieve of Eratosthenes
 * main.c: main routines
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
#include <time.h>
#include <math.h>
#include <yase.h>

/* Saved program name - globally accessed for error messages, etc. */
const char * yase_program_name;

/* Help format string */
static const char * help_format =
"Usage: %s [OPTION]... MAX\n"
"Count and display the number of primes on the interval [0,MAX].  MAX may\n"
"be an expression, e.g. 2^32-1.  Supported operations are addition (+),\n"
"subtraction (-), multiplication (*), and exponentiation  (** or ^).\n\n"
"Options:\n"
" --help      display this help meessage\n"
" --version   display version information\n";

/*
 * Main routine!
 *
 * Essentially, the strategy here is this:
 *  - Interpret the arguments to find the maximum value to check
 *  - Find the sieving primes (sieve_seed()).  This does not stop
 *    mid-byte in the sieve, but continues until the end of the byte
 *    containing the last bit needed to be checked to find sieving
 *    primes.  The sieving primes are stored in a prime set.
 *  - Sieve the rest.  This picks up with the next byte written above
 *    and continues to the last byte needed to be checked overall.  The
 *    first unneeded bit of the last byte is calculated and sent so that
 *    the possible leftover bits can be "pruned out."
 *
 * The important thing to note is that each sieve call deals in ranges
 * rounded to the byte.  It seems to be much simpler to do this and then
 * make adjustments to bit-granularity later.
 */
int main(int argc, char * argv[])
{
	uint64_t next_byte, end_byte, seed_end_byte, seed_end_bit, max,
	         seed_max, count;
	unsigned int percent;
	double start, elapsed;
	struct prime_set set;
	enum args_action action;

	/* Save program name, for error messages and such */
	yase_program_name = argv[0];

	/* Process arguments */
	action = process_args(argc, argv, &max);

	/* Act according to the arguments passed */
	switch(action)
	{
		/* Invalid arguments, so fail */
		case ACTION_FAIL:
			return EXIT_FAILURE;

		/* Display help.  We intentionally fall through to display the
		 * version as well. */
		case ACTION_HELP:
			printf(help_format, argv[0]);
			putchar('\n');

		/* Display version */
		case ACTION_VERSION:
			printf("yase version %u.%u.%u\n",
			       VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
			puts("Copyright (c) 2015 Matthew Ingwersen");
			return EXIT_SUCCESS;

		/* Perform sieving */
		case ACTION_SIEVE:
			/* Handled by everything that follows */
			break;
	}

	/* Initialization message */
	printf("yase %u.%u.%u starting, checking numbers <= %" PRIu64 "\n",
	       VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, max);

	/* The sieving skips over all of the wheel primes and the pre-sieved
	   primes, so account for them manually */
	count = WHEEL_PRIMES_SKIPPED + PRESIEVE_PRIMES;

	/* Initialize wheel table */
	puts("Initializing wheel table . . .");
	wheel_init();

	/* Initialize popcnt table */
	puts("Initializing population count table . . .");
	popcnt_init();

	/* Get start CPU time */
	start = clock();

	/* Initialize pre-sieve */
	puts("Initializing pre-sieve . . .");
	presieve_init();

	/*
	 * Calculate the end byte (the first byte that is not touched).
	 * This expression is essentially a variation on the idea that to
	 * find p/q rounding up, you can round down (p+q-1)/q.  28 is used
	 * instead of 29 because the first bit in each byte is 30k+1.  Thus,
	 * 30k should still round down.  Try it with a few numbers--you'll
	 * see that it works.
	 */
	end_byte = ((max + 1) + 28) / 30;

	/*
	 * Calculate the max value to check when finding sieving primes.
	 * This is the largest value such that value * value <= max.
	 */
	seed_max = (uint64_t) sqrt((double) max);

	/* Find the end byte (the first byte that is not touched) for the
	   seed sieve, using the same expression as above */
	seed_end_byte = ((seed_max + 1) + 28) / 30;

	/* Find the first bit that we do not need to check for the seed
	   sieve */
	if(seed_max % 30 == 0)
	{
		seed_end_bit = seed_end_byte * 8;
	}
	else
	{
		seed_end_bit = (seed_end_byte - 1) * 8;
		seed_end_bit += wheel30_last_idx[seed_max % 30] + 1;
	}

	/* The next byte = seed_end_byte */
	next_byte = seed_end_byte;

	/* Initialize prime set */
	puts("Initializing sieving prime set . . .");
	prime_set_init(&set, next_byte, end_byte);

	/* Run the sieve for seeds */
	puts("Finding sieving primes . . .");
	sieve_seed(seed_end_byte, seed_end_bit, &count, &set);

	/* Run the sieve for each segment */
	percent = 0;
	printf("Sieving . . . %u%%", 0);
	fflush(stdout);
	while(next_byte < end_byte)
	{
		/*
		 * A note about the logic here:
		 *  - seg_end_byte = first byte not to process (upper bound)
		 *  - seg_end_bit  = first bit in the last byte processed
		 *                   (seg_end_byte - 1) not to process.  If set
		 *                   to 0, the whole final bit should be used.
		 */
		uint64_t seg_end_byte = next_byte + SEGMENT_BYTES;
		unsigned int seg_end_bit = 0;
		unsigned int new_percent;

		/* If segment is longer than necessary, trim it */
		if(seg_end_byte > end_byte)
		{ 
			seg_end_byte = end_byte;

			/* If max is a multiple of 30, we don't need to prune any
			   extra bits, so leave seg_end_bit = 0.  Otherwise, find
			   the end bit (first bit to be thrown out). */
			if(max % 30 != 0)
			{
				seg_end_bit = (wheel30_last_idx[max % 30] + 1) % 8;
			}
		}

		/* Run the sieve on the segment */
		sieve_segment(next_byte,
		              seg_end_byte,
		              seg_end_bit,
		              &set,
		              &count);

		/* Move forward */
		next_byte = seg_end_byte;
		prime_set_advance(&set);

		/* Update the progress counter if the percentage has changed */
		new_percent = (unsigned int) (next_byte * 100 / end_byte);
		if(new_percent != percent)
		{
			percent = new_percent;
			printf("\rSieving . . . %u%%", percent);
			fflush(stdout);
		}
	}
	putchar('\n');

	/* Perform cleanup (freeing dynamically-allocated memory) */
	puts("Cleaning up . . .");
	prime_set_cleanup(&set);
	presieve_cleanup();
	
	/* Print number found and elapsed time */
	elapsed = (clock() - start) / CLOCKS_PER_SEC;
	printf("Found %" PRIu64 " primes in %.2f seconds.\n", count,
	       elapsed);
	return EXIT_SUCCESS;
}
