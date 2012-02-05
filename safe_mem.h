/*
 * Copyright (c) 2011 - 2012 Alex Hornung <alex@alexhornung.com>.
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

#define SAFEMEM_NBUCKETS 128

#if SAFEMEM_NBUCKETS > 1
#define alloc_safe_mem(b, x) \
        _alloc_safe_mem(b, x, __FILE__, __LINE__)

#define free_safe_mem(b, x) \
        _free_safe_mem(b, x, __FILE__, __LINE__)
#else
#define alloc_safe_mem(x) \
        _alloc_safe_mem(0, x, __FILE__, __LINE__)

#define free_safe_mem(x) \
        _free_safe_mem(0, x, __FILE__, __LINE__)
#endif

typedef void (*safe_mem_ctor_t) (int, void *);
typedef void (*safe_mem_dtor_t) (int, void *);

void *_alloc_safe_mem(int bucket, size_t req_sz, const char *file, int line);
void _free_safe_mem(int bucket, void *mem, const char *file, int line);
void check_and_purge_safe_mem(void);
void free_safe_mem_bucket(int bucket);
void init_safe_mem_bucket(int bucket, safe_mem_ctor_t ctor,
    safe_mem_dtor_t dtor);
