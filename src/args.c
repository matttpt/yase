/*
 * yase - Yet Another Sieve of Eratosthenes
 * args.c: routines to process arguments
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

/* Processes program arguments, returning the action to take.  If the
   action is ACTION_SIEVE (i.e. normal program execution), this will
   write out the maximum value to check to the pointer passed. */
enum args_action process_args(
		int argc,
		char * argv[],
		uint64_t * min,
		uint64_t * max)
{
	int i;

	/* If any argument is "--help", we will display help information.
	   If any argument is "--version", we will display the version.
	   If both are present, the first one is used. */
	for(i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "--help") == 0)
		{
			return ACTION_HELP;
		}
		else if(strcmp(argv[i], "--version") == 0)
		{
			return ACTION_VERSION;
		}
	}

	/* No version or help flags.  Proceed as usual, checking that we
	   only have one or two real arguments. */
	if(argc != 2 && argc != 3)
	{
		fprintf(stderr, "%s: invalid arguments (expected 1 or 2, got "
		        "%d)\n", yase_program_name, argc - 1);
		return ACTION_FAIL;
	}

	/* Get the minimum and maximum values */
	if(argc == 3)
	{
		/* Get the minimum from the first argument */
		if(!evaluate(argv[1], min))
		{
			fprintf(stderr, "%s: failed to evaluate minimum value\n",
			        yase_program_name);
			return ACTION_FAIL;
		}

		/* Get the minimum from the second argument */
		if(!evaluate(argv[2], max))
		{
			fprintf(stderr, "%s: failed to evaluate maximum value\n",
			        yase_program_name);
			return ACTION_FAIL;
		}
	}
	else
	{
		/* Only the maximum is provided.  Assume the minimum is 0. */
		*min = 0;

		/* Get the maximum from the first argument */
		if(!evaluate(argv[1], max))
		{
			fprintf(stderr, "%s: failed to evaluate maximum value\n",
			        yase_program_name);
			return ACTION_FAIL;
		}
	}

	/* Ensure that max >= min */
	if(*max < *min)
	{
		fprintf(stderr, "%s: minimum is greater than maximum\n",
		        yase_program_name);
		return ACTION_FAIL;
	}

	/* No problems.  We should sieve. */
	return ACTION_SIEVE;
}
