/*
 * Copyright (c) 2012 Alex Hornung <alex@alexhornung.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <gmp.h>
#include <mpfr.h>

#include "hashtable.h"

#include "optype.h"
#include "num.h"
#include "var.h"
#include "ast.h"
#include "calc.h"
#include "safe_mem.h"
#include "func.h"

static hashtable_t funtbl;


static
num_t
builtin_mpfr_fun_one_arg(void *priv, const char *s, int nargs, num_t * argv)
{
	mpfr_fun_one_arg_t fn = priv;
	num_t r;
	num_t a;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, argv[0]);

	fn(F(r), F(a), round_mode);

	return r;
}


static
num_t
builtin_mpfr_fun_one_arg_nornd(void *priv, const char *s, int nargs, num_t * argv)
{
	mpfr_fun_one_arg_nornd_t fn = priv;
	num_t r;
	num_t a;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, argv[0]);

	fn(F(r), F(a));

	return r;
}


static
num_t
builtin_mpfr_fun_two_arg(void *priv, const char *s, int nargs, num_t * argv)
{
	mpfr_fun_two_arg_t fn = priv;
	num_t r;
	num_t a, b;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, argv[0]);
	b = num_new_fp(N_TEMP, argv[1]);

	fn(F(r), F(a), F(b), round_mode);

	return r;
}


static
num_t
builtin_mpfr_fun_two_arg_ul(void *priv, const char *s, int nargs, num_t * argv)
{
	mpfr_fun_two_arg_ul_t fn = priv;
	num_t r;
	num_t a, b;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, argv[0]);
	b = num_new_fp(N_TEMP, argv[1]);

	if (!mpfr_fits_ulong_p(F(b), round_mode)) {
		yyxerror
		    ("Second argument to '%s' needs to fit into an unsigned long C datatype");
		return NULL;
	}

	fn(F(r), F(a), mpfr_get_ui(F(b), round_mode), round_mode);

	return r;
}


static
num_t
builtin_mpz_fun_one_arg(void *priv, const char *s, int nargs, num_t * argv)
{
	mpz_fun_one_arg_t fn = priv;
	num_t r;
	num_t a;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);

	fn(Z(r), Z(a));

	return r;
}


static
num_t
builtin_mpz_fun_one_arg_ul(void *priv, const char *s, int nargs, num_t * argv)
{
	mpz_fun_one_arg_ul_t fn = priv;
	num_t r;
	num_t a;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);

	if (!mpz_fits_ulong_p(Z(a))) {
		yyxerror
		    ("Argument to '%s' needs to fit into an unsigned long C datatype");
		return NULL;
	}

	fn(Z(r), mpz_get_ui(Z(a)));

	return r;
}


static
num_t
builtin_mpz_fun_one_arg_bitcnt(void *priv, const char *s, int nargs, num_t * argv)
{
	mpz_fun_one_arg_bitcnt_t fn = priv;
	mp_bitcnt_t bitcnt;
	num_t r;
	num_t a;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);

	bitcnt = fn(Z(a));

	mpz_init_set_ui(Z(r), (unsigned long)bitcnt);

	return r;
}


static
num_t
builtin_mpz_fun_two_arg(void *priv, const char *s, int nargs, num_t * argv)
{
	mpz_fun_two_arg_t fn = priv;
	num_t r;
	num_t a, b;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);
	b = num_new_z(N_TEMP, argv[1]);

	fn(Z(r), Z(a), Z(b));

	return r;
}


static
num_t
builtin_mpz_fun_two_arg_ul(void *priv, const char *s, int nargs, num_t * argv)
{
	mpz_fun_two_arg_ul_t fn = priv;
	num_t r;
	num_t a, b;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);
	b = num_new_z(N_TEMP, argv[1]);

	if (!mpz_fits_ulong_p(Z(b))) {
		yyxerror
		    ("Second argument to '%s' needs to fit into an unsigned long C datatype");
		return NULL;
	}

	fn(Z(r), Z(a), mpz_get_ui(Z(b)));

	return r;
}


static
num_t
builtin_mpz_fun_two_arg_bitcnt(void *priv, const char *s, int nargs, num_t * argv)
{
	mpz_fun_two_arg_bitcnt_t fn = priv;
	mp_bitcnt_t bitcnt;
	num_t r;
	num_t a, b;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);
	b = num_new_z(N_TEMP, argv[1]);

	bitcnt = fn(Z(a), Z(b));

	mpz_init_set_ui(Z(r), (unsigned long)bitcnt);

	return r;
}


num_t
call_fun(const char *s, explist_t l, hashtable_t vartbl)
{
	func_t fn;
	explist_t p;
	namelist_t pn;
	num_t *args;
	int nargs, i;
	var_t v;
	num_t r;

	nargs = 0;
	for (p = l; p != NULL; p = p->next)
		++nargs;

	if ((fn = funlookup(s, 0)) == NULL) {
		yyxerror("Unknown function '%s'", s);
		return NULL;
	}

	if (nargs < fn->minargs || nargs > fn->maxargs) {
		if (fn->minargs == fn->maxargs)
			yyxerror("Function '%s' takes exactly %d arguments", s,
			    fn->minargs);
		else
			yyxerror
			    ("Function '%s' takes a minimum of %d and a maximum of %d arguments",
			    s, fn->minargs, fn->maxargs);

		return NULL;
	}

	if (!(fn->flags & FUNC_RAW_ARGS)) {
		if ((args =
			alloc_safe_mem(BUCKET_MANUAL,
			    sizeof(num_t) * nargs)) == NULL) {
			yyxerror("ENOMEM");
			exit(1);
		}

		i = 0;
		for (p = l; p != NULL; p = p->next) {
		  args[i++] = eval(p->ast, vartbl);
		}
	}

	if (fn->builtin) {
		if (!(fn->flags & FUNC_RAW_ARGS)) {
			r = fn->fn(fn->priv, s, nargs, args);
		} else {
			r = fn->fn(vartbl, s, nargs, (void *)l);
		}
	} else {
		hashtable_t argtbl = ext_varinit(121);

		for (pn = fn->namelist, i = 0; pn != NULL; pn = pn->next, i++) {
			v = ext_varlookup(argtbl, pn->name, 1);
			v->v = args[i];
			v->no_numfree = 1;
		}

		r = eval(fn->ast, argtbl);

		hashtable_destroy(argtbl);
	}

	if (!(fn->flags & FUNC_RAW_ARGS))
		free_safe_mem(BUCKET_MANUAL, args);

	return r;
}


static
num_t
builtin_min(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t r;
	num_t t;
	int i;

	r = num_new_fp(N_TEMP, argv[0]);
	for (i = 1; i < nargs; i++) {
		t = num_new_fp(N_TEMP, argv[i]);
		if (mpfr_less_p(F(t), F(r)))
			r = t;
	}

	return r;
}


static
num_t
builtin_max(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t r;
	num_t t;
	int i;

	r = num_new_fp(N_TEMP, argv[0]);
	for (i = 1; i < nargs; i++) {
		t = num_new_fp(N_TEMP, argv[i]);
		if (mpfr_greater_p(F(t), F(r)))
			r = t;
	}

	return r;
}


static
num_t
builtin_avg(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t r;
	num_t t;
	int i;
	unsigned long n = (unsigned long) nargs;

	r = num_new_fp(N_TEMP, argv[0]);
	for (i = 1; i < nargs; i++) {
		t = num_new_fp(N_TEMP, argv[i]);
		mpfr_add(F(r), F(r), F(t), round_mode);
	}

	mpfr_div_ui(F(r), F(r), n, round_mode);

	return r;
}


static
num_t
builtin_sgn(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t a, r;
	int sgn;

	r = num_new_z(N_TEMP, NULL);
	if (argv[0]->num_type == NUM_INT) {
		a = num_new_z(N_TEMP, argv[0]);
		sgn = mpz_sgn(Z(a));
	} else {
		a = num_new_fp(N_TEMP, argv[0]);
		sgn = mpfr_sgn(F(a));
	}
	mpz_set_si(Z(r), (long)sgn);
	return r;
}


static
num_t
builtin_bits(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t a, r;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);
	mpz_set_ui(Z(r), (unsigned long)mpz_sizeinbase(Z(a), 2));
	return r;
}


static
num_t
builtin_msb(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t a, r;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);
	if (mpz_sgn(Z(a)) <= 0) {
		yyxerror("msb: argument must be a positive integer");
		return NULL;
	}
	mpz_set_ui(Z(r), (unsigned long)(mpz_sizeinbase(Z(a), 2) - 1));
	return r;
}


static
num_t
builtin_ctz(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t a, r;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, argv[0]);
	if (mpz_sgn(Z(a)) == 0) {
		yyxerror("ctz: argument must be non-zero");
		return NULL;
	}
	mpz_set_ui(Z(r), (unsigned long)mpz_scan1(Z(a), 0));
	return r;
}


static
num_t
builtin_deg2rad(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t r, a, pi;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, argv[0]);
	pi = num_new_const_pi(0);

	mpfr_mul_si(F(r), F(pi), 2, round_mode);
	mpfr_div_si(F(r), F(r), 360, round_mode);
	mpfr_mul(F(r), F(r), F(a), round_mode);

	return r;
}


static
num_t
builtin_rad2deg(void *priv, const char *s, int nargs, num_t * argv)
{
	num_t r, a, pi;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, argv[0]);
	pi = num_new_const_pi(0);

	mpfr_mul_si(F(r), F(pi), 2, round_mode);
	mpfr_div_si(F(r), F(r), 360, round_mode);
	mpfr_div(F(r), F(a), F(r), round_mode);

	return r;
}


static
num_t
builtin_tabulate(void *priv, const char *s, int nargs, num_t * argv)
{
	hashtable_t vartbl = (hashtable_t)priv;
	explist_t p, l;
	ast_t first_arg_ast;
	const char *fn_name;
	func_t fn;
	num_t *results;
	num_t *args;
	int i;
	int n, maxarglen, maxreslen;
	char buf[256];
	char buf2[256];

	l = (explist_t)argv;
	p = l;

	first_arg_ast = p->ast;
	if (first_arg_ast->op_type != OP_VARREF) {
		yyxerror("Function 'tabulate' takes a varref as first argument");
		return NULL;
	}

	fn_name = ((astref_t) first_arg_ast)->name;

	if ((fn = funlookup(fn_name, 0)) == NULL) {
		yyxerror("Unknown function '%s'", fn_name);
		return NULL;
	}

	--nargs;

	if ((args =
		alloc_safe_mem(BUCKET_NUM_TEMP,
		    sizeof(num_t) * nargs)) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	if ((results =
		alloc_safe_mem(BUCKET_NUM_TEMP,
		    sizeof(num_t) * nargs)) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	maxarglen = maxreslen = 0;
	for (p = l->next, i = 0; p != NULL; p = p->next, ++i) {
		struct explist lx;

		args[i] = eval(p->ast, vartbl);

		lx.ast = p->ast;
		lx.next = NULL;
		results[i] = call_fun(fn_name, &lx, vartbl);

		n = num_snprint(buf, sizeof(buf)-1, 0, results[i]);
		if (n > maxreslen)
			maxreslen = n;

		n = num_snprint(buf, sizeof(buf)-1, 0, args[i]);
		if (n > maxarglen)
			maxarglen = n;
	}

	for (i = 0; i < nargs; i++) {
		num_snprint(buf, sizeof(buf)-1, maxarglen, args[i]);
		num_snprint(buf2, sizeof(buf2)-1, maxreslen, results[i]);
		printf("%s | %s\n", buf, buf2);
	}

	return NULL;
}


struct builtin_arg_help
{
	const char *name;
	const char *desc;
};

static const struct builtin_arg_help arg_help_sqrt[] = {
	{ "a", "Number whose square root is returned." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_cbrt[] = {
	{ "a", "Number whose cube root is returned." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_root[] = {
	{ "a", "Value whose root is requested." },
	{ "n", "Degree of the root." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_abs[] = {
	{ "a", "Value whose magnitude is returned." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_sgn[] = {
	{ "a", "Value whose sign is tested." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_log[] = {
	{ "a", "Positive value whose logarithm is returned." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_exp[] = {
	{ "a", "Exponent applied to Euler's number." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_angle_rad[] = {
	{ "a", "Angle in radians." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_inverse_trig[] = {
	{ "a", "Input value whose inverse trigonometric result is requested." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_atan2[] = {
	{ "y", "Vertical component. This is the first argument." },
	{ "x", "Horizontal component. This is the second argument." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_hyperbolic[] = {
	{ "a", "Input value." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_hypot[] = {
	{ "x", "First side length." },
	{ "y", "Second side length." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_round[] = {
	{ "a", "Value to round to the nearest integer." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_ceil[] = {
	{ "a", "Value to round upward." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_floor[] = {
	{ "a", "Value to round downward." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_trunc[] = {
	{ "a", "Value whose fractional part is discarded." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_deg2rad[] = {
	{ "a", "Angle in degrees." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_rad2deg[] = {
	{ "a", "Angle in radians." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_nextprime[] = {
	{ "a", "Integer after which to search for the next prime." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_two_ints[] = {
	{ "a", "First integer." },
	{ "b", "Second integer." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_remfac[] = {
	{ "a", "Value to reduce." },
	{ "b", "Factor to remove repeatedly from a." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_binom[] = {
	{ "a", "Total number of items." },
	{ "b", "Number of selected items." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_fib[] = {
	{ "n", "Index of the Fibonacci number to return." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_inv[] = {
	{ "a", "Value to invert." },
	{ "N", "Modulus." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_hamdist[] = {
	{ "a", "First integer." },
	{ "b", "Second integer." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_popcount[] = {
	{ "a", "Integer whose set bits are counted." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_bits[] = {
	{ "a", "Integer whose representation width is measured." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_msb[] = {
	{ "a", "Positive integer whose highest set bit is located." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_ctz[] = {
	{ "a", "Non-zero integer whose trailing zero bits are counted." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_variadic_values[] = {
	{ "a", "First value." },
	{ "b", "Second value." },
	{ "...", "Additional values." },
	{ NULL, NULL }
};

static const struct builtin_arg_help arg_help_tabulate[] = {
	{ "f", "Function name to call." },
	{ "a", "First argument expression to evaluate and pass to f." },
	{ "...", "Additional argument expressions to evaluate and pass to f one at a time." },
	{ NULL, NULL }
};

struct builtin_funcs
{
	const char *name;
	void *priv;
	builtin_func_t fn;
	int minargs;
	int maxargs;
	int flags;
	const char *summary;
	const char *returns;
	const struct builtin_arg_help *args;
	const char *example;
} builtin_funcs[] = {
  { "sqrt"      , mpfr_sqrt      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Square root.",
    "The principal square root of a.",
    arg_help_sqrt,
    NULL },
  { "cbrt"      , mpfr_cbrt      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Cube root.",
    "The real cube root of a.",
    arg_help_cbrt,
    NULL },
  { "root"      , mpfr_rootn_ui  , builtin_mpfr_fun_two_arg_ul    , 2, 2   , 0,
    "n-th root.",
    "The n-th root of a.",
    arg_help_root,
    "root(81, 4) => 3" },
  { "abs"       , mpfr_abs       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Absolute value.",
    "The absolute value of a.",
    arg_help_abs,
    NULL },
  { "sgn"       , NULL           , builtin_sgn                    , 1, 1   , 0,
    "Sign test.",
    "-1 if a is negative, 0 if a is zero, or 1 if a is positive.",
    arg_help_sgn,
    NULL },
  { "ln"        , mpfr_log       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Natural logarithm.",
    "The natural logarithm of a.",
    arg_help_log,
    NULL },
  { "log2"      , mpfr_log2      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Base-2 logarithm.",
    "The base-2 logarithm of a.",
    arg_help_log,
    NULL },
  { "log10"     , mpfr_log10     , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Base-10 logarithm.",
    "The base-10 logarithm of a.",
    arg_help_log,
    NULL },
  { "exp"       , mpfr_exp       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Exponential.",
    "Euler's number raised to the power a.",
    arg_help_exp,
    NULL },
  { "sec"       , mpfr_sec       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Secant.",
    "The secant of a.",
    arg_help_angle_rad,
    NULL },
  { "csc"       , mpfr_csc       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Cosecant.",
    "The cosecant of a.",
    arg_help_angle_rad,
    NULL },
  { "cot"       , mpfr_cot       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Cotangent.",
    "The cotangent of a.",
    arg_help_angle_rad,
    NULL },
  { "cos"       , mpfr_cos       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Cosine.",
    "The cosine of a.",
    arg_help_angle_rad,
    NULL },
  { "sin"       , mpfr_sin       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Sine.",
    "The sine of a.",
    arg_help_angle_rad,
    NULL },
  { "tan"       , mpfr_tan       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Tangent.",
    "The tangent of a.",
    arg_help_angle_rad,
    NULL },
  { "acos"      , mpfr_acos      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Inverse cosine.",
    "An angle in radians whose cosine is a.",
    arg_help_inverse_trig,
    NULL },
  { "asin"      , mpfr_asin      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Inverse sine.",
    "An angle in radians whose sine is a.",
    arg_help_inverse_trig,
    NULL },
  { "atan"      , mpfr_atan      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Inverse tangent.",
    "An angle in radians whose tangent is a.",
    arg_help_inverse_trig,
    NULL },
  { "atan2"     , mpfr_atan2     , builtin_mpfr_fun_two_arg       , 2, 2   , 0,
    "Quadrant-aware arctangent.",
    "An angle in radians whose tangent is y / x, using the signs of both arguments to choose the correct quadrant.",
    arg_help_atan2,
    "atan2(1, -1) => 2.35619..." },
  { "cosh"      , mpfr_cosh      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Hyperbolic cosine.",
    "The hyperbolic cosine of a.",
    arg_help_hyperbolic,
    NULL },
  { "sinh"      , mpfr_sinh      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Hyperbolic sine.",
    "The hyperbolic sine of a.",
    arg_help_hyperbolic,
    NULL },
  { "tanh"      , mpfr_tanh      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Hyperbolic tangent.",
    "The hyperbolic tangent of a.",
    arg_help_hyperbolic,
    NULL },
  { "sech"      , mpfr_sech      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Hyperbolic secant.",
    "The hyperbolic secant of a.",
    arg_help_hyperbolic,
    NULL },
  { "csch"      , mpfr_csch      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Hyperbolic cosecant.",
    "The hyperbolic cosecant of a.",
    arg_help_hyperbolic,
    NULL },
  { "coth"      , mpfr_coth      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Hyperbolic cotangent.",
    "The hyperbolic cotangent of a.",
    arg_help_hyperbolic,
    NULL },
  { "acosh"     , mpfr_acosh     , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Inverse hyperbolic cosine.",
    "The inverse hyperbolic cosine of a.",
    arg_help_hyperbolic,
    NULL },
  { "asinh"     , mpfr_asinh     , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Inverse hyperbolic sine.",
    "The inverse hyperbolic sine of a.",
    arg_help_hyperbolic,
    NULL },
  { "atanh"     , mpfr_atanh     , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Inverse hyperbolic tangent.",
    "The inverse hyperbolic tangent of a.",
    arg_help_hyperbolic,
    NULL },
  { "erf"       , mpfr_erf       , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Error function.",
    "The error function of a.",
    arg_help_hyperbolic,
    NULL },
  { "erfc"      , mpfr_erfc      , builtin_mpfr_fun_one_arg       , 1, 1   , 0,
    "Complementary error function.",
    "The complementary error function of a.",
    arg_help_hyperbolic,
    NULL },
  { "hypot"     , mpfr_hypot     , builtin_mpfr_fun_two_arg       , 2, 2   , 0,
    "Hypotenuse.",
    "sqrt(x^2 + y^2).",
    arg_help_hypot,
    NULL },
  { "round"     , mpfr_round     , builtin_mpfr_fun_one_arg_nornd , 1, 1   , 0,
    "Round to nearest integer.",
    "a rounded to the nearest integer.",
    arg_help_round,
    NULL },
  { "ceil"      , mpfr_ceil      , builtin_mpfr_fun_one_arg_nornd , 1, 1   , 0,
    "Round upward.",
    "The smallest integer not less than a.",
    arg_help_ceil,
    NULL },
  { "floor"     , mpfr_floor     , builtin_mpfr_fun_one_arg_nornd , 1, 1   , 0,
    "Round downward.",
    "The largest integer not greater than a.",
    arg_help_floor,
    NULL },
  { "trunc"     , mpfr_trunc     , builtin_mpfr_fun_one_arg_nornd , 1, 1   , 0,
    "Truncate to an integer.",
    "a with its fractional part discarded.",
    arg_help_trunc,
    NULL },
  { "int"       , mpfr_trunc     , builtin_mpfr_fun_one_arg_nornd , 1, 1   , 0,
    "Alias for trunc.",
    "a with its fractional part discarded.",
    arg_help_trunc,
    NULL },
  { "deg2rad"   , NULL           , builtin_deg2rad                , 1, 1   , 0,
    "Degrees to radians.",
    "Angle a converted to radians.",
    arg_help_deg2rad,
    NULL },
  { "rad2deg"   , NULL           , builtin_rad2deg                , 1, 1   , 0,
    "Radians to degrees.",
    "Angle a converted to degrees.",
    arg_help_rad2deg,
    NULL },

  { "nextprime" , mpz_nextprime  , builtin_mpz_fun_one_arg        , 1, 1   , 0,
    "Next prime.",
    "The smallest prime greater than a.",
    arg_help_nextprime,
    NULL },
  { "gcd"       , mpz_gcd        , builtin_mpz_fun_two_arg        , 2, 2   , 0,
    "Greatest common divisor.",
    "The greatest common divisor of a and b.",
    arg_help_two_ints,
    NULL },
  { "lcm"       , mpz_lcm        , builtin_mpz_fun_two_arg        , 2, 2   , 0,
    "Least common multiple.",
    "The least common multiple of a and b.",
    arg_help_two_ints,
    NULL },
  { "remfac"    , mpz_remove     , builtin_mpz_fun_two_arg        , 2, 2   , 0,
    "Remove a repeated factor.",
    "a with all factors of b removed.",
    arg_help_remfac,
    NULL },
  { "bin"       , mpz_bin_ui     , builtin_mpz_fun_two_arg_ul     , 2, 2   , 0,
    "Binomial coefficient.",
    "The number of ways to choose b items from a items.",
    arg_help_binom,
    NULL },
  { "comb"      , mpz_bin_ui     , builtin_mpz_fun_two_arg_ul     , 2, 2   , 0,
    "Binomial coefficient.",
    "The number of ways to choose b items from a items.",
    arg_help_binom,
    NULL },
  { "fib"       , mpz_fib_ui     , builtin_mpz_fun_one_arg_ul     , 1, 1   , 0,
    "Fibonacci number.",
    "The n-th Fibonacci number.",
    arg_help_fib,
    NULL },
  { "invert"    , mpz_invert     , builtin_mpz_fun_two_arg        , 2, 2   , 0,
    "Modular inverse.",
    "A value x such that (a * x) % N == 1, when one exists.",
    arg_help_inv,
    "invert(3, 11) => 4" },
  { "inv"       , mpz_invert     , builtin_mpz_fun_two_arg        , 2, 2   , 0,
    "Modular inverse.",
    "A value x such that (a * x) % N == 1, when one exists.",
    arg_help_inv,
    "inv(3, 11) => 4" },

  { "hamdist"   , mpz_hamdist    , builtin_mpz_fun_two_arg_bitcnt , 2, 2   , 0,
    "Hamming distance.",
    "The number of bit positions where a and b differ.",
    arg_help_hamdist,
    NULL },
  { "countones" , mpz_popcount   , builtin_mpz_fun_one_arg_bitcnt , 1, 1   , 0,
    "Count set bits.",
    "The number of 1-bits in a.",
    arg_help_popcount,
    NULL },
  { "popcount"  , mpz_popcount   , builtin_mpz_fun_one_arg_bitcnt , 1, 1   , 0,
    "Count set bits.",
    "The number of 1-bits in a.",
    arg_help_popcount,
    NULL },
  { "popcnt"    , mpz_popcount   , builtin_mpz_fun_one_arg_bitcnt , 1, 1   , 0,
    "Count set bits.",
    "The number of 1-bits in a.",
    arg_help_popcount,
    NULL },
  { "bits"      , NULL           , builtin_bits                   , 1, 1   , 0,
    "Bit width.",
    "The number of bits needed to represent a.",
    arg_help_bits,
    NULL },
  { "msb"       , NULL           , builtin_msb                    , 1, 1   , 0,
    "Most significant set bit.",
    "The zero-based index of the highest set bit in a.",
    arg_help_msb,
    NULL },
  { "ctz"       , NULL           , builtin_ctz                    , 1, 1   , 0,
    "Count trailing zeros.",
    "The number of consecutive zero bits at the least-significant end of a.",
    arg_help_ctz,
    NULL },

  { "min"       , NULL           , builtin_min                    , 2, 1000, 0,
    "Minimum of all arguments.",
    "The smallest argument.",
    arg_help_variadic_values,
    NULL },
  { "max"       , NULL           , builtin_max                    , 2, 1000, 0,
    "Maximum of all arguments.",
    "The largest argument.",
    arg_help_variadic_values,
    NULL },
  { "avg"       , NULL           , builtin_avg                    , 2, 1000, 0,
    "Average of all arguments.",
    "The arithmetic mean of all arguments.",
    arg_help_variadic_values,
    NULL },

  { "tabulate"  , NULL           , builtin_tabulate               , 2, 1000, FUNC_RAW_ARGS,
    "Print a table of a unary function applied to each following argument expression.",
    "No numeric result. The table is printed directly.",
    arg_help_tabulate,
    "tabulate(sqrt, 1, 4, 9, 16)" },

  { NULL        , NULL           , NULL                           , 0, 0   , 0,
    NULL,
    NULL,
    NULL,
    NULL }
};

static void
_initbuiltin(void)
{
	func_t fn;
	int i;

	for (i = 0; builtin_funcs[i].name != NULL; i++) {
		fn = funlookup(builtin_funcs[i].name, 1);
		fn->priv = builtin_funcs[i].priv;
		fn->fn = builtin_funcs[i].fn;
		fn->minargs = builtin_funcs[i].minargs;
		fn->maxargs = builtin_funcs[i].maxargs;
		fn->builtin = 1;
		fn->flags = builtin_funcs[i].flags;
	}
}


void
user_newfun(char *name, namelist_t nl, ast_t a)
{
	func_t fn;
	namelist_t p;
	int i;

	i = 0;
	for (p = nl; p != NULL; p = p->next)
		++i;

	if ((fn = funlookup(name, 0)) != NULL) {
		  ast_delete(fn->ast);
		  namelist_delete(fn->namelist);
	} else {
		fn = funlookup(name, 1);
	}

	fn = funlookup(name, 1);
	fn->priv = NULL;
	fn->fn = NULL;
	fn->minargs = fn->maxargs = i;
	fn->builtin = 0;
	fn->flags = 0;

	fn->namelist = nl;
	fn->ast = a;

	printf("Defined function '%s'\n", name);
}


static void
funhashdtor(hashobj_t obj)
{
	func_t fn;

	assert(obj != NULL);

	if ((fn = obj->data) != NULL) {
		free_safe_mem(BUCKET_FUN, fn);
	}
}


int
funinit(void)
{
	funtbl = hashtable_new(9901, NULL, funhashdtor);
	_initbuiltin();

	return 0;
}


func_t
funlookup(const char *s, int alloc)
{
	func_t fn;
	hashobj_t obj = hashtable_lookup(funtbl, s, alloc);

	if (obj == NULL)
		return NULL;

	if ((fn = obj->data) != NULL)
		return fn;

	if ((fn = alloc_safe_mem(BUCKET_FUN, sizeof(*fn))) == NULL) {	/* BUCKET_MANUAL */
		yyxerror("ENOMEM");
		exit(1);
	}

	memset(fn, 0, sizeof(*fn));

	obj->data = fn;

	return fn;
}


struct fun_iteration {
	char **s;
	int count;
	int allocsize;
	void *priv;
	var_it_fn fn;
};


static
int
_name_comp(const void *a, const void *b)
{
	return strcmp(* (char * const *) a, * (char * const *) b);
}


static
void
_fun_iterator_gen(void *priv, hashobj_t obj)
{
	struct fun_iteration *ip = priv;
	ip->fn(ip->priv, obj->str);
}

void
fun_iterate(void *priv, var_it_fn fn)
{
	struct fun_iteration it;
	it.fn = fn;
	it.priv = priv;
	it.s = NULL;
	it.count = 0;
	it.allocsize = 0;
	hashtable_iterate(funtbl, _fun_iterator_gen, &it);
}


static
void
_fun_iterator(void *priv, hashobj_t obj)
{
	struct fun_iteration *ip = priv;

	if (ip->count == ip->allocsize) {
		ip->allocsize += 32;
		ip->s = realloc(ip->s, ip->allocsize * sizeof(char *));
		if (ip->s == NULL) {
			yyxerror("ENOMEM");
			exit(1);
		}
	}

	ip->s[ip->count++] = obj->str;
}


void
funlist(void)
{
	struct fun_iteration i;
	func_t f;
	int n;
	int maxlen = 0;
	int len;

	i.s = NULL;
	i.count = 0;
	i.allocsize = 0;

	hashtable_iterate(funtbl, _fun_iterator, &i);
	qsort(i.s, i.count, sizeof(char *), _name_comp);

	for (n = 0; n < i.count; n++) {
		len = strlen(i.s[n]);
		if (len > maxlen)
			maxlen = len;
	}

	maxlen += 2;

	printf("Functions:\n");
	for (n = 0; n < i.count; n++) {
		f = funlookup(i.s[n], 0);
		printf("%s%*s%s\n", i.s[n], maxlen-(int)strlen(i.s[n]), "",
		    f->builtin ? "[builtin]" : "[user-defined]");
	}
}

static const struct builtin_funcs *
_builtin_help_lookup(const char *name)
{
	int i;

	for (i = 0; builtin_funcs[i].name != NULL; i++) {
		if (strcmp(builtin_funcs[i].name, name) == 0)
			return &builtin_funcs[i];
	}

	return NULL;
}

static const char *
_builtin_generic_arg_name(int idx)
{
	static const char *const names[] = {
		"a", "b", "c", "d", "e", "f", "g", "h"
	};
	static char fallback[16];

	if (idx >= 0 && idx < (int)(sizeof(names) / sizeof(names[0])))
		return names[idx];

	snprintf(fallback, sizeof(fallback), "arg%d", idx + 1);
	return fallback;
}

static void
_print_builtin_signature(const struct builtin_funcs *builtin)
{
	int i;

	printf("%s(", builtin->name);
	for (i = 0; i < builtin->minargs; i++) {
		const char *arg_name = _builtin_generic_arg_name(i);

		if (builtin->args != NULL && builtin->args[i].name != NULL)
			arg_name = builtin->args[i].name;
		if (i != 0)
			printf(", ");
		printf("%s", arg_name);
	}
	if (builtin->minargs != builtin->maxargs) {
		if (builtin->minargs != 0)
			printf(", ");
		printf("...");
	}
	printf(") [builtin]\n");
}

static void
_print_user_signature(const char *name, namelist_t namelist)
{
	namelist_t p;
	int first = 1;

	printf("%s(", name);
	for (p = namelist; p != NULL; p = p->next) {
		if (!first)
			printf(", ");
		printf("%s", p->name);
		first = 0;
	}
	printf(") [user-defined]\n");
}

static void
_print_arg_help(const struct builtin_arg_help *args)
{
	int maxlen = 0;
	int len;
	const struct builtin_arg_help *arg;

	if (args == NULL || args[0].name == NULL)
		return;

	for (arg = args; arg->name != NULL; arg++) {
		len = strlen(arg->name);
		if (len > maxlen)
			maxlen = len;
	}

	printf("\nArguments:\n");
	for (arg = args; arg->name != NULL; arg++) {
		printf("  %s%*s%s\n", arg->name,
		    maxlen - (int)strlen(arg->name) + 2, "", arg->desc);
	}
}

static void
_print_returns(const char *returns)
{
	if (returns == NULL || *returns == '\0')
		return;

	printf("\nReturns:\n");
	printf("  %s\n", returns);
}

static void
_print_example(const char *example)
{
	if (example == NULL || *example == '\0')
		return;

	printf("\nExample:\n");
	printf("  %s\n", example);
}

void
funhelp(const char *name)
{
	func_t fn;
	const struct builtin_funcs *builtin;
	namelist_t p;
	int argc = 0;
	struct builtin_arg_help *user_args;

	if ((fn = funlookup(name, 0)) == NULL) {
		yyxerror("Unknown function '%s'", name);
		return;
	}

	if (fn->builtin) {
		builtin = _builtin_help_lookup(name);
		if (builtin == NULL) {
			yyxerror("Missing builtin help for '%s'", name);
			return;
		}

		_print_builtin_signature(builtin);
		if (builtin->summary != NULL && *builtin->summary != '\0')
			printf("  %s\n", builtin->summary);
		_print_arg_help(builtin->args);
		_print_returns(builtin->returns);
		_print_example(builtin->example);
		return;
	}

	for (p = fn->namelist; p != NULL; p = p->next)
		argc++;

	if ((user_args = calloc((size_t)argc + 1, sizeof(*user_args))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	for (p = fn->namelist, argc = 0; p != NULL; p = p->next, argc++) {
		user_args[argc].name = p->name;
		user_args[argc].desc = "User-defined parameter.";
	}

	_print_user_signature(name, fn->namelist);
	printf("  User-defined function. Argument names come from the definition; richer descriptions are unavailable.\n");
	_print_arg_help(user_args);
	_print_returns("Result of the function body.");
	free(user_args);
}


