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
builtin_mpfr_fun_one_arg(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpfr_fun_one_arg_nornd(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpfr_fun_two_arg(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpfr_fun_two_arg_ul(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpz_fun_one_arg(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpz_fun_one_arg_ul(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpz_fun_one_arg_bitcnt(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpz_fun_two_arg(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpz_fun_two_arg_ul(void *priv, char *s, int nargs, num_t * argv)
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
builtin_mpz_fun_two_arg_bitcnt(void *priv, char *s, int nargs, num_t * argv)
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
call_fun(char *s, explist_t l, hashtable_t vartbl)
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

	if (fn->builtin) {
		r = fn->fn(fn->priv, s, nargs, args);
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

	free_safe_mem(BUCKET_MANUAL, args);

	return r;
}


static
num_t
builtin_min(void *priv, char *s, int nargs, num_t * argv)
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
builtin_max(void *priv, char *s, int nargs, num_t * argv)
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
builtin_avg(void *priv, char *s, int nargs, num_t * argv)
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


struct builtin_funcs
{
	const char *name;
	void *priv;
	builtin_func_t fn;
	int minargs;
	int maxargs;
} builtin_funcs[] = {
  { "sqrt"      , mpfr_sqrt      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "cbrt"      , mpfr_cbrt      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "root"      , mpfr_root      , builtin_mpfr_fun_two_arg_ul    , 2, 2     },
  { "abs"       , mpfr_abs       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "ln"        , mpfr_log       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "log2"      , mpfr_log2      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "log10"     , mpfr_log10     , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "exp"       , mpfr_exp       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "sec"       , mpfr_sec       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "csc"       , mpfr_csc       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "cot"       , mpfr_cot       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "cos"       , mpfr_cos       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "sin"       , mpfr_sin       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "tan"       , mpfr_tan       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "acos"      , mpfr_acos      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "asin"      , mpfr_asin      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "atan"      , mpfr_atan      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "atan2"     , mpfr_atan2     , builtin_mpfr_fun_two_arg       , 2, 2     },
  { "cosh"      , mpfr_cosh      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "sinh"      , mpfr_sinh      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "tanh"      , mpfr_tanh      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "sech"      , mpfr_sech      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "csch"      , mpfr_csch      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "coth"      , mpfr_coth      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "acosh"     , mpfr_acosh     , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "asinh"     , mpfr_asinh     , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "atanh"     , mpfr_atanh     , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "erf"       , mpfr_erf       , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "erfc"      , mpfr_erfc      , builtin_mpfr_fun_one_arg       , 1, 1     },
  { "hypot"     , mpfr_hypot     , builtin_mpfr_fun_two_arg       , 2, 2     },
  { "round"     , mpfr_round     , builtin_mpfr_fun_one_arg_nornd , 1, 1     },
  { "ceil"      , mpfr_ceil      , builtin_mpfr_fun_one_arg_nornd , 1, 1     },
  { "floor"     , mpfr_floor     , builtin_mpfr_fun_one_arg_nornd , 1, 1     },
  { "trunc"     , mpfr_trunc     , builtin_mpfr_fun_one_arg_nornd , 1, 1     },
  { "int"       , mpfr_trunc     , builtin_mpfr_fun_one_arg_nornd , 1, 1     },

  { "nextprime" , mpz_nextprime  , builtin_mpz_fun_one_arg        , 1, 1     },
  { "gcd"       , mpz_gcd        , builtin_mpz_fun_two_arg        , 2, 2     },
  { "lcm"       , mpz_lcm        , builtin_mpz_fun_two_arg        , 2, 2     },
  { "remfac"    , mpz_remove     , builtin_mpz_fun_two_arg        , 2, 2     },
  { "bin"       , mpz_bin_ui     , builtin_mpz_fun_two_arg_ul     , 2, 2     },
  { "fib"       , mpz_fib_ui     , builtin_mpz_fun_one_arg_ul     , 1, 1     },
  { "invert"    , mpz_invert     , builtin_mpz_fun_two_arg        , 2, 2     },
  { "inv"       , mpz_invert     , builtin_mpz_fun_two_arg        , 2, 2     },

  { "hamdist"   , mpz_hamdist    , builtin_mpz_fun_two_arg_bitcnt , 2, 2     },
  { "countones" , mpz_popcount   , builtin_mpz_fun_one_arg_bitcnt , 1, 1     },
  { "popcount"  , mpz_popcount   , builtin_mpz_fun_one_arg_bitcnt , 1, 1     },
  { "popcnt"    , mpz_popcount   , builtin_mpz_fun_one_arg_bitcnt , 1, 1     },

  { "min"       , NULL           , builtin_min                    , 2, 1000  },
  { "max"       , NULL           , builtin_max                    , 2, 1000  },
  { "avg"       , NULL           , builtin_avg                    , 2, 1000  },

  { NULL        , NULL           , NULL                           , 0, 0     }
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
};


static
int
_name_comp(const void *a, const void *b)
{
	return strcmp(* (char * const *) a, * (char * const *) b);
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

