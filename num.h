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

typedef enum NUM_TYPE
{
	NUM_INVALID = 0,
	NUM_INT,
	NUM_FP
} numtype_t;


typedef struct num
{
	numtype_t num_type;

	union
	{
		mpz_t z;
		mpfr_t f;
	} v;
} *num_t;


#define N_TEMP 0x01

#define F(n) (n->v.f)
#define Z(n) (n->v.z)

num_t num_new(int flags);
num_t num_new_z(int flags, num_t b);
num_t num_new_fp(int flags, num_t b);
num_t num_new_z_or_fp(int flags, num_t b);
num_t num_new_from_str(int flags, numtype_t typehint, char *str);
num_t num_new_const_pi(int flags);
num_t num_new_const_catalan(int flags);
num_t num_new_const_e(int flags);
num_t num_new_const_zero(int flags);

void num_delete(num_t a);
void num_delete_temp(void);

num_t num_int_two_op(optype_t op_type, num_t a, num_t b);
num_t num_int_one_op(optype_t op_type, num_t a);
num_t num_int_part_sel(pseltype_t op_type, num_t hi, num_t lo, num_t a);
num_t num_float_two_op(optype_t op_type, num_t a, num_t b);
num_t num_float_one_op(optype_t op_type, num_t a);
num_t num_cmp(cmptype_t ct, num_t a, num_t b);
int num_is_zero(num_t a);

void num_init(void);




