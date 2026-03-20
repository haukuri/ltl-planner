/***** ltl2ba : cache.c *****/

/* Written by Denis Oddoux, LIAFA, France                                 */
/* Copyright (c) 2001  Denis Oddoux                                       */
/* Modified by Paul Gastin, LSV, France                                   */
/* Copyright (c) 2007  Paul Gastin                                        */
/*                                                                        */
/* This program is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License as published by   */
/* the Free Software Foundation; either version 2 of the License, or      */
/* (at your option) any later version.                                    */
/*                                                                        */
/* This program is distributed in the hope that it will be useful,        */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/* GNU General Public License for more details.                           */
/*                                                                        */
/* You should have received a copy of the GNU General Public License      */
/* along with this program; if not, write to the Free Software            */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA*/
/*                                                                        */
/* Based on the translation algorithm by Gastin and Oddoux,               */
/* presented at the 13th International Conference on Computer Aided       */
/* Verification, CAV 2001, Paris, France.                                 */
/* Proceedings - LNCS 2102, pp. 53-65                                     */
/*                                                                        */
/* Send bug-reports and/or questions to Paul Gastin                       */
/* http://www.lsv.ens-cachan.fr/~gastin                                   */
/*                                                                        */
/* Some of the code in this file was taken from the Spin software         */
/* Written by Gerard J. Holzmann, Bell Laboratories, U.S.A.               */

#include "ltl2ba.h"

static int	ismatch(Node *, Node *);

void
cache_dump(Ltl2baContext *ctx)
{	Cache *d; int nr=0;

	printf("\nCACHE DUMP:\n");
	for (d = ctx->cache_stored; d; d = d->nxt, nr++)
	{	if (d->same) continue;
		printf("B%3d: ", nr); dump(ctx, d->before); printf("\n");
		printf("A%3d: ", nr); dump(ctx, d->after); printf("\n");
	}
	printf("============\n");
}

Node *
in_cache(Ltl2baContext *ctx, Node *n)
{	Cache *d; int nr=0;

	for (d = ctx->cache_stored; d; d = d->nxt, nr++)
		if (isequal(ctx, d->before, n))
		{	ctx->CacheHits++;
			if (d->same && ismatch(n, d->before)) return n;
			return dupnode(ctx, d->after);
		}
	return ZN;
}

Node *
cached(Ltl2baContext *ctx, Node *n)
{	Cache *d;
	Node *m;

	if (!n) return n;
	if ((m = in_cache(ctx, n)))
		return m;

	ctx->Caches++;
	d = (Cache *) tl_emalloc(ctx, sizeof(Cache));
	d->before = dupnode(ctx, n);
	d->after  = Canonical(ctx, n); /* n is released */

	if (ismatch(d->before, d->after))
	{	d->same = 1;
		releasenode(ctx, 1, d->after);
		d->after = d->before;
	}
	d->nxt = ctx->cache_stored;
	ctx->cache_stored = d;
	return dupnode(ctx, d->after);
}

void
cache_stats(Ltl2baContext *ctx)
{
	printf("cache stores     : %9ld\n", ctx->Caches);
	printf("cache hits       : %9ld\n", ctx->CacheHits);
}

void
releasenode(Ltl2baContext *ctx, int all_levels, Node *n)
{
	if (!n) return;

	if (all_levels)
	{	releasenode(ctx, 1, n->lft);
		n->lft = ZN;
		releasenode(ctx, 1, n->rgt);
		n->rgt = ZN;
	}
	tfree(ctx, (void *) n);
}

Node *
tl_nn(Ltl2baContext *ctx, int t, Node *ll, Node *rl)
{	Node *n = (Node *) tl_emalloc(ctx, sizeof(Node));

	n->ntyp = (short) t;
	n->lft  = ll;
	n->rgt  = rl;

	return n;
}

Node *
getnode(Ltl2baContext *ctx, Node *p)
{	Node *n;

	if (!p) return p;

	n =  (Node *) tl_emalloc(ctx, sizeof(Node));
	n->ntyp = p->ntyp;
	n->sym  = p->sym; /* same name */
	n->lft  = p->lft;
	n->rgt  = p->rgt;

	return n;
}

Node *
dupnode(Ltl2baContext *ctx, Node *n)
{	Node *d;

	if (!n) return n;
	d = getnode(ctx, n);
	d->lft = dupnode(ctx, n->lft);
	d->rgt = dupnode(ctx, n->rgt);
	return d;
}

int
one_lft(int ntyp, Node *x, Node *in)
{
	if (!x)  return 1;
	if (!in) return 0;

	if (sameform(NULL, x, in))
		return 1;

	if (in->ntyp != ntyp)
		return 0;

	if (one_lft(ntyp, x, in->lft))
		return 1;

	return one_lft(ntyp, x, in->rgt);
}

int
all_lfts(int ntyp, Node *from, Node *in)
{
	if (!from) return 1;

	if (from->ntyp != ntyp)
		return one_lft(ntyp, from, in);

	if (!one_lft(ntyp, from->lft, in))
		return 0;

	return all_lfts(ntyp, from->rgt, in);
}

int
sametrees(int ntyp, Node *a, Node *b)
{	/* toplevel is an AND or OR */
	/* both trees are right-linked, but the leafs */
	/* can be in different places in the two trees */

	if (!all_lfts(ntyp, a, b))
		return 0;

	return all_lfts(ntyp, b, a);
}

int	/* a better isequal() */
sameform(Ltl2baContext *ctx, Node *a, Node *b)
{
	if (!a && !b) return 1;
	if (!a || !b) return 0;
	if (a->ntyp != b->ntyp) return 0;

	if (a->sym
	&&  b->sym
	&&  strcmp(a->sym->name, b->sym->name) != 0)
		return 0;

	switch (a->ntyp) {
	case TRUE:
	case FALSE:
		return 1;
	case PREDICATE:
		if (!a->sym || !b->sym) fatal(ctx, "sameform...");
		return !strcmp(a->sym->name, b->sym->name);

	case NOT:
#ifdef NXT
	case NEXT:
#endif
		return sameform(ctx, a->lft, b->lft);
	case U_OPER:
	case V_OPER:
		if (!sameform(ctx, a->lft, b->lft))
			return 0;
		if (!sameform(ctx, a->rgt, b->rgt))
			return 0;
		return 1;

	case AND:
	case OR:	/* the hard case */
		return sametrees(a->ntyp, a, b);

	default:
		printf("type: %d\n", a->ntyp);
		fatal(ctx, "cannot happen, sameform");
	}

	return 0;
}

int
isequal(Ltl2baContext *ctx, Node *a, Node *b)
{
	if (!a && !b)
		return 1;

	if (!a || !b)
	{	if (!a)
		{	if (b->ntyp == TRUE)
				return 1;
		} else
		{	if (a->ntyp == TRUE)
				return 1;
		}
		return 0;
	}
	if (a->ntyp != b->ntyp)
		return 0;

	if (a->sym
	&&  b->sym
	&&  strcmp(a->sym->name, b->sym->name) != 0)
		return 0;

	if (isequal(ctx, a->lft, b->lft)
	&&  isequal(ctx, a->rgt, b->rgt))
		return 1;

	return sameform(ctx, a, b);
}

static int
ismatch(Node *a, Node *b)
{
	if (!a && !b) return 1;
	if (!a || !b) return 0;
	if (a->ntyp != b->ntyp) return 0;

	if (a->sym
	&&  b->sym
	&&  strcmp(a->sym->name, b->sym->name) != 0)
		return 0;

	if (ismatch(a->lft, b->lft)
	&&  ismatch(a->rgt, b->rgt))
		return 1;

	return 0;
}

int
any_term(Ltl2baContext *ctx, Node *srch, Node *in)
{
	if (!in) return 0;

	if (in->ntyp == AND)
		return	any_term(ctx, srch, in->lft) ||
			any_term(ctx, srch, in->rgt);

	return isequal(ctx, in, srch);
}

int
any_and(Ltl2baContext *ctx, Node *srch, Node *in)
{
	if (!in) return 0;

	if (srch->ntyp == AND)
		return	any_and(ctx, srch->lft, in) &&
			any_and(ctx, srch->rgt, in);

	return any_term(ctx, srch, in);
}

int
any_lor(Ltl2baContext *ctx, Node *srch, Node *in)
{
	if (!in) return 0;

	if (in->ntyp == OR)
		return	any_lor(ctx, srch, in->lft) ||
			any_lor(ctx, srch, in->rgt);

	return isequal(ctx, in, srch);
}

int
anywhere(Ltl2baContext *ctx, int tok, Node *srch, Node *in)
{
	if (!in) return 0;

	switch (tok) {
	case AND:	return any_and(ctx, srch, in);
	case  OR:	return any_lor(ctx, srch, in);
	case   0:	return any_term(ctx, srch, in);
	}
	fatal(ctx, "cannot happen, anywhere");
	return 0;
}
