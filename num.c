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

#include "hashtable.h"

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

static
int
num_is_z(num_t a)
{
	if (a == NULL)
		return 1;
	else if (a->num_type == NUM_INT)
		return 1;
	else if (a->num_type == NUM_FP && mpfr_integer_p(F(a)))
		return 1;
	else
		return 0;
}

static
int
num_both_z(num_t a, num_t b)
{
	return num_is_z(a) && num_is_z(b);
}

static
mpfr_prec_t
num_prec(num_t a)
{
	mpfr_prec_t prec;

	if (a != NULL && a->num_type == NUM_INT) {
		/* XXX: completely arbitrary! */
		return 2048;
	} else if (a != NULL && a->num_type == NUM_FP) {
		return mpfr_get_prec(F(a));
	} else {
		return mpfr_get_default_prec();
	}
}

static
mpfr_prec_t
num_max_prec(num_t a, num_t b)
{
	mpfr_prec_t prec_a = num_prec(a);
	mpfr_prec_t prec_b = num_prec(b);

	return (prec_a > prec_b) ? prec_a : prec_b;
}


num_t
num_new(int flags)
{
	num_t r;

	if ((r = alloc_safe_mem((flags & N_TEMP) ? BUCKET_NUM_TEMP : BUCKET_NUM, sizeof(*r))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	memset(r, 0, sizeof(*r));
	return r;
}


num_t
num_new_z_or_fp(int flags, num_t b)
{
	assert (b != NULL);

	if (b->num_type == NUM_INT)
		return num_new_z(flags, b);
	else
		return num_new_fp(flags, b);
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
			mpfr_get_z(Z(r), F(b), round_mode);
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
		mpfr_prec_t prec_b = num_prec(b);
		if (prec_b > mpfr_get_default_prec())
			mpfr_set_prec(F(r), prec_b);

		if (b->num_type == NUM_INT)
			mpfr_set_z(F(r), Z(b), round_mode);
		else if (b->num_type == NUM_FP)
			mpfr_set(F(r), F(b), round_mode);
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
			yyxerror("mpz_init_set_str");
			mpz_clear(Z(n));
		}
	} else {
		if (str[1] == 'd')
			str += 2;

		mpfr_init(F(n));
		r = mpfr_strtofr(F(n), str, &suffix, 0, round_mode);

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
				yyxerror("Unknown suffix");
				exit(1);
			}

			mpfr_mul_d(F(n), F(n), exp, round_mode);
		}
	}

	return n;
}


num_t
num_new_const_pi(int flags)
{
	num_t r;

	r = num_new_fp(flags, NULL);
	mpfr_const_pi(F(r), round_mode);

	return r;
}


num_t
num_new_const_catalan(int flags)
{
	num_t r;

	r = num_new_fp(flags, NULL);
	mpfr_const_catalan(F(r), round_mode);

	return r;
}


num_t
num_new_const_e(int flags)
{
	mpfr_t one;
	num_t r;

	r = num_new_fp(flags, NULL);

	mpfr_init_set_si(one, 1, round_mode);
	mpfr_exp(F(r), one, round_mode);
	mpfr_clear(one);

	return r;
}


num_t
num_new_const_zero(int flags)
{
	num_t r;

	r = num_new_z(flags, NULL);
	mpz_init_set_si(Z(r), 0);

	return r;
}


int
num_is_zero(num_t a)
{
	int nz = 0;

	switch (a->num_type) {
	case NUM_INT:
		nz = mpz_cmp_si(Z(a), 0);
		break;

	case NUM_FP:
		nz = mpfr_cmp_si(F(a), 0);
		break;

	default:
		yyxerror("invalid number at num_is_zero!");
		exit(1);
	}

	return !nz;
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
			yyxerror
			    ("Second argument to shift needs to fit into an unsigned long C datatype");
			return NULL;
		}
		mpz_fdiv_q_2exp(Z(r), Z(a), mpz_get_ui(Z(b)));
		break;

	case OP_SHL:
		if (!mpz_fits_ulong_p(Z(b))) {
			yyxerror
			    ("Second argument to shift needs to fit into an unsigned long C datatype");
			return NULL;
		}
		mpz_mul_2exp(Z(r), Z(a), mpz_get_ui(Z(b)));
		break;

	default:
		yyxerror("Unknown op in num_int_two_op");
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
			yyxerror
			    ("Argument to factorial needs to fit into an unsigned long C datatype");
			return NULL;
		}
		mpz_fac_ui(Z(r), mpz_get_ui(Z(a)));
		break;

	default:
		yyxerror("Unknown op in num_int_one_op");
	}

	return r;
}


num_t
num_int_part_sel(pseltype_t op_type, num_t hi, num_t lo, num_t a)
{
	num_t r, m;
	unsigned long mask_shl, lo_ui;

	r = num_new_z(N_TEMP, NULL);
	m = num_new_z(N_TEMP, NULL);
	a = num_new_z(N_TEMP, a);
	hi = num_new_z(N_TEMP, hi);
	if (lo != NULL)
		lo = num_new_z(N_TEMP, lo);

	switch (op_type) {
	case PSEL_SINGLE:
		lo = num_new_z(N_TEMP, hi);
		break;

	case PSEL_FRANGE:
		break;

	case PSEL_DRANGE:
		if (mpz_cmp_si(Z(lo), 0L) < 0) {
			yyxerror("low index of part select operation must be "
			    "positive");
			return NULL;
		}
		mpz_sub_ui(Z(lo), Z(lo), 1UL);
		mpz_sub(Z(lo), Z(hi), Z(lo));
		break;
	}

	if (mpz_cmp_si(Z(hi), 0L) < 0) {
		yyxerror("high index of part select operation must be "
		    "positive");
		return NULL;
	}

	if (mpz_cmp_si(Z(lo), 0L) < 0) {
		yyxerror("low index of part select operation must be "
		    "positive");
		return NULL;
	}

	if (!mpz_fits_ulong_p(Z(hi))) {
		yyxerror("high index of part select operation needs to fit "
		    "into an unsigned long C datatype");
		return NULL;
	}

	if (!mpz_fits_ulong_p(Z(lo))) {
		yyxerror("low index of part select operation needs to fit "
		    "into an unsigned long C datatype");
		return NULL;
	}

	lo_ui = mpz_get_ui(Z(lo));
	mask_shl = 1 + (mpz_get_ui(Z(hi)) - lo_ui);

	/*
	 * We do the part sel by:
	 *  (1) shifting right by 'lo'
	 *  (2) anding with ((1 << mask_shl)-1)
	 */
	mpz_div_2exp(Z(r), Z(a), lo_ui);
	mpz_set_ui(Z(m), 1UL);
	mpz_mul_2exp(Z(m), Z(m), mask_shl);
	mpz_sub_ui(Z(m), Z(m), 1UL);

	mpz_and(Z(r), Z(r), Z(m));

	return r;
}


num_t
num_float_two_op(optype_t op_type, num_t a, num_t b)
{
	num_t r, r_z, rem_z, a_z, b_z;
	int both_z = num_both_z(a, b);

	r = num_new_fp(N_TEMP, NULL);
	mpfr_set_prec(F(r), num_max_prec(a, b));

	a = num_new_fp(N_TEMP, a);
	b = num_new_fp(N_TEMP, b);

	if (both_z) {
		r_z = num_new_z(N_TEMP, NULL);
		rem_z = num_new_z(N_TEMP, NULL);
		a_z = num_new_z(N_TEMP, a);
		b_z = num_new_z(N_TEMP, b);
	}

	switch (op_type) {
	case OP_ADD:
		if (both_z) {
			mpz_add(Z(r_z), Z(a_z), Z(b_z));
			return r_z;
		} else {
			mpfr_add(F(r), F(a), F(b), round_mode);
		}
		break;

	case OP_SUB:
		if (both_z) {
			mpz_sub(Z(r_z), Z(a_z), Z(b_z));
			return r_z;
		} else {
			mpfr_sub(F(r), F(a), F(b), round_mode);
		}
		break;

	case OP_MUL:
		if (both_z) {
			mpz_mul(Z(r_z), Z(a_z), Z(b_z));
			return r_z;
		} else {
			mpfr_mul(F(r), F(a), F(b), round_mode);
		}
		break;

	case OP_DIV:
		if (both_z)
			mpz_divmod(Z(r_z), Z(rem_z), Z(a_z), Z(b_z));

		if (both_z && num_is_zero(rem_z))
			return r_z;
		else
			mpfr_div(F(r), F(a), F(b), round_mode);
		break;

	case OP_MOD:
		if (both_z) {
			mpz_mod(Z(r_z), Z(a_z), Z(b_z));
			return r_z;
		} else {
			/*
			 * XXX: mpfr_fmod or mpfr_remainder
			 */
			mpfr_fmod(F(r), F(a), F(b), round_mode);
		}
		break;

	case OP_POW:
		mpfr_pow(F(r), F(a), F(b), round_mode);
		break;

	default:
		yyxerror("Unknown op in num_float_two_op");
	}

	return r;
}


num_t
num_cmp(cmptype_t ct, num_t a, num_t b)
{
	num_t r;
	int s;
	int both_z = num_both_z(a, b);

	r = num_new_z(N_TEMP, NULL);

	if (both_z) {
		a = num_new_z(N_TEMP, a);
		b = num_new_z(N_TEMP, b);

		s = mpz_cmp(Z(a), Z(b));
	} else {
		a = num_new_fp(N_TEMP, a);
		b = num_new_fp(N_TEMP, b);

		s = mpfr_cmp(F(a), F(b));
	}

	switch (ct) {
	case CMP_GE: mpz_set_ui(Z(r), (s >= 0) ? 1 : 0); break;
	case CMP_LE: mpz_set_ui(Z(r), (s <= 0) ? 1 : 0); break;
	case CMP_NE: mpz_set_ui(Z(r), (s != 0) ? 1 : 0); break;
	case CMP_EQ: mpz_set_ui(Z(r), (s == 0) ? 1 : 0); break;
	case CMP_GT: mpz_set_ui(Z(r), (s >  0) ? 1 : 0); break;
	case CMP_LT: mpz_set_ui(Z(r), (s <  0) ? 1 : 0); break;
	default:
		yyxerror("Unknown cmp in num_cmp");
	}

	return r;
}


num_t
num_float_one_op(optype_t op_type, num_t a)
{
	num_t r;

	r = num_new_fp(N_TEMP, NULL);
	mpfr_set_prec(F(r), num_prec(a));

	a = num_new_fp(N_TEMP, a);

	switch (op_type) {
	case OP_UMINUS:
		mpfr_neg(F(r), F(a), round_mode);
		break;

	default:
		yyxerror("Unknown op in num_float_two_op");
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

	mpfr_set_default_prec(256);
}
