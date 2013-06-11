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

typedef enum OP_TYPE
{
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_POW,
	OP_AND,
	OP_FAC,
	OP_OR,
	OP_XOR,
	OP_SHR,
	OP_SHL,
	OP_ASR,
	OP_ASL,
	OP_UMINUS,
	OP_UINV,
	OP_CALL,
	OP_VARREF,
	OP_VARASSIGN,
	OP_NUM,
	OP_CMP,
	OP_LISTING,
	OP_FLOW,
	OP_PSEL
} optype_t;


typedef enum CMP_TYPE
{
	CMP_GE,
	CMP_LE,
	CMP_NE,
	CMP_EQ,
	CMP_GT,
	CMP_LT
} cmptype_t;


typedef enum FLOW_TYPE
{
	FLOW_IF,
	FLOW_WHILE
} flowtype_t;

typedef enum PSEL_TYPE
{
	PSEL_SINGLE, /* single bit */
	PSEL_FRANGE, /* fixed range */
	PSEL_DRANGE  /* descending dynamic range */
} pseltype_t;
