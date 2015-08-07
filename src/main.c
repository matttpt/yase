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
	uint64_t seed_end_byte, max, count;
	unsigned int seed_end_bit;
	struct interval inter;
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

	/* Calculate start and end values */
	calculate_interval(0, max, &inter);

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
