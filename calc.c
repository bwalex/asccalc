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

#include <histedit.h>

#include "optype.h"
#include "num.h"
#include "var.h"
#include "ast.h"
#include "calc.h"
#include "func.h"
#include "safe_mem.h"
#include "lex.yy.h"

void
yyerror(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);

	fprintf(stderr, "%d: error: ", yylineno);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
}

static
char *
prompt(EditLine *el)
{
	int *lcont;

	el_get(el, EL_CLIENTDATA, &lcont);
	if (*lcont)
		return "... ";
	else
		return "-> ";
}

int
main(int argc, char *argv[])
{
	YY_BUFFER_STATE bp;
	char *s;
	char *progname = argv[0];
	EditLine *el;
	History *hist;
	HistEvent ev;
	int count, error;
	int linecont, linecontchar;
	char *contbuf;
	size_t contbufsz;
	
	varinit();
	num_init();
	funinit();

	hist = history_init();
	history(hist, &ev, H_SETSIZE, 1000);
	el = el_init(progname, stdin, stdout, stderr);
	el_set(el, EL_PROMPT, prompt);
	el_set(el, EL_SIGNAL, 1);
	el_set(el, EL_HIST, history, hist);
	el_set(el, EL_EDITOR, "emacs");
	el_set(el, EL_CLIENTDATA, &linecontchar);

	printf("ascalc - A Simple Console Calculator\n");
	printf("Copyright (c) 2012 Alex Hornung\n");
	printf("Type 'help' for available commands\n");
	printf("\n");
	
	linecont = 0;
	linecontchar = 0;
	contbuf = NULL;
	contbufsz = 0;
	for (;;) {
		s = el_gets(el, &count);
		if (count <= 0)
			continue;

		/*
		 * XXX: This continuation magic is hideous. It really really needs some
		 *      love.
		 *
		 * NOTES:
		 *      count -2 because the last character is \n.
		 *      linecont specifies whether line continuation is currently in use.
		 *      linecontchar specifies whether the current line is still to be
		 *      continued; i.e. it ended with the line continuation character \.
		 */
		if (s[count-2] == '\\') {
			if ((contbuf = realloc(contbuf, contbufsz + count + 2)) == NULL) {
				yyerror("ENOMEM");
				exit(1);
			}

			linecontchar = 1;
			s[count-2] = '\0';
			linecont = 1;

		} else {
			linecontchar = 0;
		}

		if (linecont) {
			contbuf[contbufsz] = '\0';
			sprintf(contbuf + contbufsz, "%s", s);
			contbufsz += count-2;
			if (linecontchar)
				continue;
			else
				s = contbuf;
		}

		history(hist, &ev, H_ENTER, s);

		bp = yy_scan_string(s);
		yy_switch_to_buffer(bp);
		error = yyparse();
		yy_delete_buffer(bp);

		if (linecont && !linecontchar) {
			free(contbuf);
			contbuf = NULL;
			contbufsz = 0;
			linecont = 0;
		}
	}

	history_end(hist);
	el_end(el);
	
	return 0;
}

num_t
eval(ast_t a)
{
	astcmp_t acmp;
	num_t n, l, r;
	var_t var;

	assert(a != NULL);

	switch (a->op_type) {
	case OP_CMP:
		acmp = (astcmp_t)a;
		l = eval(acmp->l);
		r = eval(acmp->r);
		if (l == NULL || r == NULL)
			return NULL;
		n = num_cmp(acmp->cmp_type, l, r);
		break;
		
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
	case OP_POW:
		l = eval(a->l);
		r = eval(a->r);
		if (l == NULL || r == NULL)
			return NULL;
		n = num_float_two_op(a->op_type, l, r);
		break;

	case OP_AND:
	case OP_OR:
	case OP_XOR:
	case OP_SHR:
	case OP_SHL:
		l = eval(a->l);
		r = eval(a->r);
		if (l == NULL || r == NULL)
			return NULL;
		n = num_int_two_op(a->op_type, l, r);
		break;

	case OP_UMINUS:
		l = eval(a->l);
		if (l == NULL)
			return NULL;
		n = num_float_one_op(a->op_type, l);
		break;

	case OP_UINV:
	case OP_FAC:
		l = eval(a->l);
		if (l == NULL)
			return NULL;
		n = num_int_one_op(a->op_type, l);
		break;

	case OP_NUM:
		n = ((astnum_t) a)->num;
		break;

	case OP_VARREF:
		var = varlookup(((astref_t) a)->name, 0);
		if (var == NULL) {
			yyerror("Variable '%s' not defined",
			    ((astref_t) a)->name);
			return NULL;
		}
		n = var->v;
		break;

	case OP_VARASSIGN:
		l = eval(((astassign_t)a)->v);
		if (l == NULL)
			return NULL;
		n = ((astassign_t) a)->var->v = num_new_fp(0, l);
		break;

	case OP_CALL:
		n = call_builtin(((astcall_t) a)->name, ((astcall_t) a)->l);
		break;

	default:
		yyerror("Unknown op type %d", a->op_type);
	}

	return n;
}

static char mode = 'd';

void
mode_switch(char new_mode)
{
	mode = new_mode;
}


void
test_print_num(num_t n)
{
	const char *prefix = "";
	char *s;
	num_t a;
	int base;

	switch (mode) {
	case 'b': base = 2; prefix = "0b"; break;
	case 'd': base = 10; prefix = ""; break;
	case 'x':
	case 'h': base = 16; prefix = "0x"; break;
	case 'o': base = 8; prefix = "0"; break;
	default: base = 10; prefix = "";
	}

	if (base != 10 && n->num_type != NUM_INT)
		a = num_new_z(N_TEMP, n);
	else
		a = n;

	if (a->num_type == NUM_INT) {
		if ((s = mpz_get_str(NULL, base, Z(a))) == NULL) {
			yyerror("ENOMEM");
			exit(1);
		}
		mpfr_printf("%s%s\n", prefix, s);
		free(s);
	} else if (a->num_type == NUM_FP) {
		mpfr_printf("%Rg\n", F(a));
	} else {
		printf("invalid!\n");
	}
}


extern int nallocations;

void
go(ast_t a)
{
	var_t var;
	num_t ans;

	//printf("Allocations: %d\n", nallocations);
	ans = eval(a);

	if (ans == NULL)
		return;

	printf("ans = ");
	test_print_num(ans);

	var = varlookup("ans", 1);
	if (var->v != NULL)
		num_delete(var->v);
	var->v = num_new_fp(0, ans);

	num_delete_temp();
	ast_delete(a);

	//printf("Allocations: %d\n", nallocations);
}


void
help(void)
{
	printf("Commands:\n");

	printf("\tls\t\t- Lists all variables\n\n");

	printf("\tlsfn\t\t- Lists all functions\n\n");

	printf("\tm <MODE>\t- Same as 'mode'\n\n");

	printf("\tmode <MODE>\t- Switches to <MODE>, where mode is one\n");
	printf("\t\t\t  of the following: b,d,h,o,x - for binary, decimal, \n");
	printf("\t\t\t  hexadecimal, octal, hexadecimal output\n\n");
	printf("\tquit\t\t- Exits the program\n\n");
}
