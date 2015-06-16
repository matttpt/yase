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
#include <errno.h>
#include <yase.h>

/*
 * Main routine!
 *
 * Essentially, the strategy here is this:
 *  - Interpret the arguments to find the maximum value to check
 *  - Find the sieving primes (sieve_seed()).  This does not stop
 *    mid-byte in the sieve, but continues until the end of the byte
 *    containing the last bit needed to be checked to find sieving
 *    primes.  The next sieve byte is written to the pointer passed.
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
	unsigned long next_byte, end_byte, max, count = WHEEL_PRIMES_SKIPPED;
	unsigned int percent;
	double start, elapsed;
	char * strtoul_end;
	struct multiple * multiples;

	/* Validate arguments */
	if(argc != 2)
	{
		fprintf(stderr, "%s: invalid arguments (expected 1, got %d)\n",
		        argv[0], argc - 1);
		return EXIT_FAILURE;
	}

	/* Get the range from the first argument.  errno is set to 0 to
	   be able to distinguish an error condition. */
	errno = 0;
	max = strtoul(argv[1], &strtoul_end, 10);
	if(errno != 0)
	{
		fputs(argv[0], stderr);
		perror(": strtoul");
		fprintf(stderr, "%s: invalid max range `%s'\n", argv[0], argv[1]);
		return EXIT_FAILURE;
	}
	if(*strtoul_end != '\0')
	{
		fprintf(stderr, "%s: junk after integer for max range\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Things break if we are asked to sieve less than one byte's worth
	   of a range.  Thus, max >= 30. */
	if(max < 30)
	{
		fprintf(stderr, "%s: maximum number to check must be >= 30\n",
		        argv[0]);
		return EXIT_FAILURE;
	}

	/* Initialization message */
	printf("yase %u.%u.%u starting, checking numbers <= %lu\n",
	       VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, max);

	/* Initialize wheel table */
	puts("Initializing wheel table . . .");
	wheel_init();

	/* Initialize popcnt table */
	puts("Initializing population count table . . .");
	popcnt_init();

	/* Get start CPU time */
	start = clock();

	/* Run the sieve for seeds */
	puts("Finding sieving primes . . .");
	multiples = sieve_seed(max, &count, &next_byte);

	/*
	 * Calculate the end byte (the first byte that is not touched).
	 * This expression is essentially a variation on the idea that to
	 * find p/q rounding up, you can round down (p+q-1)/q.  28 is used
	 * instead of 29 because the first bit in each byte is 30k+1.  Thus,
	 * 30k should still round down.  Try it with a few numbers--you'll
	 * see that it works.
	 */
	end_byte = ((max + 1) + 28) / 30;
	
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
		unsigned long seg_end_byte = next_byte + SEGMENT_BYTES;
		unsigned long seg_end_bit = 0;
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
				seg_end_bit = (wheel_last_idx[max % 30] + 1) % 8;
			}
		}

		/* Run the sieve on the segment */
		sieve_segment(next_byte,
		              seg_end_byte,
		              seg_end_bit,
		              multiples,
		              &count);

		/* Move forward */
		next_byte = seg_end_byte;

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

	/* Free each multiple */
	puts("Cleaning up . . .");
	while(multiples != NULL)
	{
		struct multiple * to_free = multiples;
		multiples = multiples->next;
		free(to_free);
	}
	
	/* Print number found and elapsed time */
	elapsed = (clock() - start) / CLOCKS_PER_SEC;
	printf("Found %lu primes in %.2f seconds.\n", count, elapsed);
	return EXIT_SUCCESS;
}	
