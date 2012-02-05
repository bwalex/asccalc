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

typedef struct hashobj
{
	char *str;
	void *data;

	struct hashobj *next;
	struct hashobj *prev;
} *hashobj_t;


typedef void (*hashtable_ctor_t) (hashobj_t);
typedef void (*hashtable_dtor_t) (hashobj_t);
typedef void (*hashtable_iterator_t) (void *, hashobj_t);

typedef struct hashtable
{
	unsigned int len;

	hashtable_ctor_t ctor;
	hashtable_dtor_t dtor;

	hashobj_t table[1];
} *hashtable_t;



hashtable_t hashtable_new(unsigned int len, hashtable_ctor_t ctor,
    hashtable_dtor_t dtor);
hashobj_t hashtable_lookup(hashtable_t tbl, const char *needle, int alloc);
void hashtable_remove(hashtable_t tbl, const char *needle);
void hashtable_destroy(hashtable_t tbl);
void hashtable_iterate(hashtable_t tbl, hashtable_iterator_t fn, void *priv);

