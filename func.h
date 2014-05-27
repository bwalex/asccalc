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

typedef num_t(*builtin_func_t) (void *, char *, int, num_t *);

typedef int (*mpfr_fun_one_arg_t) (mpfr_t, mpfr_t, mpfr_rnd_t);
typedef int (*mpfr_fun_one_arg_nornd_t) (mpfr_t, mpfr_t);
typedef int (*mpfr_fun_two_arg_t) (mpfr_t, mpfr_t, mpfr_t, mpfr_rnd_t);
typedef int (*mpfr_fun_two_arg_ul_t) (mpfr_t, mpfr_t, unsigned long,
    mpfr_rnd_t);

typedef void (*mpz_fun_one_arg_t) (mpz_t, mpz_t);
typedef void (*mpz_fun_one_arg_ul_t) (mpz_t, unsigned long);
typedef mp_bitcnt_t (*mpz_fun_one_arg_bitcnt_t) (mpz_t);
typedef void (*mpz_fun_two_arg_t) (mpz_t, mpz_t, mpz_t);
typedef void (*mpz_fun_two_arg_ul_t) (mpz_t, mpz_t, unsigned long);
typedef mp_bitcnt_t (*mpz_fun_two_arg_bitcnt_t) (mpz_t, mpz_t);

typedef struct func
{
	void *priv;
	builtin_func_t fn;
	int minargs;
	int maxargs;
	int builtin;

	namelist_t namelist;
	ast_t ast;
} *func_t;



int funinit(void);
func_t funlookup(const char *s, int alloc);
num_t call_fun(char *s, explist_t l, hashtable_t vartbl);
void funlist(void);
void user_newfun(char *name, namelist_t nl, ast_t a);

