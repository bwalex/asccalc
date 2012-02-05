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

#include <stdlib.h>
#include <stdio.h>
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
#include "hashtable.h"
#include "safe_mem.h"

static hashtable_t vartbl;


static
void
_initconstants(void)
{
	var_t var;

	var = varlookup("pi", 1);
	var->v = num_new_const_pi(0);

	var = varlookup("G", 1);
	var->v = num_new_const_catalan(0);

	var = varlookup("e", 1);
	var->v = num_new_const_e(0);
}


static
void
varhashdtor(hashobj_t obj)
{
	var_t var;

	assert(obj != NULL);

	if ((var = obj->data) != NULL) {
		num_delete(var->v);
		free_safe_mem(BUCKET_VAR, var);
	}
}


int
varinit(void)
{
	vartbl = hashtable_new(9901, NULL, varhashdtor);
	_initconstants();

	return 0;
}


var_t
varlookup(const char *s, int alloc)
{
	var_t var;
	hashobj_t obj = hashtable_lookup(vartbl, s, alloc);

	if (obj == NULL)
		return NULL;

	if ((var = obj->data) != NULL)
		return var;

	if ((var = alloc_safe_mem(BUCKET_VAR, sizeof(*var))) == NULL) {	/* BUCKET_MANUAL */
		yyerror("ENOMEM");
		exit(1);
	}

	obj->data = var;

	return var;
}


struct var_iteration {
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
_var_iterator(void *priv, hashobj_t obj)
{
	struct var_iteration *ip = priv;

	if (ip->count == ip->allocsize) {
		ip->allocsize += 32;
		ip->s = realloc(ip->s, ip->allocsize * sizeof(char *));
		if (ip->s == NULL) {
			yyerror("ENOMEM");
			exit(1);
		}
	}

	ip->s[ip->count++] = obj->str;
}


void
varlist(void)
{
	struct var_iteration i;
	var_t var;
	int n;

	i.s = NULL;
	i.count = 0;
	i.allocsize = 0;

	hashtable_iterate(vartbl, _var_iterator, &i);
	qsort(i.s, i.count, sizeof(char *), _name_comp);

	for (n = 0; n < i.count; n++) {
		printf("%s = ", i.s[n]);
		var = varlookup(i.s[n], 0);
		test_print_num(var->v);
	}
}

