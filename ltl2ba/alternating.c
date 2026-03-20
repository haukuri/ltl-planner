/***** ltl2ba : alternating.c *****/

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
|*              Generation of the alternating automaton             *|
\********************************************************************/

static ATrans *build_alternating(Ltl2baContext *ctx, Node *p);

int calculate_node_size(Node *p) /* returns the number of temporal nodes */
{
  switch(p->ntyp) {
  case AND:
  case OR:
  case U_OPER:
  case V_OPER:
    return(calculate_node_size(p->lft) + calculate_node_size(p->rgt) + 1);
#ifdef NXT
  case NEXT:
    return(calculate_node_size(p->lft) + 1);
#endif
  default:
    return 1;
    break;
  }
}

int calculate_sym_size(Node *p) /* returns the number of predicates */
{
  switch(p->ntyp) {
  case AND:
  case OR:
  case U_OPER:
  case V_OPER:
    return(calculate_sym_size(p->lft) + calculate_sym_size(p->rgt));
#ifdef NXT
  case NEXT:
    return(calculate_sym_size(p->lft));
#endif
  case NOT:
  case PREDICATE:
    return 1;
  default:
    return 0;
  }
}

ATrans *dup_trans(Ltl2baContext *ctx, ATrans *trans)  /* returns copy of a transition */
{
  ATrans *result;
  if(!trans) return trans;
  result = emalloc_atrans(ctx);
  copy_set(ctx, trans->to,  result->to,  0);
  copy_set(ctx, trans->pos, result->pos, 1);
  copy_set(ctx, trans->neg, result->neg, 1);
  return result;
}

void do_merge_trans(Ltl2baContext *ctx, ATrans **result, ATrans *trans1, ATrans *trans2)
{ /* merges two transitions */
  if(!trans1 || !trans2) {
    free_atrans(ctx, *result, 0);
    *result = (ATrans *)0;
    return;
  }
  if(!*result)
    *result = emalloc_atrans(ctx);
  do_merge_sets(ctx, (*result)->to, trans1->to,  trans2->to,  0);
  do_merge_sets(ctx, (*result)->pos, trans1->pos, trans2->pos, 1);
  do_merge_sets(ctx, (*result)->neg, trans1->neg, trans2->neg, 1);
  if(!empty_intersect_sets(ctx, (*result)->pos, (*result)->neg, 1)) {
    free_atrans(ctx, *result, 0);
    *result = (ATrans *)0;
  }
}

ATrans *merge_trans(Ltl2baContext *ctx, ATrans *trans1, ATrans *trans2) /* merges two transitions */
{
  ATrans *result = emalloc_atrans(ctx);
  do_merge_trans(ctx, &result, trans1, trans2);
  return result;
}

static int already_done(Ltl2baContext *ctx, Node *p) /* finds the id of the node, if already explored */
{
  int i;
  for(i = 1; i<ctx->node_id; i++)
    if (isequal(ctx, p, ctx->label[i]))
      return i;
  return -1;
}

static int get_sym_id(Ltl2baContext *ctx, char *s) /* finds the id of a predicate */
{
  int i;
  for(i=0; i<ctx->sym_id; i++)
    if (!strcmp(s, ctx->sym_table[i]))
      return i;
  ctx->sym_table[ctx->sym_id] = s;
  return ctx->sym_id++;
}

static ATrans *boolean(Ltl2baContext *ctx, Node *p) /* transitions to boolean nodes */
{
  ATrans *t1, *t2, *lft, *rgt, *result = (ATrans *)0;
  switch(p->ntyp) {
  case TRUE:
    result = emalloc_atrans(ctx);
    clear_set(ctx, result->to,  0);
    clear_set(ctx, result->pos, 1);
    clear_set(ctx, result->neg, 1);
  case FALSE:
    break;
  case AND:
    lft = boolean(ctx, p->lft);
    rgt = boolean(ctx, p->rgt);
    for(t1 = lft; t1; t1 = t1->nxt) {
      for(t2 = rgt; t2; t2 = t2->nxt) {
	ATrans *tmp = merge_trans(ctx, t1, t2);
	if(tmp) {
	  tmp->nxt = result;
	  result = tmp;
	}
      }
    }
    free_atrans(ctx, lft, 1);
    free_atrans(ctx, rgt, 1);
    break;
  case OR:
    lft = boolean(ctx, p->lft);
    for(t1 = lft; t1; t1 = t1->nxt) {
      ATrans *tmp = dup_trans(ctx, t1);
      tmp->nxt = result;
      result = tmp;
    }
    free_atrans(ctx, lft, 1);
    rgt = boolean(ctx, p->rgt);
    for(t1 = rgt; t1; t1 = t1->nxt) {
      ATrans *tmp = dup_trans(ctx, t1);
      tmp->nxt = result;
      result = tmp;
    }
    free_atrans(ctx, rgt, 1);
    break;
  default:
    build_alternating(ctx, p);
    result = emalloc_atrans(ctx);
    clear_set(ctx, result->to,  0);
    clear_set(ctx, result->pos, 1);
    clear_set(ctx, result->neg, 1);
    add_set(result->to, already_done(ctx, p));
  }
  return result;
}

static ATrans *build_alternating(Ltl2baContext *ctx, Node *p) /* builds alternating automaton */
{
  ATrans *t1, *t2, *t = (ATrans *)0;
  int node = already_done(ctx, p);
  if(node >= 0) return ctx->transition[node];

  switch (p->ntyp) {

  case TRUE:
    t = emalloc_atrans(ctx);
    clear_set(ctx, t->to,  0);
    clear_set(ctx, t->pos, 1);
    clear_set(ctx, t->neg, 1);
  case FALSE:
    break;

  case PREDICATE:
    t = emalloc_atrans(ctx);
    clear_set(ctx, t->to,  0);
    clear_set(ctx, t->pos, 1);
    clear_set(ctx, t->neg, 1);
    add_set(t->pos, get_sym_id(ctx, p->sym->name));
    break;

  case NOT:
    t = emalloc_atrans(ctx);
    clear_set(ctx, t->to,  0);
    clear_set(ctx, t->pos, 1);
    clear_set(ctx, t->neg, 1);
    add_set(t->neg, get_sym_id(ctx, p->lft->sym->name));
    break;

#ifdef NXT
  case NEXT:
    t = boolean(ctx, p->lft);
    break;
#endif

  case U_OPER:    /* p U q <-> q || (p && X (p U q)) */
    for(t2 = build_alternating(ctx, p->rgt); t2; t2 = t2->nxt) {
      ATrans *tmp = dup_trans(ctx, t2);  /* q */
      tmp->nxt = t;
      t = tmp;
    }
    for(t1 = build_alternating(ctx, p->lft); t1; t1 = t1->nxt) {
      ATrans *tmp = dup_trans(ctx, t1);  /* p */
      add_set(tmp->to, ctx->node_id);  /* X (p U q) */
      tmp->nxt = t;
      t = tmp;
    }
    add_set(ctx->final_set, ctx->node_id);
    break;

  case V_OPER:    /* p V q <-> (p && q) || (p && X (p V q)) */
    for(t1 = build_alternating(ctx, p->rgt); t1; t1 = t1->nxt) {
      ATrans *tmp;

      for(t2 = build_alternating(ctx, p->lft); t2; t2 = t2->nxt) {
	tmp = merge_trans(ctx, t1, t2);  /* p && q */
	if(tmp) {
	  tmp->nxt = t;
	  t = tmp;
	}
      }

      tmp = dup_trans(ctx, t1);  /* p */
      add_set(tmp->to, ctx->node_id);  /* X (p V q) */
      tmp->nxt = t;
      t = tmp;
    }
    break;

  case AND:
    t = (ATrans *)0;
    for(t1 = build_alternating(ctx, p->lft); t1; t1 = t1->nxt) {
      for(t2 = build_alternating(ctx, p->rgt); t2; t2 = t2->nxt) {
	ATrans *tmp = merge_trans(ctx, t1, t2);
	if(tmp) {
	  tmp->nxt = t;
	  t = tmp;
	}
      }
    }
    break;

  case OR:
    t = (ATrans *)0;
    for(t1 = build_alternating(ctx, p->lft); t1; t1 = t1->nxt) {
      ATrans *tmp = dup_trans(ctx, t1);
      tmp->nxt = t;
      t = tmp;
    }
    for(t1 = build_alternating(ctx, p->rgt); t1; t1 = t1->nxt) {
      ATrans *tmp = dup_trans(ctx, t1);
      tmp->nxt = t;
      t = tmp;
    }
    break;

  default:
    break;
  }

  ctx->transition[ctx->node_id] = t;
  ctx->label[ctx->node_id++] = p;
  return(t);
}

/********************************************************************\
|*        Simplification of the alternating automaton               *|
\********************************************************************/

static void simplify_atrans(Ltl2baContext *ctx, ATrans **trans) /* simplifies the transitions */
{
  ATrans *t, *father = (ATrans *)0;
  for(t = *trans; t;) {
    ATrans *t1;
    for(t1 = *trans; t1; t1 = t1->nxt) {
      if((t1 != t) &&
	 included_set(ctx, t1->to,  t->to,  0) &&
	 included_set(ctx, t1->pos, t->pos, 1) &&
	 included_set(ctx, t1->neg, t->neg, 1))
	break;
    }
    if(t1) {
      if (father)
	father->nxt = t->nxt;
      else
	*trans = t->nxt;
      free_atrans(ctx, t, 0);
      if (father)
	t = father->nxt;
      else
	t = *trans;
      continue;
    }
    ctx->atrans_count++;
    father = t;
    t = t->nxt;
  }
}

static void simplify_astates(Ltl2baContext *ctx) /* simplifies the alternating automaton */
{
  ATrans *t;
  int i, *acc = make_set(ctx, -1, 0); /* no state is accessible initially */

  for(t = ctx->transition[0]; t; t = t->nxt, i = 0)
    merge_sets(ctx, acc, t->to, 0); /* all initial states are accessible */

  for(i = ctx->node_id - 1; i > 0; i--) {
    if (!in_set(acc, i)) { /* frees unaccessible states */
      ctx->label[i] = ZN;
      free_atrans(ctx, ctx->transition[i], 1);
      ctx->transition[i] = (ATrans *)0;
      continue;
    }
    ctx->astate_count++;
    simplify_atrans(ctx, &ctx->transition[i]);
    for(t = ctx->transition[i]; t; t = t->nxt)
      merge_sets(ctx, acc, t->to, 0);
  }

  tfree(ctx, acc);
}

/********************************************************************\
|*            Display of the alternating automaton                  *|
\********************************************************************/

static void print_alternating(Ltl2baContext *ctx) /* dumps the alternating automaton */
{
  int i;
  ATrans *t;

  fprintf(ctx->tl_out, "init :\n");
  for(t = ctx->transition[0]; t; t = t->nxt) {
    print_set(ctx, t->to, 0);
    fprintf(ctx->tl_out, "\n");
  }

  for(i = ctx->node_id - 1; i > 0; i--) {
    if(!ctx->label[i])
      continue;
    fprintf(ctx->tl_out, "state %i : ", i);
    dump(ctx, ctx->label[i]);
    fprintf(ctx->tl_out, "\n");
    for(t = ctx->transition[i]; t; t = t->nxt) {
      if (empty_set(ctx, t->pos, 1) && empty_set(ctx, t->neg, 1))
	fprintf(ctx->tl_out, "1");
      print_set(ctx, t->pos, 1);
      if (!empty_set(ctx, t->pos,1) && !empty_set(ctx, t->neg,1)) fprintf(ctx->tl_out, " & ");
      print_set(ctx, t->neg, 2);
      fprintf(ctx->tl_out, " -> ");
      print_set(ctx, t->to, 0);
      fprintf(ctx->tl_out, "\n");
    }
  }
}

/********************************************************************\
|*                       Main method                                *|
\********************************************************************/

void mk_alternating(Ltl2baContext *ctx, Node *p) /* generates alternating automaton */
{
  int nsize;
  if(ctx->tl_stats) getrusage(RUSAGE_SELF, &ctx->tr_debut);

  nsize = calculate_node_size(p) + 1; /* number of states in the automaton */
  ctx->label = (Node **) tl_emalloc(ctx, nsize * sizeof(Node *));
  ctx->transition = (ATrans **) tl_emalloc(ctx, nsize * sizeof(ATrans *));
  ctx->node_size = nsize / (8 * sizeof(int)) + 1;

  { int ssize = calculate_sym_size(p); /* number of predicates */
    if(ssize) ctx->sym_table = (char **) tl_emalloc(ctx, ssize * sizeof(char *));
    ctx->sym_size = ssize / (8 * sizeof(int)) + 1;
  }

  ctx->final_set = make_set(ctx, -1, 0);
  ctx->transition[0] = boolean(ctx, p); /* generates the alternating automaton */

  if(ctx->tl_verbose) {
    fprintf(ctx->tl_out, "\nAlternating automaton before simplification\n");
    print_alternating(ctx);
  }

  if(ctx->tl_simp_diff) {
    simplify_astates(ctx); /* keeps only accessible states */
    if(ctx->tl_verbose) {
      fprintf(ctx->tl_out, "\nAlternating automaton after simplification\n");
      print_alternating(ctx);
    }
  }

  if(ctx->tl_stats) {
    getrusage(RUSAGE_SELF, &ctx->tr_fin);
    timeval_subtract (&ctx->t_diff, &ctx->tr_fin.ru_utime, &ctx->tr_debut.ru_utime);
    fprintf(ctx->tl_out, "\nBuilding and simplification of the alternating automaton: %ld.%06lis",
		ctx->t_diff.tv_sec, ctx->t_diff.tv_usec);
    fprintf(ctx->tl_out, "\n%i states, %i transitions\n", ctx->astate_count, ctx->atrans_count);
  }

  releasenode(ctx, 1, p);
  tfree(ctx, ctx->label);
}
