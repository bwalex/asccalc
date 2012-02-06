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

ast_t
ast_new(optype_t type, ast_t l, ast_t r)
{
	ast_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyerror("ENOMEM");
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
		yyerror("ENOMEM");
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
		yyerror("ENOMEM");
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
		yyerror("ENOMEM");
		exit(1);
	}

	a->op_type = OP_VARASSIGN;
	a->var = varlookup(s, 1);
	a->v = v;

	free(s);

	return (ast_t) a;
}


ast_t
ast_newnum(numtype_t type, char *str)
{
	astnum_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyerror("ENOMEM");
		exit(1);
	}

	a->num = num_new_from_str(0, type, str);
	a->op_type = OP_NUM;

	return (ast_t) a;
}


ast_t
ast_newcmp(cmptype_t ct, ast_t l, ast_t r)
{
	astcmp_t a;

	if ((a = alloc_safe_mem(BUCKET_AST, sizeof(*a))) == NULL) {
		yyerror("ENOMEM");
		exit(1);
	}

	a->op_type = OP_CMP;
	a->cmp_type = ct;
	a->l = l;
	a->r = r;

	return (ast_t) a;
}


explist_t
ast_newexplist(ast_t exp, explist_t next)
{
	explist_t el;

	if ((el = alloc_safe_mem(BUCKET_AST, sizeof(*el))) == NULL) {
		yyerror("ENOMEM");
		exit(1);
	}

	el->ast = exp;
	el->next = next;

	return el;
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
ast_delete(ast_t a)
{
	astnum_t an;
	astcall_t ac;
	astref_t ar;
	astassign_t aa;
	astcmp_t acmp;

	
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
		if (a->l != NULL)
			ast_delete(a->l);
		if (a->r != NULL)
			ast_delete(a->r);
		break;

	case OP_CMP:
		acmp = (astcmp_t)a;
		if (acmp->l != NULL)
			ast_delete(a->l);
		if (acmp->r != NULL)
			ast_delete(a->r);
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
		break;

	default:
		yyerror("Unknown type in ast_delete: %d", a->op_type);
	}

	free_safe_mem(BUCKET_AST, a);
}
