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

typedef struct ast
{
	optype_t op_type;

	struct ast *l;
	struct ast *r;
} *ast_t;


typedef struct astnum
{
	optype_t op_type;

	numtype_t num_type;
	num_t num;
} *astnum_t;


typedef struct astref
{
	optype_t op_type;

	char *name;
} *astref_t;


typedef struct astassign
{
	optype_t op_type;

	var_t var;
	ast_t v;
} *astassign_t;


typedef struct explist
{
	ast_t ast;
	struct explist *next;
} *explist_t;


typedef struct astcall
{
	optype_t op_type;

	char *name;
	explist_t l;
} *astcall_t;





ast_t ast_new(optype_t type, ast_t l, ast_t r);
ast_t ast_newcall(char *s, explist_t l);
ast_t ast_newref(char *s);
ast_t ast_newassign(char *s, ast_t v);
ast_t ast_newnum(numtype_t type, char *str);
explist_t ast_newexplist(ast_t exp, explist_t next);
void ast_delete(ast_t a);
