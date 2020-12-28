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
"Usage: %s [OPTION]... [MIN] MAX\n"
"Count and display the number of primes on the interval [MIN,MAX].  MIN\n"
"and MAX be expressions, e.g. 2^32-1.  Supported operations are addition\n"
"(+), subtraction (-), multiplication (*), and exponentiation (** or ^).\n"
"If MIN is not provided, it is assumed to be 0.\n\n"
"Options:\n"
" --help      display this help meessage\n"
" --version   display version information\n";

/* Table of pi(x) values for x < 30 */
static unsigned int pi_under_30[30] =
{
	0, 0, 1, 2, 2, 3, 3, 4, 4, 4,
	4, 5, 5, 6, 6, 6, 6, 7, 7, 8,
	8, 8, 8, 9, 9, 9, 9, 9, 9, 10
};

/*
 * Main routine!
 *
 * Essentially, the strategy here is this:
 *  - Interpret the arguments to find the range of values to check.
 *  - Find the sieving primes (sieve_seed()).  The sieving primes are
 *    stored in a prime set.
 *  - Sieve the requested interval (sieve_interval()).
 */
int main(int argc, char * argv[])
{
	uint64_t seed_end_byte, min, max, count;
	unsigned int seed_end_bit;
	struct interval inter;
	double start, elapsed;
	struct prime_set set;
	enum args_action action;

	/* Save program name, for error messages and such */
	yase_program_name = argv[0];

	/* Process arguments */
	action = process_args(argc, argv, &min, &max);

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
	printf("yase %u.%u.%u starting, checking numbers on "
	       "[%" PRIu64 ", %" PRIu64"]\n",
	       VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, min, max);

	/* If the maximum is under 30, we handle calculations via table */
	if(max < 30)
	{
		count = pi_under_30[max];
		if(min != 0)
		{
			count -= pi_under_30[min - 1];
		}
		printf("Found %" PRIu64 " primes (via pi(x) table).\n", count);
		return EXIT_SUCCESS;
	}

	/* The sieving skips over all of the wheel primes and the pre-sieved
	   primes, so account for them manually */
	if(min < 30)
	{
		count = WHEEL_PRIMES_SKIPPED + PRESIEVE_PRIMES;
		if(min != 0)
		{
			count -= pi_under_30[min - 1];
		}
	}
	else
	{
		count = 0;
	}

	/* Initialize wheel table */
	puts("Initializing wheel table . . .");
	wheel_init();

	/* Initialize popcnt */
	puts("Initializing population count . . .");
	popcnt_init();

	/* Get start CPU time */
	start = clock();

	/* Initialize pre-sieve */
	puts("Initializing pre-sieve . . .");
	presieve_init();

	/* Calculate start and end values */
	calculate_interval(min, max, &inter);

	/* Calculate seed start and end values */
	calculate_seed_interval(max, &seed_end_byte, &seed_end_bit);

	/* Initialize prime set */
	puts("Initializing sieving prime set . . .");
	prime_set_init(&set, &inter);

	/* Run the sieve for seeds */
	puts("Finding sieving primes . . .");
	sieve_seed(seed_end_byte, seed_end_bit, &set);

	/* Run the main sieve */
	sieve_interval(&inter, &set, &count);

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
