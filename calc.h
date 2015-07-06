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

#ifndef _CALC_H
#define _CALC_H

#include "parse_ctx.h"

#define MAX_HIST_LEN	1000
#define HISTORY_FILE	filename_in_home(".asccalc.history")
#define RC_DIRECTORY	filename_in_home(".asccalc.rc.d")
#define RC_FILE		filename_in_home(".asccalc.rc")

#define BUCKET_MANUAL 0
#define BUCKET_AST 1
#define BUCKET_NUM 2
#define BUCKET_NUM_TEMP 3
#define BUCKET_VAR 4
#define BUCKET_FUN 5

extern mpfr_rnd_t round_mode;

void go(struct parse_ctx *ctx, ast_t a);
void test_print_num(num_t n);
void yyxerror(const char *s, ...);
void free_temp_bucket(void);
void help(void);
int yy_input_helper(char *buf, size_t max_size);
int yyparse(struct parse_ctx *ctx);
void mode_switch(char new_mode);
void graceful_exit(void);
#endif
