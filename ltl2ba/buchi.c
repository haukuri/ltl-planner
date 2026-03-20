/***** ltl2ba : buchi.c *****/

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

#include "ltl2ba.h"

/********************************************************************\
|*        Simplification of the generalized Buchi automaton         *|
\********************************************************************/

static void free_bstate(Ltl2baContext *ctx, BState *s)
{
  free_btrans(ctx, s->trans->nxt, s->trans, 1);
  tfree(ctx, s);
}

static BState *remove_bstate(Ltl2baContext *ctx, BState *s, BState *s1)
{
  BState *prv = s->prv;
  s->prv->nxt = s->nxt;
  s->nxt->prv = s->prv;
  free_btrans(ctx, s->trans->nxt, s->trans, 0);
  s->trans = (BTrans *)0;
  s->nxt = ctx->bremoved->nxt;
  ctx->bremoved->nxt = s;
  s->prv = s1;
  for(s1 = ctx->bremoved->nxt; s1 != ctx->bremoved; s1 = s1->nxt)
    if(s1->prv == s)
      s1->prv = s->prv;
  return prv;
}

static void copy_btrans(Ltl2baContext *ctx, BTrans *from, BTrans *to) {
  to->to    = from->to;
  copy_set(ctx, from->pos, to->pos, 1);
  copy_set(ctx, from->neg, to->neg, 1);
}

static int simplify_btrans(Ltl2baContext *ctx)
{
  BState *s;
  BTrans *t, *t1;
  int changed = 0;

  if(ctx->tl_stats) getrusage(RUSAGE_SELF, &ctx->tr_debut);

  for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt)
    for (t = s->trans->nxt; t != s->trans;) {
      t1 = s->trans->nxt;
      copy_btrans(ctx, t, s->trans);
      while((t == t1) || (t->to != t1->to) ||
            !included_set(ctx, t1->pos, t->pos, 1) ||
            !included_set(ctx, t1->neg, t->neg, 1))
        t1 = t1->nxt;
      if(t1 != s->trans) {
        BTrans *free = t->nxt;
        t->to    = free->to;
        copy_set(ctx, free->pos, t->pos, 1);
        copy_set(ctx, free->neg, t->neg, 1);
        t->nxt   = free->nxt;
        if(free == s->trans) s->trans = t;
        free_btrans(ctx, free, 0, 0);
        changed++;
      }
      else
        t = t->nxt;
    }

  if(ctx->tl_stats) {
    getrusage(RUSAGE_SELF, &ctx->tr_fin);
    timeval_subtract (&ctx->t_diff, &ctx->tr_fin.ru_utime, &ctx->tr_debut.ru_utime);
    fprintf(ctx->tl_out, "\nSimplification of the Buchi automaton - transitions: %ld.%06lis",
		ctx->t_diff.tv_sec, ctx->t_diff.tv_usec);
    fprintf(ctx->tl_out, "\n%i transitions removed\n", changed);

  }
  return changed;
}

static int same_btrans(Ltl2baContext *ctx, BTrans *s, BTrans *t)
{
  return((s->to == t->to) &&
	 same_sets(ctx, s->pos, t->pos, 1) &&
	 same_sets(ctx, s->neg, t->neg, 1));
}

static void remove_btrans(Ltl2baContext *ctx, BState *to)
{
  BState *s;
  BTrans *t;
  for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt)
    for (t = s->trans->nxt; t != s->trans; t = t->nxt)
      if (t->to == to) {
	BTrans *free = t->nxt;
	t->to = free->to;
	copy_set(ctx, free->pos, t->pos, 1);
	copy_set(ctx, free->neg, t->neg, 1);
	t->nxt   = free->nxt;
	if(free == s->trans) s->trans = t;
	free_btrans(ctx, free, 0, 0);
      }
}

static void retarget_all_btrans(Ltl2baContext *ctx)
{
  BState *s;
  BTrans *t;
  for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt)
    for (t = s->trans->nxt; t != s->trans; t = t->nxt)
      if (!t->to->trans) { /* t->to has been removed */
	t->to = t->to->prv;
	if(!t->to) { /* t->to has no transitions */
	  BTrans *free = t->nxt;
	  t->to = free->to;
	  copy_set(ctx, free->pos, t->pos, 1);
	  copy_set(ctx, free->neg, t->neg, 1);
	  t->nxt   = free->nxt;
	  if(free == s->trans) s->trans = t;
	  free_btrans(ctx, free, 0, 0);
	}
      }
  while(ctx->bremoved->nxt != ctx->bremoved) { /* clean the 'removed' list */
    s = ctx->bremoved->nxt;
    ctx->bremoved->nxt = ctx->bremoved->nxt->nxt;
    tfree(ctx, s);
  }
}

static int all_btrans_match(Ltl2baContext *ctx, BState *a, BState *b)
{
  BTrans *s, *t;

  if (((a->final == ctx->accept) || (b->final == ctx->accept)) &&
      (a->final + b->final != 2 * ctx->accept)
      && a->incoming >=0
      && b->incoming >=0)
    return 0;

  for (s = a->trans->nxt; s != a->trans; s = s->nxt) {
    copy_btrans(ctx, s, b->trans);
    t = b->trans->nxt;
    while(!same_btrans(ctx, s, t))
      t = t->nxt;
    if(t == b->trans) return 0;
  }
  for (s = b->trans->nxt; s != b->trans; s = s->nxt) {
    copy_btrans(ctx, s, a->trans);
    t = a->trans->nxt;
    while(!same_btrans(ctx, s, t))
      t = t->nxt;
    if(t == a->trans) return 0;
  }
  return 1;
}

static int simplify_bstates(Ltl2baContext *ctx)
{
  BState *s, *s1, *s2;
  int changed = 0;

  if(ctx->tl_stats) getrusage(RUSAGE_SELF, &ctx->tr_debut);

  for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt) {
    if(s->trans == s->trans->nxt) { /* s has no transitions */
      s = remove_bstate(ctx, s, (BState *)0);
      changed++;
      continue;
    }
    ctx->bstates->trans = s->trans;
    ctx->bstates->final = s->final;
    s1 = s->nxt;
    while(!all_btrans_match(ctx, s, s1))
      s1 = s1->nxt;
    if(s1 != ctx->bstates) { /* s and s1 are equivalent */
      if(s1->incoming == -1) {
        s1->final = s->final;
        s1->incoming = s->incoming;
      }
      s = remove_bstate(ctx, s, s1);
      changed++;
    }
  }
  retarget_all_btrans(ctx);

  for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt) {
    for (s2 = s->nxt; s2 != ctx->bstates; s2 = s2->nxt) {
      if(s->final == s2->final && s->id == s2->id) {
        s->id = ++ctx->gstate_id;
      }
    }
  }

  if(ctx->tl_stats) {
    getrusage(RUSAGE_SELF, &ctx->tr_fin);
    timeval_subtract (&ctx->t_diff, &ctx->tr_fin.ru_utime, &ctx->tr_debut.ru_utime);
    fprintf(ctx->tl_out, "\nSimplification of the Buchi automaton - states: %ld.%06lis",
		ctx->t_diff.tv_sec, ctx->t_diff.tv_usec);
    fprintf(ctx->tl_out, "\n%i states removed\n", changed);
  }

  return changed;
}

static int bdfs(Ltl2baContext *ctx, BState *s) {
  BTrans *t;
  BScc *c;
  BScc *scc = (BScc *)tl_emalloc(ctx, sizeof(BScc));
  scc->bstate = s;
  scc->rank = ctx->brank;
  scc->theta = ctx->brank++;
  scc->nxt = ctx->bscc_stack;
  ctx->bscc_stack = scc;

  s->incoming = 1;

  for (t = s->trans->nxt; t != s->trans; t = t->nxt) {
    if (t->to->incoming == 0) {
      int result = bdfs(ctx, t->to);
      scc->theta = min(scc->theta, result);
    }
    else {
      for(c = ctx->bscc_stack->nxt; c != 0; c = c->nxt)
	if(c->bstate == t->to) {
	  scc->theta = min(scc->theta, c->rank);
	  break;
	}
    }
  }
  if(scc->rank == scc->theta) {
    if(ctx->bscc_stack == scc) { /* s is alone in a scc */
      s->incoming = -1;
      for (t = s->trans->nxt; t != s->trans; t = t->nxt)
	if (t->to == s)
	  s->incoming = 1;
    }
    ctx->bscc_stack = scc->nxt;
  }
  return scc->theta;
}

static void simplify_bscc(Ltl2baContext *ctx) {
  BState *s;
  ctx->brank = 1;
  ctx->bscc_stack = 0;

  if(ctx->bstates == ctx->bstates->nxt) return;

  for(s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt)
    s->incoming = 0;

  bdfs(ctx, ctx->bstates->prv);

  for(s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt)
    if(s->incoming == 0)
      remove_bstate(ctx, s, 0);
}




/********************************************************************\
|*              Generation of the Buchi automaton                   *|
\********************************************************************/

static BState *find_bstate(Ltl2baContext *ctx, GState **state, int final, BState *s)
{
  if((s->gstate == *state) && (s->final == final)) return s;

  s = ctx->bstack->nxt;
  ctx->bstack->gstate = *state;
  ctx->bstack->final = final;
  while(!(s->gstate == *state) || !(s->final == final))
    s = s->nxt;
  if(s != ctx->bstack) return s;

  s = ctx->bstates->nxt;
  ctx->bstates->gstate = *state;
  ctx->bstates->final = final;
  while(!(s->gstate == *state) || !(s->final == final))
    s = s->nxt;
  if(s != ctx->bstates) return s;

  s = ctx->bremoved->nxt;
  ctx->bremoved->gstate = *state;
  ctx->bremoved->final = final;
  while(!(s->gstate == *state) || !(s->final == final))
    s = s->nxt;
  if(s != ctx->bremoved) return s;

  s = (BState *)tl_emalloc(ctx, sizeof(BState));
  s->gstate = *state;
  s->id = (*state)->id;
  s->incoming = 0;
  s->final = final;
  s->trans = emalloc_btrans(ctx); /* sentinel */
  s->trans->nxt = s->trans;
  s->nxt = ctx->bstack->nxt;
  ctx->bstack->nxt = s;
  return s;
}

static int next_final(Ltl2baContext *ctx, int *set, int fin)
{
  if((fin != ctx->accept) && in_set(set, ctx->final_list[fin + 1]))
    return next_final(ctx, set, fin + 1);
  return fin;
}

static void make_btrans(Ltl2baContext *ctx, BState *s)
{
  int state_trans = 0;
  GTrans *t;
  BTrans *t1;
  BState *s1;
  if(s->gstate->trans)
    for(t = s->gstate->trans->nxt; t != s->gstate->trans; t = t->nxt) {
      int fin = next_final(ctx, t->final, (s->final == ctx->accept) ? 0 : s->final);
      BState *to = find_bstate(ctx, &t->to, fin, s);

      for(t1 = s->trans->nxt; t1 != s->trans;) {
	if(ctx->tl_simp_fly &&
	   (to == t1->to) &&
	   included_set(ctx, t->pos, t1->pos, 1) &&
	   included_set(ctx, t->neg, t1->neg, 1)) { /* t1 is redondant */
	  BTrans *free = t1->nxt;
	  t1->to->incoming--;
	  t1->to = free->to;
	  copy_set(ctx, free->pos, t1->pos, 1);
	  copy_set(ctx, free->neg, t1->neg, 1);
	  t1->nxt   = free->nxt;
	  if(free == s->trans) s->trans = t1;
	  free_btrans(ctx, free, 0, 0);
	  state_trans--;
	}
	else if(ctx->tl_simp_fly &&
		(t1->to == to ) &&
		included_set(ctx, t1->pos, t->pos, 1) &&
		included_set(ctx, t1->neg, t->neg, 1)) /* t is redondant */
	  break;
	else
	  t1 = t1->nxt;
      }
      if(t1 == s->trans) {
	BTrans *trans = emalloc_btrans(ctx);
	trans->to = to;
	trans->to->incoming++;
	copy_set(ctx, t->pos, trans->pos, 1);
	copy_set(ctx, t->neg, trans->neg, 1);
	trans->nxt = s->trans->nxt;
	s->trans->nxt = trans;
	state_trans++;
      }
    }

  if(ctx->tl_simp_fly) {
    if(s->trans == s->trans->nxt) { /* s has no transitions */
      free_btrans(ctx, s->trans->nxt, s->trans, 1);
      s->trans = (BTrans *)0;
      s->prv = (BState *)0;
      s->nxt = ctx->bremoved->nxt;
      ctx->bremoved->nxt = s;
      for(s1 = ctx->bremoved->nxt; s1 != ctx->bremoved; s1 = s1->nxt)
	if(s1->prv == s)
	  s1->prv = (BState *)0;
      return;
    }
    ctx->bstates->trans = s->trans;
    ctx->bstates->final = s->final;
    s1 = ctx->bstates->nxt;
    while(!all_btrans_match(ctx, s, s1))
      s1 = s1->nxt;
    if(s1 != ctx->bstates) { /* s and s1 are equivalent */
      free_btrans(ctx, s->trans->nxt, s->trans, 1);
      s->trans = (BTrans *)0;
      s->prv = s1;
      s->nxt = ctx->bremoved->nxt;
      ctx->bremoved->nxt = s;
      for(s1 = ctx->bremoved->nxt; s1 != ctx->bremoved; s1 = s1->nxt)
	if(s1->prv == s)
	  s1->prv = s->prv;
      return;
    }
  }
  s->nxt = ctx->bstates->nxt;
  s->prv = ctx->bstates;
  s->nxt->prv = s;
  ctx->bstates->nxt = s;
  ctx->btrans_count += state_trans;
  ctx->bstate_count++;
}

/********************************************************************\
|*                  Display of the Buchi automaton                  *|
\********************************************************************/

static void print_buchi(Ltl2baContext *ctx, BState *s)
{
  BTrans *t;
  if(s == ctx->bstates) return;

  print_buchi(ctx, s->nxt);

  fprintf(ctx->tl_out, "state ");
  if(s->id == -1)
    fprintf(ctx->tl_out, "init");
  else {
    if(s->final == ctx->accept)
      fprintf(ctx->tl_out, "accept");
    else
      fprintf(ctx->tl_out, "T%i", s->final);
    fprintf(ctx->tl_out, "_%i", s->id);
  }
  fprintf(ctx->tl_out, "\n");
  for(t = s->trans->nxt; t != s->trans; t = t->nxt) {
    if (empty_set(ctx, t->pos, 1) && empty_set(ctx, t->neg, 1))
      fprintf(ctx->tl_out, "1");
    print_set(ctx, t->pos, 1);
    if (!empty_set(ctx, t->pos, 1) && !empty_set(ctx, t->neg, 1)) fprintf(ctx->tl_out, " & ");
    print_set(ctx, t->neg, 2);
    fprintf(ctx->tl_out, " -> ");
    if(t->to->id == -1)
      fprintf(ctx->tl_out, "init\n");
    else {
      if(t->to->final == ctx->accept)
	fprintf(ctx->tl_out, "accept");
      else
	fprintf(ctx->tl_out, "T%i", t->to->final);
      fprintf(ctx->tl_out, "_%i\n", t->to->id);
    }
  }
}

static void print_spin_buchi(Ltl2baContext *ctx) {
  BTrans *t;
  BState *s;
  int accept_all = 0;
  if(ctx->bstates->nxt == ctx->bstates) { /* empty automaton */
    fprintf(ctx->tl_out, "never {    /* ");
    put_uform(ctx);
    fprintf(ctx->tl_out, " */\n");
    fprintf(ctx->tl_out, "T0_init:\n");
    fprintf(ctx->tl_out, "\tfalse;\n");
    fprintf(ctx->tl_out, "}\n");
    return;
  }
  if(ctx->bstates->nxt->nxt == ctx->bstates && ctx->bstates->nxt->id == 0) { /* true */
    fprintf(ctx->tl_out, "never {    /* ");
    put_uform(ctx);
    fprintf(ctx->tl_out, " */\n");
    fprintf(ctx->tl_out, "accept_init:\n");
    fprintf(ctx->tl_out, "\tif\n");
    fprintf(ctx->tl_out, "\t:: (1) -> goto accept_init\n");
    fprintf(ctx->tl_out, "\tfi;\n");
    fprintf(ctx->tl_out, "}\n");
    return;
  }

  fprintf(ctx->tl_out, "never { /* ");
  put_uform(ctx);
  fprintf(ctx->tl_out, " */\n");
  for(s = ctx->bstates->prv; s != ctx->bstates; s = s->prv) {
    if(s->id == 0) { /* accept_all at the end */
      accept_all = 1;
      continue;
    }
    if(s->final == ctx->accept)
      fprintf(ctx->tl_out, "accept_");
    else fprintf(ctx->tl_out, "T%i_", s->final);
    if(s->id == -1)
      fprintf(ctx->tl_out, "init:\n");
    else fprintf(ctx->tl_out, "S%i:\n", s->id);
    if(s->trans->nxt == s->trans) {
      fprintf(ctx->tl_out, "\tfalse;\n");
      continue;
    }
    fprintf(ctx->tl_out, "\tif\n");
    for(t = s->trans->nxt; t != s->trans; t = t->nxt) {
      BTrans *t1;
      fprintf(ctx->tl_out, "\t:: (");
      spin_print_set(ctx, t->pos, t->neg);
      for(t1 = t; t1->nxt != s->trans; )
	if (t1->nxt->to->id == t->to->id &&
	    t1->nxt->to->final == t->to->final) {
	  fprintf(ctx->tl_out, ") || (");
	  spin_print_set(ctx, t1->nxt->pos, t1->nxt->neg);
	  t1->nxt = t1->nxt->nxt;
	}
	else  t1 = t1->nxt;
      fprintf(ctx->tl_out, ") -> goto ");
      if(t->to->final == ctx->accept)
	fprintf(ctx->tl_out, "accept_");
      else fprintf(ctx->tl_out, "T%i_", t->to->final);
      if(t->to->id == 0)
	fprintf(ctx->tl_out, "all\n");
      else if(t->to->id == -1)
	fprintf(ctx->tl_out, "init\n");
      else fprintf(ctx->tl_out, "S%i\n", t->to->id);
    }
    fprintf(ctx->tl_out, "\tfi;\n");
  }
  if(accept_all) {
    fprintf(ctx->tl_out, "accept_all:\n");
    fprintf(ctx->tl_out, "\tskip\n");
  }
  fprintf(ctx->tl_out, "}\n");
}

/********************************************************************\
|*                       Main method                                *|
\********************************************************************/

void mk_buchi(Ltl2baContext *ctx)
{
  int i;
  BState *s = (BState *)tl_emalloc(ctx, sizeof(BState));
  GTrans *t;
  BTrans *t1;
  ctx->accept = ctx->final_list[0] - 1;

  if(ctx->tl_stats) getrusage(RUSAGE_SELF, &ctx->tr_debut);

  ctx->bstack        = (BState *)tl_emalloc(ctx, sizeof(BState)); /* sentinel */
  ctx->bstack->nxt   = ctx->bstack;
  ctx->bremoved      = (BState *)tl_emalloc(ctx, sizeof(BState)); /* sentinel */
  ctx->bremoved->nxt = ctx->bremoved;
  ctx->bstates       = (BState *)tl_emalloc(ctx, sizeof(BState)); /* sentinel */
  ctx->bstates->nxt  = s;
  ctx->bstates->prv  = s;

  s->nxt        = ctx->bstates; /* creates (unique) inital state */
  s->prv        = ctx->bstates;
  s->id = -1;
  s->incoming = 1;
  s->final = 0;
  s->gstate = 0;
  s->trans = emalloc_btrans(ctx); /* sentinel */
  s->trans->nxt = s->trans;
  for(i = 0; i < ctx->init_size; i++)
    if(ctx->init[i])
      for(t = ctx->init[i]->trans->nxt; t != ctx->init[i]->trans; t = t->nxt) {
	int fin = next_final(ctx, t->final, 0);
	BState *to = find_bstate(ctx, &t->to, fin, s);
	for(t1 = s->trans->nxt; t1 != s->trans;) {
	  if(ctx->tl_simp_fly &&
	     (to == t1->to) &&
	     included_set(ctx, t->pos, t1->pos, 1) &&
	     included_set(ctx, t->neg, t1->neg, 1)) { /* t1 is redondant */
	    BTrans *free = t1->nxt;
	    t1->to->incoming--;
	    t1->to = free->to;
	    copy_set(ctx, free->pos, t1->pos, 1);
	    copy_set(ctx, free->neg, t1->neg, 1);
	    t1->nxt   = free->nxt;
	    if(free == s->trans) s->trans = t1;
	    free_btrans(ctx, free, 0, 0);
	  }
	else if(ctx->tl_simp_fly &&
		(t1->to == to ) &&
		included_set(ctx, t1->pos, t->pos, 1) &&
		included_set(ctx, t1->neg, t->neg, 1)) /* t is redondant */
	  break;
	  else
	    t1 = t1->nxt;
	}
	if(t1 == s->trans) {
	  BTrans *trans = emalloc_btrans(ctx);
	  trans->to = to;
	  trans->to->incoming++;
	  copy_set(ctx, t->pos, trans->pos, 1);
	  copy_set(ctx, t->neg, trans->neg, 1);
	  trans->nxt = s->trans->nxt;
	  s->trans->nxt = trans;
	}
      }

  while(ctx->bstack->nxt != ctx->bstack) {
    s = ctx->bstack->nxt;
    ctx->bstack->nxt = ctx->bstack->nxt->nxt;
    if(!s->incoming) {
      free_bstate(ctx, s);
      continue;
    }
    make_btrans(ctx, s);
  }

  retarget_all_btrans(ctx);

  if(ctx->tl_stats) {
    getrusage(RUSAGE_SELF, &ctx->tr_fin);
    timeval_subtract (&ctx->t_diff, &ctx->tr_fin.ru_utime, &ctx->tr_debut.ru_utime);
    fprintf(ctx->tl_out, "\nBuilding the Buchi automaton : %ld.%06lis",
		ctx->t_diff.tv_sec, ctx->t_diff.tv_usec);
    fprintf(ctx->tl_out, "\n%i states, %i transitions\n", ctx->bstate_count, ctx->btrans_count);
  }

  if(ctx->tl_verbose) {
    fprintf(ctx->tl_out, "\nBuchi automaton before simplification\n");
    print_buchi(ctx, ctx->bstates->nxt);
    if(ctx->bstates == ctx->bstates->nxt)
      fprintf(ctx->tl_out, "empty automaton, refuses all words\n");
  }

  if(ctx->tl_simp_diff) {
    simplify_btrans(ctx);
    if(ctx->tl_simp_scc) simplify_bscc(ctx);
    while(simplify_bstates(ctx)) {
      simplify_btrans(ctx);
      if(ctx->tl_simp_scc) simplify_bscc(ctx);
    }

    if(ctx->tl_verbose) {
      fprintf(ctx->tl_out, "\nBuchi automaton after simplification\n");
      print_buchi(ctx, ctx->bstates->nxt);
      if(ctx->bstates == ctx->bstates->nxt)
	fprintf(ctx->tl_out, "empty automaton, refuses all words\n");
      fprintf(ctx->tl_out, "\n");
    }
  }

  print_spin_buchi(ctx);
}
