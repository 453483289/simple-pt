/*
 * Copyright (c) 2015, Intel Corporation
 * Author: Andi Kleen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */



#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "symtab.h"

struct symtab *symtabs;

/* caller must fill in st->end */
struct symtab *add_symtab(unsigned num, unsigned long cr3,
			  unsigned long base, char *fn)
{
	struct symtab *st = malloc(sizeof(struct symtab));
	if (!st)
		exit(ENOMEM);
	st->num = num;
	st->next = symtabs;
	symtabs = st;
	st->cr3 = cr3;
	st->base = base;
	st->syms = malloc(num * sizeof(struct sym));
	if (!st->syms)
		exit(ENOMEM);
	st->end = 0;
	st->fn = fn ? strdup(fn) : NULL;
	return st;
}

int cmp_sym(const void *ap, const void *bp)
{
	const struct sym *a = ap;
	const struct sym *b = bp;
	if (a->val >= b->val && a->val < b->val + b->size)
		return 0;
	if (b->val >= a->val && b->val < a->val + a->size)
		return 0;
	return a->val - b->val;
}

struct sym *findsym(unsigned long val, unsigned long cr3)
{
	struct symtab *st;
	struct sym search = { .val = val }, *s;

	/* add last hit cache here */

	for (st = symtabs; st; st = st->next) {
		if (st->cr3 && cr3 && cr3 != st->cr3)
			continue;
		if (val < st->base || val >= st->end)
			continue;
		s = bsearch(&search, st->syms,  st->num, sizeof(struct sym), cmp_sym);
		if (s)
			return s;
	}
	return NULL;
}

char *find_ip_fn(unsigned long val, unsigned long cr3)
{
	struct symtab *st;
	for (st = symtabs; st; st = st->next) {
		if (st->cr3 && cr3 && cr3 != st->cr3)
			continue;
		if (val < st->base || val >= st->end)
			continue;
		return st->fn;
	}
	return NULL;
}

bool seen_cr3(unsigned long cr3)
{
	struct symtab *st;

	for (st = symtabs; st; st = st->next) {
		if (st->cr3 == cr3)
			return true;
	}
	return false;
}

void dump_symtab(struct symtab *st)
{
	int j;
	for (j = 0; j < st->num; j++) {
		struct sym *s = &st->syms[j];
		if (s->val && s->name[0])
			printf("%lx %s\n", s->val, s->name);
	}
}

void sort_symtab(struct symtab *st)
{
	qsort(st->syms, st->num, sizeof(struct sym), cmp_sym);
}
