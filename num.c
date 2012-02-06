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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <gmp.h>
#include <mpfr.h>

#include "optype.h"
#include "num.h"
#include "var.h"
#include "ast.h"
#include "calc.h"
#include "safe_mem.h"

void
num_delete(num_t a)
{
	free_safe_mem(BUCKET_NUM, a);
}


void
num_delete_temp(void)
{
	free_safe_mem_bucket(BUCKET_NUM_TEMP);
}


num_t
num_new(int flags)
{
	num_t r;

	if ((r = alloc_safe_mem((flags & N_TEMP) ? BUCKET_NUM_TEMP : BUCKET_NUM, sizeof(*r))) == NULL) {
		yyerror("ENOMEM");
		exit(1);
	}

	memset(r, 0, sizeof(*r));
	return r;
}


num_t
num_new_z(int flags, num_t b)
{
	num_t r;

	r = num_new(flags);
	r->num_type = NUM_INT;

	mpz_init(Z(r));

	if (b != NULL) {
		if (b->num_type == NUM_INT)
			mpz_set(Z(r), Z(b));
		else if (b->num_type == NUM_FP)
			mpfr_get_z(Z(r), F(b), MPFR_RNDN);
	}

	return r;			
}


num_t
num_new_fp(int flags, num_t b)
{
	num_t r;

	r = num_new(flags);
	r->num_type = NUM_FP;

	mpfr_init(F(r));

	if (b != NULL) {
		if (b->num_type == NUM_INT)
			mpfr_set_z(F(r), Z(b), MPFR_RNDN);
		else if (b->num_type == NUM_FP)
			mpfr_set(F(r), F(b), MPFR_RNDN);
	}

	return r;
}


num_t
num_new_from_str(int flags, numtype_t typehint, char *str)
{
	numtype_t type = typehint;
	double exp;
	char *suffix, *s;
	int base, type_override, r;
	num_t n;

	n = num_new(flags);

	/*
	 * If it's a decimal number but it doesn't have a floating point, suffix, etc
	 * treat it as an integer.
	 */
	type_override = 1;
	if (typehint == NUM_FP) {
		s = (str[1] == 'd') ? str + 2 : str;
		while (*s != '\0') {
			if (!isdigit(*s++)) {
				type_override = 0;
				break;
			}
		}

		if (type_override)
			type = NUM_INT;
	}

	n->num_type = type;

	base = 0;

	if (str[1] == 'd') {
		base = 10;
		str += 2;
	}

	if (type == NUM_INT) {
		if ((r = mpz_init_set_str(Z(n), str, base)) != 0) {
			yyerror("mpz_init_set_str");
			mpz_clear(Z(n));
		}
	} else {
		if (str[1] == 'd')
			str += 2;

		mpfr_init(F(n));
		r = mpfr_strtofr(F(n), str, &suffix, 0, MPFR_RNDN);

		/*
		 * XXX: add support for IEC binary prefixes? 
		 */
		if (*suffix != '\0') {
			switch (*suffix) {
			case 'k':
				exp = 1000;
				break;
			case 'M':
				exp = 1000000;
				break;
			case 'G':
				exp = 1000000000;
				break;
			case 'T':
				exp = 1000000000000;
				break;
			case 'P':
				exp = 1000000000000000;
				break;
			case 'E':
				exp = 1000000000000000000;
				break;
			case 'm':
				exp = 0.001;
				break;
			case 'u':
				exp = 0.000001;
				break;
			case 'n':
				exp = 0.000000001;
				break;
			case 'p':
				exp = 0.000000000001;
				break;
			case 'f':
				exp = 0.000000000000001;
				break;
			case 'a':
				exp = 0.000000000000000001;
				break;
			default:
				yyerror("Unknown suffix");
				exit(1);
			}

			mpfr_mul_d(F(n), F(n), exp, MPFR_RNDN);
		}
	}

	return n;
}


num_t
num_new_const_pi(int flags)
{
	num_t r;

	r = num_new_fp(flags, NULL);
	mpfr_const_pi(F(r), MPFR_RNDN);

	return r;
}


num_t
num_new_const_catalan(int flags)
{
	num_t r;

	r = num_new_fp(flags, NULL);
	mpfr_const_catalan(F(r), MPFR_RNDN);

	return r;
}


num_t
num_new_const_e(int flags)
{
	mpfr_t one;
	num_t r;

	r = num_new_fp(flags, NULL);

	mpfr_init_set_si(one, 1, MPFR_RNDN);
	mpfr_exp(F(r), one, MPFR_RNDN);
	mpfr_clear(one);
	
	return r;
}


num_t
num_int_two_op(optype_t op_type, num_t a, num_t b)
{
	num_t r;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, a);
	b = num_new_z(N_TEMP, b);

	switch (op_type) {
	case OP_AND:
		mpz_and(Z(r), Z(a), Z(b));
		break;

	case OP_OR:
		mpz_ior(Z(r), Z(a), Z(b));
		break;

	case OP_XOR:
		mpz_xor(Z(r), Z(a), Z(b));
		break;

	case OP_SHR:
		if (!mpz_fits_ulong_p(Z(b))) {
			yyerror
			    ("Second argument to shift needs to fit into an unsigned long C datatype");
			return NULL;
		}
		mpz_fdiv_q_2exp(Z(r), Z(a), mpz_get_ui(Z(b)));
		break;

	case OP_SHL:
		if (!mpz_fits_ulong_p(Z(b))) {
			yyerror
			    ("Second argument to shift needs to fit into an unsigned long C datatype");
			return NULL;
		}
		mpz_mul_2exp(Z(r), Z(a), mpz_get_ui(Z(b)));
		break;

	default:
		yyerror("Unknown op in num_int_two_op");
	}

	return r;
}


num_t
num_int_one_op(optype_t op_type, num_t a)
{
	num_t r;

	r = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, a);

	switch (op_type) {
	case OP_UINV:
		mpz_com(Z(r), Z(a));
		break;

	case OP_FAC:
		if (!mpz_fits_ulong_p(Z(a))) {
			yyerror
			    ("Argument to factorial needs to fit into an unsigned long C datatype");
			return NULL;
		}
		mpz_fac_ui(Z(r), mpz_get_ui(Z(a)));
		break;

	default:
		yyerror("Unknown op in num_int_two_op");
	}

	return r;
}


num_t
num_float_two_op(optype_t op_type, num_t a, num_t b)
{
	num_t r;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, a);
	b = num_new_fp(N_TEMP, b);

	switch (op_type) {
	case OP_ADD:
		mpfr_add(F(r), F(a), F(b), MPFR_RNDN);
		break;

	case OP_SUB:
		mpfr_sub(F(r), F(a), F(b), MPFR_RNDN);
		break;

	case OP_MUL:
		mpfr_mul(F(r), F(a), F(b), MPFR_RNDN);
		break;

	case OP_DIV:
		mpfr_div(F(r), F(a), F(b), MPFR_RNDN);
		break;

	case OP_MOD:
		/*
		 * XXX: mpfr_fmod or mpfr_remainder 
		 */
		mpfr_fmod(F(r), F(a), F(b), MPFR_RNDN);
		break;

	case OP_POW:
		mpfr_pow(F(r), F(a), F(b), MPFR_RNDN);
		break;

	default:
		yyerror("Unknown op in num_float_two_op");
	}

	return r;
}


num_t
num_cmp(cmptype_t ct, num_t a, num_t b)
{
	num_t r;
	int s;
	
	r = num_new_z(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, a);
	b = num_new_fp(N_TEMP, b);

	s = mpfr_cmp(F(a), F(b));
	
	switch (ct) {
	case CMP_GE: mpz_set_ui(Z(r), (s >= 0) ? 1 : 0); break;
	case CMP_LE: mpz_set_ui(Z(r), (s <= 0) ? 1 : 0); break;
	case CMP_NE: mpz_set_ui(Z(r), (s != 0) ? 1 : 0); break;
	case CMP_EQ: mpz_set_ui(Z(r), (s == 0) ? 1 : 0); break;
	case CMP_GT: mpz_set_ui(Z(r), (s >  0) ? 1 : 0); break;
	case CMP_LT: mpz_set_ui(Z(r), (s <  0) ? 1 : 0); break;
	default:
		yyerror("Unknown cmp in num_cmp");
	}

	return r;
}


num_t
num_float_one_op(optype_t op_type, num_t a)
{
	num_t r;

	r = num_new_fp(N_TEMP, NULL);
	a = num_new_fp(N_TEMP, a);
	
	switch (op_type) {
	case OP_UMINUS:
		mpfr_neg(F(r), F(a), MPFR_RNDN);
		break;

	default:
		yyerror("Unknown op in num_float_two_op");
	}

	return r;
}


static
void
num_dtor(int bucket, void *p)
{
	num_t a = (num_t)p;

	if (a->num_type == NUM_INT)
		mpz_clear(Z(a));
	else if (a->num_type == NUM_FP)
		mpfr_clear(F(a));

	a->num_type = NUM_INVALID;
}


void
num_init(void)
{
	init_safe_mem_bucket(BUCKET_NUM, NULL, num_dtor);
	init_safe_mem_bucket(BUCKET_NUM_TEMP, NULL, num_dtor);
}
