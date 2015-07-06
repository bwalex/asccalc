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
#include "func.h"
#include "safe_mem.h"

ast_t
ast_new(optype_t type, ast_t l, ast_t r)
{
	ast_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	a->op_type = type;
	a->l = l;
	a->r = r;

	return a;
}


ast_t
ast_newcall(char *s, explist_t l)
{
	astcall_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	a->op_type = OP_CALL;
	a->name = s;
	a->l = l;

	return (ast_t) a;
}


ast_t
ast_newref(char *s)
{
	astref_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	/*
	 * NOTE: we don't resolve the name now for the benefit of
	 *       (user defined) functions that access global variables.
	 *
	 *       When these functions access global variables, they
	 *       should get the freshest value associated to that name.
	 */
	a->op_type = OP_VARREF;
	a->name = s;

	return (ast_t) a;
}


ast_t
ast_newassign(char *s, ast_t v)
{
	astassign_t a;

	assert(s != NULL);

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	a->op_type = OP_VARASSIGN;
	a->name = s;
	a->v = v;

	return (ast_t) a;
}


ast_t
ast_newnum(numtype_t type, char *str)
{
	astnum_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	a->num = num_new_from_str(0, type, str);
	a->op_type = OP_NUM;

	return (ast_t) a;
}


ast_t
ast_newpsel(pseltype_t type, ast_t l, ast_t hi, ast_t lo)
{
	astpsel_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	a->op_type = OP_PSEL;
	a->l = l;

	a->psel_type = type;
	a->hi = hi;
	a->lo = lo;

	return (ast_t) a;
}


ast_t
ast_newcmp(cmptype_t ct, ast_t l, ast_t r)
{
	astcmp_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	a->op_type = OP_CMP;
	a->cmp_type = ct;
	a->l = l;
	a->r = r;

	return (ast_t) a;
}


ast_t
ast_newflow(flowtype_t ft, ast_t c, ast_t t, ast_t f)
{
	astflow_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	a->op_type = OP_FLOW;
	a->flow_type = ft;
	a->cond = c;
	a->t = t;
	a->f = f;

	return (ast_t) a;
}


explist_t
ast_newexplist(ast_t exp, explist_t next)
{
	explist_t el;

	if ((el = alloc_safe_mem(BUCKET_AST, sizeof(*el))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	el->ast = exp;
	el->next = next;

	return el;
}


namelist_t
ast_newnamelist(char *s, namelist_t next)
{
	namelist_t nl;

	if ((nl = alloc_safe_mem(BUCKET_AST, sizeof(*nl))) == NULL) {
		yyxerror("ENOMEM");
		exit(1);
	}

	nl->name = s;
	nl->next = next;

	return nl;
}


static
void
explist_delete(explist_t e)
{
	explist_t c;

	while (e != NULL) {
		c = e;
		e = e->next;
		ast_delete(c->ast);
		free_safe_mem(BUCKET_AST, c);
	}
}


void
namelist_delete(namelist_t e)
{
	namelist_t c;

	while (e != NULL) {
		c = e;
		e = e->next;
		free(c->name);
		free_safe_mem(BUCKET_AST, c);
	}
}



num_t
eval(ast_t a, hashtable_t vartbl)
{
	astcmp_t acmp;
	astflow_t af;
	astpsel_t ap;
	num_t n, c, l, r, hi, lo;
	var_t var;

	assert(a != NULL);

	switch (a->op_type) {
	case OP_CMP:
		acmp = (astcmp_t)a;
		l = eval(acmp->l, vartbl);
		r = eval(acmp->r, vartbl);
		if (l == NULL || r == NULL)
			return NULL;
		n = num_cmp(acmp->cmp_type, l, r);
		break;

	case OP_LISTING:
		eval(a->l, vartbl);
		n = eval(a->r, vartbl);
		break;

	case OP_FLOW:
		af = (astflow_t)a;
		switch (af->flow_type) {
		case FLOW_IF:
			c = eval(af->cond, vartbl);
			if (c == NULL)
				return NULL;
			if (!num_is_zero(c)) {
				if (af->t == NULL)
					n = num_new_const_zero(N_TEMP);
				else
					n = eval(af->t, vartbl);
			} else {
				if (af->f == NULL)
					n = num_new_const_zero(N_TEMP);
				else
					n = eval(af->f, vartbl);
			}
			break;

		case FLOW_WHILE:
			n = num_new_const_zero(N_TEMP);
			c = eval(af->cond, vartbl);
			while (!num_is_zero(c)) {
				n = eval(af->t, vartbl);
				c = eval(af->cond, vartbl);
			}
			break;
		}
		break;

	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
	case OP_POW:
		l = eval(a->l, vartbl);
		r = eval(a->r, vartbl);
		if (l == NULL || r == NULL)
			return NULL;
		n = num_float_two_op(a->op_type, l, r);
		break;

	case OP_AND:
	case OP_OR:
	case OP_XOR:
	case OP_SHR:
	case OP_SHL:
		l = eval(a->l, vartbl);
		r = eval(a->r, vartbl);
		if (l == NULL || r == NULL)
			return NULL;
		n = num_int_two_op(a->op_type, l, r);
		break;

	case OP_UMINUS:
		l = eval(a->l, vartbl);
		if (l == NULL)
			return NULL;
		n = num_float_one_op(a->op_type, l);
		break;

	case OP_UINV:
	case OP_FAC:
		l = eval(a->l, vartbl);
		if (l == NULL)
			return NULL;
		n = num_int_one_op(a->op_type, l);
		break;

	case OP_PSEL:
		ap = (astpsel_t)a;
		l = eval(ap->l, vartbl);
		if (l == NULL)
			return NULL;
		hi = eval(ap->hi, vartbl);
		if (hi == NULL)
			return NULL;
		if (ap->lo != NULL) {
			lo = eval(ap->lo, vartbl);
			if (lo == NULL)
				return NULL;
		} else {
			lo = NULL;
		}
		n = num_int_part_sel(ap->psel_type, hi, lo, l);
		break;

	case OP_NUM:
		n = ((astnum_t) a)->num;
		break;

	case OP_VARREF:
		var = NULL;
		if (vartbl != NULL)
			var = ext_varlookup(vartbl, ((astref_t) a)->name, 0);

		if (var == NULL) {
			var = varlookup(((astref_t) a)->name, 0);
			if (var == NULL) {
				yyxerror("Variable '%s' not defined",
					((astref_t) a)->name);
				return NULL;
			}
		}
		n = var->v;
		break;

	case OP_VARASSIGN:
		l = eval(((astassign_t)a)->v, vartbl);
		if (l == NULL)
			return NULL;

		var = NULL;
		if (vartbl != NULL)
			var = ext_varlookup(vartbl, ((astassign_t) a)->name, 0);
		else
			var = varlookup(((astassign_t) a)->name, 0);

		if (var != NULL) {
			/* dispose of old var first, unless it's the same as is being returned */
			if (!var->no_numfree && (l != var->v))
				num_delete(var->v);
		} else {
			if (vartbl != NULL)
				var = ext_varlookup(vartbl, ((astassign_t) a)->name, 1);
			else
				var = varlookup(((astassign_t) a)->name, 1);
		}

		n = var->v = num_new_z_or_fp(0, l);
		break;

	case OP_CALL:
		n = call_fun(((astcall_t) a)->name, ((astcall_t) a)->l, vartbl);
		break;

	default:
		yyxerror("Unknown op type %d", a->op_type);
	}

	return n;
}


void
ast_delete(ast_t a)
{
	astnum_t an;
	astcall_t ac;
	astref_t ar;
	astassign_t aa;
	astcmp_t acmp;
	astflow_t af;
	astpsel_t ap;


	switch (a->op_type) {
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
	case OP_POW:
	case OP_AND:
	case OP_OR:
	case OP_XOR:
	case OP_SHR:
	case OP_SHL:
	case OP_ASR:
	case OP_ASL:
	case OP_FAC:
	case OP_UMINUS:
	case OP_UINV:
	case OP_LISTING:
		if (a->l != NULL)
			ast_delete(a->l);
		if (a->r != NULL)
			ast_delete(a->r);
		break;

	case OP_CMP:
		acmp = (astcmp_t)a;
		if (acmp->l != NULL)
			ast_delete(acmp->l);
		if (acmp->r != NULL)
			ast_delete(acmp->r);
		break;

	case OP_FLOW:
		af = (astflow_t)a;
		if (af->cond != NULL)
			ast_delete(af->cond);
		if (af->t != NULL)
			ast_delete(af->t);
		if (af->f != NULL)
			ast_delete(af->f);
		break;

	case OP_NUM:
		an = (astnum_t)a;
		num_delete(an->num);
		break;


	case OP_CALL:
		ac = (astcall_t)a;
		free(ac->name);
		explist_delete(ac->l);
		break;

	case OP_VARREF:
		ar = (astref_t)a;
		free(ar->name);
		break;

	case OP_VARASSIGN:
		aa = (astassign_t)a;
		ast_delete(aa->v);
		free(aa->name);
		break;

	case OP_PSEL:
		ap = (astpsel_t)a;
		ast_delete(ap->l);
		ast_delete(ap->hi);
		if (ap->lo != NULL)
			ast_delete(ap->lo);
		break;

	default:
		yyxerror("Unknown type in ast_delete: %d", a->op_type);
	}

	free_safe_mem(BUCKET_AST, a);
}
