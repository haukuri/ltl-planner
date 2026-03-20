/***** ltl2ba : generalized.c *****/

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

static void print_generalized(Ltl2baContext *ctx);

/********************************************************************\
|*        Simplification of the generalized Buchi automaton         *|
\********************************************************************/

static void free_gstate(Ltl2baContext *ctx, GState *s)
{
  free_gtrans(ctx, s->trans->nxt, s->trans, 1);
  tfree(ctx, s->nodes_set);
  tfree(ctx, s);
}

static GState *remove_gstate(Ltl2baContext *ctx, GState *s, GState *s1)
{
  GState *prv = s->prv;
  s->prv->nxt = s->nxt;
  s->nxt->prv = s->prv;
  free_gtrans(ctx, s->trans->nxt, s->trans, 0);
  s->trans = (GTrans *)0;
  tfree(ctx, s->nodes_set);
  s->nodes_set = 0;
  s->nxt = ctx->gremoved->nxt;
  ctx->gremoved->nxt = s;
  s->prv = s1;
  for(s1 = ctx->gremoved->nxt; s1 != ctx->gremoved; s1 = s1->nxt)
    if(s1->prv == s)
      s1->prv = s->prv;
  return prv;
}

static void copy_gtrans(Ltl2baContext *ctx, GTrans *from, GTrans *to)
{
  to->to = from->to;
  copy_set(ctx, from->pos,   to->pos,   1);
  copy_set(ctx, from->neg,   to->neg,   1);
  copy_set(ctx, from->final, to->final, 0);
}

static int same_gtrans(Ltl2baContext *ctx, GState *a, GTrans *s, GState *b, GTrans *t, int use_scc)
{
  if((s->to != t->to) ||
     ! same_sets(ctx, s->pos, t->pos, 1) ||
     ! same_sets(ctx, s->neg, t->neg, 1))
    return 0;
  if(same_sets(ctx, s->final, t->final, 0))
    return 1;
  if( use_scc &&
      ( in_set(ctx->bad_scc, a->incoming) ||
        in_set(ctx->bad_scc, b->incoming) ||
        (a->incoming != s->to->incoming) ||
        (b->incoming != t->to->incoming) ) )
    return 1;
  return 0;
  /* dead code from original kept for reference */
  if(!use_scc)
    return 0;
  if( (a->incoming == b->incoming) && (a->incoming == s->to->incoming) )
    return 0;
  return 1;
}

static int simplify_gtrans(Ltl2baContext *ctx)
{
  int changed = 0;
  GState *s;
  GTrans *t, *t1;

  if(ctx->tl_stats) getrusage(RUSAGE_SELF, &ctx->tr_debut);

  for(s = ctx->gstates->nxt; s != ctx->gstates; s = s->nxt) {
    t = s->trans->nxt;
    while(t != s->trans) {
      copy_gtrans(ctx, t, s->trans);
      t1 = s->trans->nxt;
      while ( !((t != t1)
          && (t1->to == t->to)
          && included_set(ctx, t1->pos, t->pos, 1)
          && included_set(ctx, t1->neg, t->neg, 1)
          && (included_set(ctx, t->final, t1->final, 0)
              || (ctx->tl_simp_scc && ((s->incoming != t->to->incoming) || in_set(ctx->bad_scc, s->incoming))))) )
        t1 = t1->nxt;
      if(t1 != s->trans) {
        GTrans *f = t->nxt;
        t->to = f->to;
        copy_set(ctx, f->pos, t->pos, 1);
        copy_set(ctx, f->neg, t->neg, 1);
        copy_set(ctx, f->final, t->final, 0);
        t->nxt = f->nxt;
        if(f == s->trans) s->trans = t;
        free_gtrans(ctx, f, 0, 0);
        changed++;
      }
      else
        t = t->nxt;
    }
  }

  if(ctx->tl_stats) {
    getrusage(RUSAGE_SELF, &ctx->tr_fin);
    timeval_subtract (&ctx->t_diff, &ctx->tr_fin.ru_utime, &ctx->tr_debut.ru_utime);
    fprintf(ctx->tl_out, "\nSimplification of the generalized Buchi automaton - transitions: %ld.%06lis",
		ctx->t_diff.tv_sec, ctx->t_diff.tv_usec);
    fprintf(ctx->tl_out, "\n%i transitions removed\n", changed);
  }

  return changed;
}

static void retarget_all_gtrans(Ltl2baContext *ctx)
{
  GState *s;
  GTrans *t;
  int i;
  for (i = 0; i < ctx->init_size; i++)
    if (ctx->init[i] && !ctx->init[i]->trans)
      ctx->init[i] = ctx->init[i]->prv;
  for (s = ctx->gstates->nxt; s != ctx->gstates; s = s->nxt)
    for (t = s->trans->nxt; t != s->trans; )
      if (!t->to->trans) {
	t->to = t->to->prv;
	if(!t->to) {
	  GTrans *f = t->nxt;
	  t->to = f->to;
	  copy_set(ctx, f->pos, t->pos, 1);
	  copy_set(ctx, f->neg, t->neg, 1);
	  copy_set(ctx, f->final, t->final, 0);
	  t->nxt   = f->nxt;
	  if(f == s->trans) s->trans = t;
	  free_gtrans(ctx, f, 0, 0);
	}
	else
	  t = t->nxt;
      }
      else
	t = t->nxt;
  while(ctx->gremoved->nxt != ctx->gremoved) {
    s = ctx->gremoved->nxt;
    ctx->gremoved->nxt = ctx->gremoved->nxt->nxt;
    if(s->nodes_set) tfree(ctx, s->nodes_set);
    tfree(ctx, s);
  }
}

static int all_gtrans_match(Ltl2baContext *ctx, GState *a, GState *b, int use_scc)
{
  GTrans *s, *t;
  for (s = a->trans->nxt; s != a->trans; s = s->nxt) {
    copy_gtrans(ctx, s, b->trans);
    t = b->trans->nxt;
    while(!same_gtrans(ctx, a, s, b, t, use_scc)) t = t->nxt;
    if(t == b->trans) return 0;
  }
  for (t = b->trans->nxt; t != b->trans; t = t->nxt) {
    copy_gtrans(ctx, t, a->trans);
    s = a->trans->nxt;
    while(!same_gtrans(ctx, a, s, b, t, use_scc)) s = s->nxt;
    if(s == a->trans) return 0;
  }
  return 1;
}

static int simplify_gstates(Ltl2baContext *ctx)
{
  int changed = 0;
  GState *a, *b;

  if(ctx->tl_stats) getrusage(RUSAGE_SELF, &ctx->tr_debut);

  for(a = ctx->gstates->nxt; a != ctx->gstates; a = a->nxt) {
    if(a->trans == a->trans->nxt) {
      a = remove_gstate(ctx, a, (GState *)0);
      changed++;
      continue;
    }
    ctx->gstates->trans = a->trans;
    b = a->nxt;
    while(!all_gtrans_match(ctx, a, b, ctx->tl_simp_scc)) b = b->nxt;
    if(b != ctx->gstates) {
      if(a->incoming > b->incoming)
        a = remove_gstate(ctx, a, b);
      else
        remove_gstate(ctx, b, a);
      changed++;
    }
  }
  retarget_all_gtrans(ctx);

  if(ctx->tl_stats) {
    getrusage(RUSAGE_SELF, &ctx->tr_fin);
    timeval_subtract (&ctx->t_diff, &ctx->tr_fin.ru_utime, &ctx->tr_debut.ru_utime);
    fprintf(ctx->tl_out, "\nSimplification of the generalized Buchi automaton - states: %ld.%06lis",
		ctx->t_diff.tv_sec, ctx->t_diff.tv_usec);
    fprintf(ctx->tl_out, "\n%i states removed\n", changed);
  }

  return changed;
}

static int gdfs(Ltl2baContext *ctx, GState *s) {
  GTrans *t;
  GScc *c;
  GScc *scc = (GScc *)tl_emalloc(ctx, sizeof(GScc));
  scc->gstate = s;
  scc->rank = ctx->grank;
  scc->theta = ctx->grank++;
  scc->nxt = ctx->gscc_stack;
  ctx->gscc_stack = scc;

  s->incoming = 1;

  for (t = s->trans->nxt; t != s->trans; t = t->nxt) {
    if (t->to->incoming == 0) {
      int result = gdfs(ctx, t->to);
      scc->theta = min(scc->theta, result);
    }
    else {
      for(c = ctx->gscc_stack->nxt; c != 0; c = c->nxt)
	if(c->gstate == t->to) {
	  scc->theta = min(scc->theta, c->rank);
	  break;
	}
    }
  }
  if(scc->rank == scc->theta) {
    while(ctx->gscc_stack != scc) {
      ctx->gscc_stack->gstate->incoming = ctx->scc_id;
      ctx->gscc_stack = ctx->gscc_stack->nxt;
    }
    scc->gstate->incoming = ctx->scc_id++;
    ctx->gscc_stack = scc->nxt;
  }
  return scc->theta;
}

static void simplify_gscc(Ltl2baContext *ctx) {
  GState *s;
  GTrans *t;
  int i, **scc_final;
  ctx->grank = 1;
  ctx->gscc_stack = 0;
  ctx->scc_id = 1;

  if(ctx->gstates == ctx->gstates->nxt) return;

  for(s = ctx->gstates->nxt; s != ctx->gstates; s = s->nxt)
    s->incoming = 0;

  for(i = 0; i < ctx->init_size; i++)
    if(ctx->init[i] && ctx->init[i]->incoming == 0)
      gdfs(ctx, ctx->init[i]);

  scc_final = (int **)tl_emalloc(ctx, ctx->scc_id * sizeof(int *));
  for(i = 0; i < ctx->scc_id; i++)
    scc_final[i] = make_set(ctx, -1,0);

  for(s = ctx->gstates->nxt; s != ctx->gstates; s = s->nxt)
    if(s->incoming == 0)
      s = remove_gstate(ctx, s, 0);
    else
      for (t = s->trans->nxt; t != s->trans; t = t->nxt)
        if(t->to->incoming == s->incoming)
          merge_sets(ctx, scc_final[s->incoming], t->final, 0);

  ctx->scc_size = (ctx->scc_id + 1) / (8 * sizeof(int)) + 1;
  ctx->bad_scc=make_set(ctx, -1,2);

  for(i = 0; i < ctx->scc_id; i++)
    if(!included_set(ctx, ctx->final_set, scc_final[i], 0))
       add_set(ctx->bad_scc, i);

  for(i = 0; i < ctx->scc_id; i++)
    tfree(ctx, scc_final[i]);
  tfree(ctx, scc_final);
}

/********************************************************************\
|*        Generation of the generalized Buchi automaton             *|
\********************************************************************/

static int is_final(Ltl2baContext *ctx, int *from, ATrans *at, int i)
{
  ATrans *t;
  int in_to;
  if((ctx->tl_fjtofj && !in_set(at->to, i)) ||
    (!ctx->tl_fjtofj && !in_set(from,  i))) return 1;
  in_to = in_set(at->to, i);
  rem_set(at->to, i);
  for(t = ctx->transition[i]; t; t = t->nxt)
    if(included_set(ctx, t->to, at->to, 0) &&
       included_set(ctx, t->pos, at->pos, 1) &&
       included_set(ctx, t->neg, at->neg, 1)) {
      if(in_to) add_set(at->to, i);
      return 1;
    }
  if(in_to) add_set(at->to, i);
  return 0;
}

static GState *find_gstate(Ltl2baContext *ctx, int *set, GState *s)
{
  if(same_sets(ctx, set, s->nodes_set, 0)) return s;

  s = ctx->gstack->nxt;
  ctx->gstack->nodes_set = set;
  while(!same_sets(ctx, set, s->nodes_set, 0))
    s = s->nxt;
  if(s != ctx->gstack) return s;

  s = ctx->gstates->nxt;
  ctx->gstates->nodes_set = set;
  while(!same_sets(ctx, set, s->nodes_set, 0))
    s = s->nxt;
  if(s != ctx->gstates) return s;

  s = ctx->gremoved->nxt;
  ctx->gremoved->nodes_set = set;
  while(!same_sets(ctx, set, s->nodes_set, 0))
    s = s->nxt;
  if(s != ctx->gremoved) return s;

  s = (GState *)tl_emalloc(ctx, sizeof(GState));
  s->id = (empty_set(ctx, set, 0)) ? 0 : ctx->gstate_id++;
  s->incoming = 0;
  s->nodes_set = dup_set(ctx, set, 0);
  s->trans = emalloc_gtrans(ctx);
  s->trans->nxt = s->trans;
  s->nxt = ctx->gstack->nxt;
  ctx->gstack->nxt = s;
  return s;
}

static void make_gtrans(Ltl2baContext *ctx, GState *s) {
  int i, *list, state_trans = 0, trans_exist = 1;
  GState *s1;
  ATrans *t1;
  AProd *prod = (AProd *)tl_emalloc(ctx, sizeof(AProd));
  prod->nxt = prod;
  prod->prv = prod;
  prod->prod = emalloc_atrans(ctx);
  clear_set(ctx, prod->prod->to,  0);
  clear_set(ctx, prod->prod->pos, 1);
  clear_set(ctx, prod->prod->neg, 1);
  prod->trans = prod->prod;
  prod->trans->nxt = prod->prod;
  list = list_set(ctx, s->nodes_set, 0);

  for(i = 1; i < list[0]; i++) {
    AProd *p = (AProd *)tl_emalloc(ctx, sizeof(AProd));
    p->astate = list[i];
    p->trans = ctx->transition[list[i]];
    if(!p->trans) trans_exist = 0;
    p->prod = merge_trans(ctx, prod->nxt->prod, p->trans);
    p->nxt = prod->nxt;
    p->prv = prod;
    p->nxt->prv = p;
    p->prv->nxt = p;
  }

  while(trans_exist) {
    AProd *p = prod->nxt;
    t1 = p->prod;
    if(t1) {
      GTrans *trans, *t2;
      clear_set(ctx, ctx->fin, 0);
      for(i = 1; i < ctx->final_list[0]; i++)
	if(is_final(ctx, s->nodes_set, t1, ctx->final_list[i]))
	  add_set(ctx->fin, ctx->final_list[i]);
      for(t2 = s->trans->nxt; t2 != s->trans;) {
	if(ctx->tl_simp_fly &&
	   included_set(ctx, t1->to, t2->to->nodes_set, 0) &&
	   included_set(ctx, t1->pos, t2->pos, 1) &&
	   included_set(ctx, t1->neg, t2->neg, 1) &&
	   same_sets(ctx, ctx->fin, t2->final, 0)) {
	  GTrans *f = t2->nxt;
	  t2->to->incoming--;
	  t2->to = f->to;
	  copy_set(ctx, f->pos, t2->pos, 1);
	  copy_set(ctx, f->neg, t2->neg, 1);
	  copy_set(ctx, f->final, t2->final, 0);
	  t2->nxt   = f->nxt;
	  if(f == s->trans) s->trans = t2;
	  free_gtrans(ctx, f, 0, 0);
	  state_trans--;
	}
	else if(ctx->tl_simp_fly &&
		included_set(ctx, t2->to->nodes_set, t1->to, 0) &&
		included_set(ctx, t2->pos, t1->pos, 1) &&
		included_set(ctx, t2->neg, t1->neg, 1) &&
		same_sets(ctx, t2->final, ctx->fin, 0)) {
	  break;
	}
	else {
	  t2 = t2->nxt;
	}
      }
      if(t2 == s->trans) {
	trans = emalloc_gtrans(ctx);
	trans->to = find_gstate(ctx, t1->to, s);
	trans->to->incoming++;
	copy_set(ctx, t1->pos, trans->pos, 1);
	copy_set(ctx, t1->neg, trans->neg, 1);
	copy_set(ctx, ctx->fin,   trans->final, 0);
	trans->nxt = s->trans->nxt;
	s->trans->nxt = trans;
	state_trans++;
      }
    }
    if(!p->trans)
      break;
    while(!p->trans->nxt)
      p = p->nxt;
    if(p == prod)
      break;
    p->trans = p->trans->nxt;
    do_merge_trans(ctx, &(p->prod), p->nxt->prod, p->trans);
    p = p->prv;
    while(p != prod) {
      p->trans = ctx->transition[p->astate];
      do_merge_trans(ctx, &(p->prod), p->nxt->prod, p->trans);
      p = p->prv;
    }
  }

  tfree(ctx, list);
  while(prod->nxt != prod) {
    AProd *p = prod->nxt;
    prod->nxt = p->nxt;
    free_atrans(ctx, p->prod, 0);
    tfree(ctx, p);
  }
  free_atrans(ctx, prod->prod, 0);
  tfree(ctx, prod);

  if(ctx->tl_simp_fly) {
    if(s->trans == s->trans->nxt) {
      free_gtrans(ctx, s->trans->nxt, s->trans, 1);
      s->trans = (GTrans *)0;
      s->prv = (GState *)0;
      s->nxt = ctx->gremoved->nxt;
      ctx->gremoved->nxt = s;
      for(s1 = ctx->gremoved->nxt; s1 != ctx->gremoved; s1 = s1->nxt)
	if(s1->prv == s)
	s1->prv = (GState *)0;
      return;
    }

    ctx->gstates->trans = s->trans;
    s1 = ctx->gstates->nxt;
    while(!all_gtrans_match(ctx, s, s1, 0))
      s1 = s1->nxt;
    if(s1 != ctx->gstates) {
      free_gtrans(ctx, s->trans->nxt, s->trans, 1);
      s->trans = (GTrans *)0;
      s->prv = s1;
      s->nxt = ctx->gremoved->nxt;
      ctx->gremoved->nxt = s;
      for(s1 = ctx->gremoved->nxt; s1 != ctx->gremoved; s1 = s1->nxt)
	if(s1->prv == s)
	  s1->prv = s->prv;
      return;
    }
  }

  s->nxt = ctx->gstates->nxt;
  s->prv = ctx->gstates;
  s->nxt->prv = s;
  ctx->gstates->nxt = s;
  ctx->gtrans_count += state_trans;
  ctx->gstate_count++;
}

/********************************************************************\
|*            Display of the generalized Buchi automaton            *|
\********************************************************************/

static void reverse_print_generalized(Ltl2baContext *ctx, GState *s)
{
  GTrans *t;
  if(s == ctx->gstates) return;

  reverse_print_generalized(ctx, s->nxt);

  fprintf(ctx->tl_out, "state %i (", s->id);
  print_set(ctx, s->nodes_set, 0);
  fprintf(ctx->tl_out, ") : %i\n", s->incoming);
  for(t = s->trans->nxt; t != s->trans; t = t->nxt) {
    if (empty_set(ctx, t->pos, 1) && empty_set(ctx, t->neg, 1))
      fprintf(ctx->tl_out, "1");
    print_set(ctx, t->pos, 1);
    if (!empty_set(ctx, t->pos, 1) && !empty_set(ctx, t->neg, 1)) fprintf(ctx->tl_out, " & ");
    print_set(ctx, t->neg, 1);
    fprintf(ctx->tl_out, " -> %i : ", t->to->id);
    print_set(ctx, t->final, 0);
    fprintf(ctx->tl_out, "\n");
  }
}

static void print_generalized(Ltl2baContext *ctx) {
  int i;
  fprintf(ctx->tl_out, "init :\n");
  for(i = 0; i < ctx->init_size; i++)
    if(ctx->init[i])
      fprintf(ctx->tl_out, "%i\n", ctx->init[i]->id);
  reverse_print_generalized(ctx, ctx->gstates->nxt);
}

/********************************************************************\
|*                       Main method                                *|
\********************************************************************/

void mk_generalized(Ltl2baContext *ctx)
{
  ATrans *t;
  GState *s;

  if(ctx->tl_stats) getrusage(RUSAGE_SELF, &ctx->tr_debut);

  ctx->fin = new_set(ctx, 0);
  ctx->bad_scc = 0;
  ctx->final_list = list_set(ctx, ctx->final_set, 0);

  ctx->gstack        = (GState *)tl_emalloc(ctx, sizeof(GState));
  ctx->gstack->nxt   = ctx->gstack;
  ctx->gremoved      = (GState *)tl_emalloc(ctx, sizeof(GState));
  ctx->gremoved->nxt = ctx->gremoved;
  ctx->gstates       = (GState *)tl_emalloc(ctx, sizeof(GState));
  ctx->gstates->nxt  = ctx->gstates;
  ctx->gstates->prv  = ctx->gstates;

  for(t = ctx->transition[0]; t; t = t->nxt) {
    s = (GState *)tl_emalloc(ctx, sizeof(GState));
    s->id = (empty_set(ctx, t->to, 0)) ? 0 : ctx->gstate_id++;
    s->incoming = 1;
    s->nodes_set = dup_set(ctx, t->to, 0);
    s->trans = emalloc_gtrans(ctx);
    s->trans->nxt = s->trans;
    s->nxt = ctx->gstack->nxt;
    ctx->gstack->nxt = s;
    ctx->init_size++;
  }

  if(ctx->init_size) ctx->init = (GState **)tl_emalloc(ctx, ctx->init_size * sizeof(GState *));
  ctx->init_size = 0;
  for(s = ctx->gstack->nxt; s != ctx->gstack; s = s->nxt)
    ctx->init[ctx->init_size++] = s;

  while(ctx->gstack->nxt != ctx->gstack) {
    s = ctx->gstack->nxt;
    ctx->gstack->nxt = ctx->gstack->nxt->nxt;
    if(!s->incoming) {
      free_gstate(ctx, s);
      continue;
    }
    make_gtrans(ctx, s);
  }

  retarget_all_gtrans(ctx);

  if(ctx->tl_stats) {
    getrusage(RUSAGE_SELF, &ctx->tr_fin);
    timeval_subtract (&ctx->t_diff, &ctx->tr_fin.ru_utime, &ctx->tr_debut.ru_utime);
    fprintf(ctx->tl_out, "\nBuilding the generalized Buchi automaton : %ld.%06lis",
		ctx->t_diff.tv_sec, ctx->t_diff.tv_usec);
    fprintf(ctx->tl_out, "\n%i states, %i transitions\n", ctx->gstate_count, ctx->gtrans_count);
  }

  tfree(ctx, ctx->gstack);
  free_all_atrans(ctx);
  tfree(ctx, ctx->transition);

  if(ctx->tl_verbose) {
    fprintf(ctx->tl_out, "\nGeneralized Buchi automaton before simplification\n");
    print_generalized(ctx);
  }

  if(ctx->tl_simp_diff) {
    if (ctx->tl_simp_scc) simplify_gscc(ctx);
    simplify_gtrans(ctx);
    if (ctx->tl_simp_scc) simplify_gscc(ctx);
    while(simplify_gstates(ctx)) {
      if (ctx->tl_simp_scc) simplify_gscc(ctx);
      simplify_gtrans(ctx);
      if (ctx->tl_simp_scc) simplify_gscc(ctx);
    }

    if(ctx->tl_verbose) {
      fprintf(ctx->tl_out, "\nGeneralized Buchi automaton after simplification\n");
      print_generalized(ctx);
    }
  }
}
