/***** ltl2ba : mem.c *****/

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

#if 1
#define log(e, u, d)	ctx->mem_event[e][(int) u] += (long) d;
#else
#define log(e, u, d)
#endif

#define A_USER		0x55000000
#define NOTOOBIG	32768

#define POOL		0
#define ALLOC		1
#define FREE		2

union M {
	long size;
	union M *link;
};

void *
tl_emalloc(Ltl2baContext *ctx, int U)
{	union M *m;
  	long r, u;
	void *rp;

	u = (long) ((U-1)/sizeof(union M) + 2);

	if (u >= A_LARGE)
	{	log(ALLOC, 0, 1);
		if (ctx->tl_verbose)
		printf("tl_spin: memalloc %ld bytes\n", u);
		m = (union M *) emalloc(ctx, (int) u*sizeof(union M));
		ctx->All_Mem += (unsigned long) u*sizeof(union M);
	} else
	{	if (!ctx->freelist[u])
		{	r = ctx->mem_req[u] += ctx->mem_req[u] ? ctx->mem_req[u] : 1;
			if (r >= NOTOOBIG)
				r = ctx->mem_req[u] = NOTOOBIG;
			log(POOL, u, r);
			ctx->freelist[u] = (void *)
				emalloc(ctx, (int) r*u*sizeof(union M));
			ctx->All_Mem += (unsigned long) r*u*sizeof(union M);
			m = (union M *)ctx->freelist[u] + (r-2)*u;
			for ( ; m >= (union M *)ctx->freelist[u]; m -= u)
				m->link = m+u;
		}
		log(ALLOC, u, 1);
		m = (union M *)ctx->freelist[u];
		ctx->freelist[u] = (void *)(m->link);
	}
	m->size = (u|A_USER);

	for (r = 1; r < u; )
		(&m->size)[r++] = 0;

	rp = (void *) (m+1);
	memset(rp, 0, U);
	return rp;
}

void
tfree(Ltl2baContext *ctx, void *v)
{	union M *m = (union M *) v;
	long u;

	--m;
	if ((m->size&0xFF000000) != A_USER)
		Fatal(ctx, "releasing a free block");

	u = (m->size &= 0xFFFFFF);
	if (u >= A_LARGE)
	{	log(FREE, 0, 1);
		/* free(m); */
	} else
	{	log(FREE, u, 1);
		m->link = (union M *)ctx->freelist[u];
		ctx->freelist[u] = (void *)m;
	}
}

ATrans* emalloc_atrans(Ltl2baContext *ctx) {
  ATrans *result;
  if(!ctx->atrans_list) {
    result = (ATrans *)tl_emalloc(ctx, sizeof(GTrans));
    result->pos = new_set(ctx, 1);
    result->neg = new_set(ctx, 1);
    result->to  = new_set(ctx, 0);
    ctx->apool++;
  }
  else {
    result = ctx->atrans_list;
    ctx->atrans_list = ctx->atrans_list->nxt;
    result->nxt = (ATrans *)0;
  }
  ctx->aallocs++;
  return result;
}

void free_atrans(Ltl2baContext *ctx, ATrans *t, int rec) {
  if(!t) return;
  if(rec) free_atrans(ctx, t->nxt, rec);
  t->nxt = ctx->atrans_list;
  ctx->atrans_list = t;
  ctx->afrees++;
}

void free_all_atrans(Ltl2baContext *ctx) {
  ATrans *t;
  while(ctx->atrans_list) {
    t = ctx->atrans_list;
    ctx->atrans_list = t->nxt;
    tfree(ctx, t->to);
    tfree(ctx, t->pos);
    tfree(ctx, t->neg);
    tfree(ctx, t);
  }
}

GTrans* emalloc_gtrans(Ltl2baContext *ctx) {
  GTrans *result;
  if(!ctx->gtrans_list) {
    result = (GTrans *)tl_emalloc(ctx, sizeof(GTrans));
    result->pos   = new_set(ctx, 1);
    result->neg   = new_set(ctx, 1);
    result->final = new_set(ctx, 0);
    ctx->gpool++;
  }
  else {
    result = ctx->gtrans_list;
    ctx->gtrans_list = ctx->gtrans_list->nxt;
  }
  ctx->gallocs++;
  return result;
}

void free_gtrans(Ltl2baContext *ctx, GTrans *t, GTrans *sentinel, int fly) {
  ctx->gfrees++;
  if(sentinel && (t != sentinel)) {
    free_gtrans(ctx, t->nxt, sentinel, fly);
    if(fly) t->to->incoming--;
  }
  t->nxt = ctx->gtrans_list;
  ctx->gtrans_list = t;
}

BTrans* emalloc_btrans(Ltl2baContext *ctx) {
  BTrans *result;
  if(!ctx->btrans_list) {
    result = (BTrans *)tl_emalloc(ctx, sizeof(BTrans));
    result->pos = new_set(ctx, 1);
    result->neg = new_set(ctx, 1);
    ctx->bpool++;
  }
  else {
    result = ctx->btrans_list;
    ctx->btrans_list = ctx->btrans_list->nxt;
  }
  ctx->ballocs++;
  return result;
}

void free_btrans(Ltl2baContext *ctx, BTrans *t, BTrans *sentinel, int fly) {
  ctx->bfrees++;
  if(sentinel && (t != sentinel)) {
    free_btrans(ctx, t->nxt, sentinel, fly);
    if(fly) t->to->incoming--;
  }
  t->nxt = ctx->btrans_list;
  ctx->btrans_list = t;
}

void
a_stats(Ltl2baContext *ctx)
{	long	p, a, f;
	int	i;

	printf(" size\t  pool\tallocs\t frees\n");

	for (i = 0; i < A_LARGE; i++)
	{	p = ctx->mem_event[POOL][i];
		a = ctx->mem_event[ALLOC][i];
		f = ctx->mem_event[FREE][i];

		if(p|a|f)
		printf("%5d\t%6ld\t%6ld\t%6ld\n",
			i, p, a, f);
	}

	printf("atrans\t%6d\t%6d\t%6d\n",
	       ctx->apool, ctx->aallocs, ctx->afrees);
	printf("gtrans\t%6d\t%6d\t%6d\n",
	       ctx->gpool, ctx->gallocs, ctx->gfrees);
	printf("btrans\t%6d\t%6d\t%6d\n",
	       ctx->bpool, ctx->ballocs, ctx->bfrees);
}
