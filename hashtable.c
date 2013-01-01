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
#include "hashtable.h"

static int
_hash(const char *str)
{
	unsigned char *p = (unsigned char *) str;
	unsigned int h = 0;

	while (*p != '\0')
		h = (h << 4) ^ (h >> 28) ^ *p++;

	return h;
}


hashtable_t
hashtable_new(unsigned int len, hashtable_ctor_t ctor, hashtable_dtor_t dtor)
{
	hashtable_t tbl;
	size_t allocsz = sizeof(*tbl) + len * sizeof(struct hashobj *);

	if ((tbl = malloc(allocsz)) == NULL) {	/* BUCKET_MANUAL */
		fprintf(stderr,
		    "Could not allocate memory for hash table of size %d\n",
		    len);
		return NULL;
	}

	memset(tbl, 0, allocsz);

	tbl->len = len;
	tbl->ctor = ctor;
	tbl->dtor = dtor;

	return tbl;
}


hashobj_t
hashtable_lookup(hashtable_t tbl, const char *needle, int alloc)
{
	hashobj_t obj = NULL;
	hashobj_t nobj;
	unsigned int idx;

	idx = _hash(needle) % tbl->len;

	obj = tbl->table[idx];
	while ((obj != NULL) && ((strcmp(obj->str, needle)) != 0))
		obj = obj->next;

	if ((obj == NULL) && alloc) {
		if ((nobj = malloc(sizeof(*nobj))) == NULL) {
			fprintf(stderr,
			    "Could not allocate new object '%s'\n", needle);
			return NULL;
		}

		memset(nobj, 0, sizeof(*nobj));
		nobj->str = strdup(needle);

		if (tbl->ctor != NULL)
			tbl->ctor(nobj);

		obj = tbl->table[idx];
		if (obj != NULL) {
			while (obj->next != NULL)
				obj = obj->next;

			nobj->prev = obj;
			obj->next = nobj;
		} else {
			tbl->table[idx] = nobj;
		}

		obj = nobj;
	}

	return obj;
}


static void
_hashtable_delete(hashtable_t tbl, hashobj_t obj, unsigned int idx)
{
	if (obj->prev != NULL)
		obj->prev->next = obj->next;

	if (obj->next != NULL)
		obj->next->prev = obj->prev;

	if (tbl->dtor != NULL)
		tbl->dtor(obj);

	free(obj->str);
	obj->str = NULL;

	free(obj);

	if (obj == tbl->table[idx])
		tbl->table[idx] = NULL;
}


void
hashtable_remove(hashtable_t tbl, const char *needle)
{
	hashobj_t obj = hashtable_lookup(tbl, needle, 0);
	unsigned int idx;

	idx = _hash(needle) % tbl->len;

	_hashtable_delete(tbl, obj, idx);
}


void
hashtable_destroy(hashtable_t tbl)
{
	hashobj_t obj;
	unsigned int i;

	for (i = 0; i < tbl->len; i++)
		while ((obj = tbl->table[i]) != NULL)
			_hashtable_delete(tbl, obj, i);

	tbl->len = 0;
	free(tbl);
}


void
hashtable_iterate(hashtable_t tbl, hashtable_iterator_t fn, void *priv)
{
	hashobj_t obj;
	unsigned int i;

	for (i = 0; i < tbl->len; i++)
		for (obj = tbl->table[i]; obj != NULL; obj = obj->next)
			fn(priv, obj);
}
