/*
 * yase - Yet Another Sieve of Eratosthenes
 * expr.c: lexing and parsing/evaluation of math expressions
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
#include <ctype.h>
#include <errno.h>
#include <yase.h>

/*
 * This is a lexer/parser/evaluator for mathematical expressions, here
 * so that command line arguments and such can be written out as
 * expressions (e.g. 2^32-1 instead of 4294967295).  It supports
 * addition, subtraction, multiplication, and exponentiation, as well as
 * "e" notation.  The rough grammar is as follows:
 *
 *   expression     -> sum <end of string>
 *   sum            -> term [+/- term]...
 *   term           -> exponentiation [* exponentiation]...
 *   exponentiation -> literal [** exponentiation]
 */

/* Token type enumeration */
enum token_type
{
	TOK_LITERAL,
	TOK_ADD,
	TOK_SUBTRACT,
	TOK_MULTIPLY,
	TOK_RAISE,
};

/* Token for parsing mathematical expressions */
struct token
{
	enum token_type type; /* Token type                */
	struct token * next;  /* Next token in list        */
	uint64_t value;       /* Only used for TOK_LITERAL */
};

/* Gives textual names to each token type */
const char * type_to_string(enum token_type type)
{
	switch(type)
	{
		case TOK_LITERAL:  return "number";
		case TOK_ADD:      return "'+'";
		case TOK_SUBTRACT: return "'-'";
		case TOK_MULTIPLY: return "'*'";
		case TOK_RAISE:    return "'**' or '^'";
	}
	return "";
}

/**********************************************************************\
 * Parsing/evaluation.  Here we essentially have a recursive descent  *
 * parser that, instead of producing an AST or something like that,   *
 * evaluates the expression as it parses it.                          *
\**********************************************************************/

/* Accepts a certain type of token, returning true if it is the next
   token in the list */
static int accept(struct token * toks, enum token_type type)
{
	if(toks != NULL && toks->type == type)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* Expects a certain type of token, printing out an error message and
   returning false if it is not next */
static int expect(struct token * toks, enum token_type type)
{
	int res = accept(toks, type);
	if(!res)
	{
		fprintf(stderr, "expected %s\n", type_to_string(type));
		return 0;
	}
	else
	{
		return 1;
	}
}

/* Parses/evaluates exponentiation */
static int parse_exp(struct token ** toks_out, uint64_t * result)
{
	struct token * toks = *toks_out;

	/* Expect a literal */
	if(!expect(toks, TOK_LITERAL)) return 0;
	*result = toks->value;
	toks    = toks->next;

	/* If we have an ^ / ** token, keep going */
	if(accept(toks, TOK_RAISE))
	{
		uint64_t i, base, power;

		/* Expect another exponentiation */
		toks = toks->next;
		if(!parse_exp(&toks, &power)) return 0;
		base = *result;
		for(i = 1; i < power; i++)
		{
			*result *= base;
		}
	}
	*toks_out = toks;
	return 1;
}

/* Parses/evaluates a term */
static int parse_term(struct token ** toks_out, uint64_t * result)
{
	struct token * toks = *toks_out;

	/* Expect an exponential */
	if(!parse_exp(&toks, result)) return 0;

	/* If we have a * token, keep going */
	while(accept(toks, TOK_MULTIPLY))
	{
		uint64_t tmp;

		/* Expect another exponential */
		toks = toks->next;
		if(!parse_exp(&toks, &tmp)) return 0;
		*result *= tmp;
	}
	*toks_out = toks;
	return 1;
}

/* Parses/evaluates a sum */
static int parse_sum(struct token ** toks_out, uint64_t * result)
{
	struct token * toks = *toks_out;

	/* Expect a term */
	if(!parse_term(&toks, result)) return 0;

	/* If we have a + or - token, keep going */
	while(accept(toks, TOK_ADD) || accept(toks, TOK_SUBTRACT))
	{
		uint64_t tmp;
		enum token_type op;

		/* Remember operation to perform */
		op = toks->type;

		/* Expect another term */
		toks = toks->next;
		if(!parse_term(&toks, &tmp)) return 0;

		/* Add or subtract accordingly */
		if(op == TOK_ADD) *result += tmp;
		else              *result -= tmp;
	}
	*toks_out = toks;
	return 1;
}

/* Parses/evaluates the entire expression */
static int parse(struct token * toks, uint64_t * result)
{
	if(!parse_sum(&toks, result)) return 0;
	if(toks != NULL)              return 0;
	return 1;
}

/**********************************************************************\
 * Lexer/tokenization                                                 *
\**********************************************************************/

/* Helper to add a token to a list */
static void add_token(struct token ** toks, struct token * new)
{
	struct token * tok;
	if(*toks == NULL)
	{
		*toks = new;
	}
	else
	{
		tok = *toks;
		while(tok->next != NULL)
		{
			tok = tok->next;
		}
		tok->next = new;
	}
}

/* Helper to allocate a new token */
static struct token * alloc_token(void)
{
	struct token * tok = malloc(sizeof(struct token));
	if(tok == NULL)
	{
		perror("malloc");
		abort();
	}
	return tok;
}

/* Helper to free a list of tokens */
static void free_tokens(struct token * toks)
{
	while(toks != NULL)
	{
		struct token * tmp = toks;
		toks = toks->next;
		free(tmp);
	}
}

/* Produces a numeric literal token, adding it to the given list and
   updating the next character */
static int tokenize_literal(const char ** expr, struct token ** toks)
{
	struct token * new;
	uint64_t value;
	char * local_expr, * end_ptr;

	/* Read value */
	errno = 0;
	value = (uint64_t) strtoull(*expr, &end_ptr, 10);
	if(errno != 0)
	{
		perror("strtoull");
		return 0;
	}
	local_expr = end_ptr;

	/* If followed by 'e' or 'E', interpret as scientific notation */
	if(*local_expr == 'e' || *local_expr == 'E')
	{
		unsigned long exp, i;

		local_expr++;
		errno = 0;
		exp = (unsigned long) strtoul(local_expr, &end_ptr, 10);
		if(errno != 0)
		{
			perror("strtoul");
			return 0;
		}
		for(i = 0; i < exp; i++)
		{
			if(value > UINT64_MAX / 10 + 1)
			{
				fputs("numeric literal would overflow\n", stderr);
				return 0;
			}
			value *= 10;
		}
	}

	/* Update pointer for caller */
	*expr = end_ptr;

	/* Allocate and set up new token */
	new        = alloc_token();
	new->type  = TOK_LITERAL;
	new->next  = NULL;
	new->value = value;

	/* Add to list of tokens */
	add_token(toks, new);
	return 1;
}

/* Produces an operand token, adding it to the given list and updating
   the next character */
static int tokenize_operator(const char ** expr, struct token ** toks)
{
	struct token * new;
	enum token_type type;

	/* Determine operand type */
	switch(**expr)
	{
		case '+':
			type = TOK_ADD;
			(*expr)++;
			break;
		case '-':
			type = TOK_SUBTRACT;
			(*expr)++;
			break;
		case '*':
			if(*(*expr + 1) == '*')
			{
				type = TOK_RAISE;
				(*expr) += 2;
			}
			else
			{
				type = TOK_MULTIPLY;
				(*expr)++;
			}
			break;
		case '^':
			type = TOK_RAISE;
			(*expr)++;
			break;
		default:
			return 0;
	}

	/* Allocate and set up new token */
	new       = alloc_token();
	new->type = type;
	new->next = NULL;

	/* Add to list of tokens */
	add_token(toks, new);
	return 1;
}

/* Turns an expression into tokens.  The list of tokens is added to the
   given in toks.  (If the list is empty (i.e. toks == NULL), the first
   list pointer is updated.  This is what a pointer to a pointer is
   needed.) */
static int tokenize(const char * expr, struct token ** toks)
{
	/* Read in tokens based on the first character of each */
	while(*expr != '\0')
	{
		/* If whitespace, ignore.  If a digit, tokenize an integer
		   literal.  If the beginning of an operator, tokenize and
		   operator.  Otherwise, fail. */
		if(isspace(*expr))
		{
			expr++;
		}
		else if(isdigit(*expr))
		{
			if(!tokenize_literal(&expr, toks)) return 0;
		}
		else if(*expr == '+' || *expr == '-' ||
		        *expr == '*' || *expr == '^')
		{
			if(!tokenize_operator(&expr, toks)) return 0;
		}
		else
		{
			fprintf(stderr, "unexpected '%c'\n",
			        (int) *expr);
			return 0;
		}
	}
	return 1;
}

/**********************************************************************\
 * Global wrapper around tokenize() and parse()                       *
\**********************************************************************/

/* Evaluates a numeric expression */
int evaluate(const char * expr, uint64_t * result)
{
	struct token * toks = NULL;

	/* Tokenize */
	if(!tokenize(expr, &toks))
	{
		return 0;
	}

	/* Parse/evaluate */
	if(!parse(toks, result))
	{
		free_tokens(toks);
		return 0;
	}

	free_tokens(toks);
	return 1;
}
