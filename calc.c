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

#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <signal.h>
#include <assert.h>

#include <gmp.h>
#include <mpfr.h>

#include "linenoise.h"

#include "hashtable.h"

#include "optype.h"
#include "num.h"
#include "var.h"
#include "ast.h"
#include "calc.h"
#include "func.h"
#include "safe_mem.h"
#include "calc.tab.h"
#include "lex.yy.h"

static
char *
filename_in_home(const char *fname)
{
	static char p[4096];
	struct passwd *pw = getpwuid(getuid());

	snprintf(p, sizeof(p)-1, "%s/%s", pw->pw_dir, fname);

	return p;
}


void
yyxerror(const char *s, ...)
{
	va_list ap;

	va_start(ap, s);

	fprintf(stderr, "%d: error: ", 0 /* XXX: HACK! */);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");

	//nesting = 0;
	/* XXX: hack ^ */
}

static
char *
prompt(struct parse_ctx *ctx)
{
	static char buf[1024];
	int i;

	if (ctx->nesting || ctx->linecont) {
		snprintf(buf, sizeof(buf), "..");
		for (i = 0; i < ctx->nesting; i++)
			strcat(buf, "..");

		strcat(buf, " ");
	} else {
		snprintf(buf, sizeof(buf), "-> ");
	}

	return buf;
}


struct comp_helper {
	void *priv;
	const char *filter;
	size_t filter_len;
};


static
void
_var_completion_filter(void *priv, const char *var_name)
{
	struct comp_helper *ph = priv;

	if (strncmp(var_name, ph->filter, ph->filter_len) == 0) {
		linenoiseAddCompletion(ph->priv, var_name);
	}
}


static
void
linenoise_completion(const char *buf, linenoiseCompletions *lc) {
	struct comp_helper h;

	if (buf[0] == '\0') {
		linenoiseAddCompletion(lc, "ans ");
	} else {
		h.priv = lc;
		h.filter = buf;
		h.filter_len = strlen(buf);
		var_iterate(&h, _var_completion_filter);
	}
}


int
yywrap(void *scanner)
{
	struct parse_ctx *ctx;
	char *line;
	int len;

	ctx = yyget_extra(scanner);
	if (ctx->interactive) {
		if (ctx->nesting || ctx->linecont) {
			if ((line = linenoise(prompt(ctx))) != NULL) {
				linenoiseHistoryAdd(line);
				len = strlen(line);
				line = realloc(line, len+2);
				line[len] = '\n';
				line[len+1] = '\0';
				yy_scan_string(line, scanner);
				free(line);
				return 0;
			} else {
				fprintf(stderr, "Interrupted, exiting.\n");
				graceful_exit();
			}
		}
	}

	return 1;
}

// TODO: join line continuations together before saving into history

void
graceful_exit(void)
{
	int error;

	if (isatty(fileno(stdin))) {
		if ((error = linenoiseHistorySave(HISTORY_FILE)) == 0) {
			printf("Saved command history to %s\n", HISTORY_FILE);
		} else {
			fprintf(stderr, "Couldn't save command history!\n");
		}
	}

	exit(0);
}

static
void
sig_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
		fprintf(stderr, "Caught SIGTERM, exiting.\n");
		graceful_exit();
		break;
	case SIGQUIT:
		fprintf(stderr, "Caught SIGQUIT, exiting.\n");
		graceful_exit();
		break;
	default:
		fprintf(stderr, "Caught unexpected signal=%d\n", sig);
	}
}

static
int
require_fd(FILE *fp, const char *fname, int silent)
{
	struct parse_ctx ctx;

	ctx.interactive = 0;
	ctx.silent = silent;
	ctx.linecont = ctx.nesting = 0;
	ctx.filename = fname;
	yylex_init(&ctx.scanner);
	yyset_extra(&ctx, ctx.scanner);
	yyset_in(fp, ctx.scanner);
	yyparse(&ctx);
	yylex_destroy(ctx.scanner);

	return 0;
}
int
require_file(const char *file, int silent)
{
	FILE *fp;
	int error;

	if ((fp = fopen(file, "r")) == NULL)
		return -1;

	error = require_fd(fp, file, silent);

	if (isatty(fileno(stdin))) {
		if (error)
			printf("Error loading file: %s\n", file);
		else
			printf("Loaded file: %s\n", file);
	}

	fclose(fp);

	return error;
}

static
int
dot_filter(const struct dirent *de)
{
	if (de->d_name[0] == '.')
		return 0;
	return 1;
}

static
void
load_rc(void)
{
	struct dirent **namelist;
	char p[4096];
	FILE *fp;
	DIR *dp;
	int n;

	/* Load rc file, if it exists */
	require_file(RC_FILE, 1);

	/* Load each file, in order, in the rc directory */
	if ((n = scandir(RC_DIRECTORY, &namelist, dot_filter, alphasort)) >= 0) {
		for (int i = 0; i < n; i++) {
			snprintf(p, sizeof(p)-1, "%s/%s", RC_DIRECTORY, namelist[i]->d_name);
			require_file(p, 1);
			free(namelist[i]);
		}
		free(namelist);
	}
}

int
main(int argc, char *argv[])
{
	struct parse_ctx ctx;
	char *progname = argv[0];
	char *line;
	int len;
	int error;

	varinit();
	num_init();
	funinit();

	signal(SIGTERM, sig_handler);
	signal(SIGQUIT, sig_handler);

	linenoiseHistorySetMaxLen(MAX_HIST_LEN);
	linenoiseSetCompletionCallback(linenoise_completion);

	if (isatty(fileno(stdin))) {
		printf("ascalc %d.%d - A Simple Console Calculator\n",
		    MAJ_VER, MIN_VER);
		printf("Copyright (c) 2012-2015 Alex Hornung\n\n");

		load_rc();

		if ((error = linenoiseHistoryLoad(HISTORY_FILE)) == 0) {
			printf("Loaded previous command history from %s\n\n", HISTORY_FILE);
		}
		printf("Type 'help' for available commands\n");
		printf("\n");

		ctx.interactive = 1;
		ctx.silent = 0;
		ctx.linecont = ctx.nesting = 0;
		ctx.filename = "<stdin>";
		yylex_init(&ctx.scanner);
		yyset_extra(&ctx, ctx.scanner);

		while ((line = linenoise(prompt(&ctx))) != NULL) {
			linenoiseHistoryAdd(line);
			len = strlen(line);
			line = realloc(line, len + 2);
			line[len] = '\n';
			line[len+1] = '\0';
			yy_scan_string(line, ctx.scanner);
			free(line);
			yyparse(&ctx);
		}
		fprintf(stderr, "Interrupted, exiting.\n");
		graceful_exit();
	} else {
		load_rc();
		require_fd(stdin, "<stdin>", 0);
	}

	return 0;
}


static char mode = 'd';
mpfr_rnd_t round_mode = MPFR_RNDN;
static int scientific_mode = 0;

void
mode_switch(char new_mode)
{
	mode = new_mode;
	round_mode = (new_mode == 'd' || new_mode == 's') ? MPFR_RNDN : MPFR_RNDZ;
	mpfr_set_default_rounding_mode(round_mode);
	scientific_mode = (new_mode == 's');
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
			yyxerror("ENOMEM");
			exit(1);
		}
		mpfr_printf("%s%s\n", prefix, s);
		free(s);
	} else if (a->num_type == NUM_FP) {
		if (scientific_mode) {
			mpfr_printf("%.R*G\n", round_mode, F(a));
		} else if (mpfr_integer_p(F(a))) {
			mpfr_printf("%.0R*f\n", round_mode, F(a));
		} else {
			mpfr_printf("%.R*f\n", round_mode, F(a));
		}
	} else {
		printf("invalid!\n");
	}
}


extern int nallocations;

void
go(struct parse_ctx *ctx, ast_t a)
{
	var_t var;
	num_t ans, old_ans = NULL;

	//printf("Allocations: %d\n", nallocations);
	ans = eval(a, NULL);

	if (ans == NULL)
		return;

	if (!ctx->silent) {
		if (isatty(fileno(stdin)))
			printf("ans = ");
		test_print_num(ans);
	}

	var = varlookup("ans", 1);
	if (var->v != NULL)
		old_ans = var->v;

	var->v = num_new_fp(0, ans);

	if (old_ans != NULL)
		num_delete(old_ans);

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
	printf("\t\t\t  of the following: b,d,s,h,o,x - for binary, decimal, \n");
	printf("\t\t\t  scientific decimal, hexadecimal, octal, hexadecimal, \n");
	printf("\t\t\t  output\n\n");
	printf("\tquit\t\t- Exits the program\n\n");
	printf("\texit\t\t- Exits the program\n\n");
}
