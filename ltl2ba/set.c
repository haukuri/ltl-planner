/***** ltl2ba : set.c *****/

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

/* type = 2 for scc set, 1 for symbol sets, 0 for nodes sets */

int *new_set(Ltl2baContext *ctx, int type) /* creates a new set */
{
  return (int *)tl_emalloc(ctx, set_size(ctx, type) * sizeof(int));
}

int *clear_set(Ltl2baContext *ctx, int *l, int type) /* clears the set */
{
  int i;
  for(i = 0; i < set_size(ctx, type); i++) {
    l[i] = 0;
  }
  return l;
}

int *make_set(Ltl2baContext *ctx, int n, int type) /* creates the set {n}, or empty if n = -1 */
{
  int *l = clear_set(ctx, new_set(ctx, type), type);
  if(n == -1) return l;
  l[n/MOD] = 1 << (n%MOD);
  return l;
}

void copy_set(Ltl2baContext *ctx, int *from, int *to, int type) /* copies a set */
{
  int i;
  for(i = 0; i < set_size(ctx, type); i++)
    to[i] = from[i];
}

int *dup_set(Ltl2baContext *ctx, int *l, int type) /* duplicates a set */
{
  int i, *m = new_set(ctx, type);
  for(i = 0; i < set_size(ctx, type); i++)
    m[i] = l[i];
  return m;
}

void merge_sets(Ltl2baContext *ctx, int *l1, int *l2, int type) /* union into l1 */
{
  int i;
  for(i = 0; i < set_size(ctx, type); i++)
    l1[i] = l1[i] | l2[i];
}

void do_merge_sets(Ltl2baContext *ctx, int *l, int *l1, int *l2, int type) /* union into l */
{
  int i;
  for(i = 0; i < set_size(ctx, type); i++)
    l[i] = l1[i] | l2[i];
}

int *intersect_sets(Ltl2baContext *ctx, int *l1, int *l2, int type)
{
  int i, *l = new_set(ctx, type);
  for(i = 0; i < set_size(ctx, type); i++)
    l[i] = l1[i] & l2[i];
  return l;
}

int empty_intersect_sets(Ltl2baContext *ctx, int *l1, int *l2, int type)
{
  int i, test = 0;
  for(i = 0; i < set_size(ctx, type); i++)
    test |= l1[i] & l2[i];
  return !test;
}

void add_set(int *l, int n) /* adds an element to a set */
{
  l[n/MOD] |= 1 << (n%MOD);
}

void rem_set(int *l, int n) /* removes an element from a set */
{
  l[n/MOD] &= (-1 - (1 << (n%MOD)));
}

void spin_print_set(Ltl2baContext *ctx, int *pos, int *neg) /* prints for spin */
{
  int i, j, start = 1;
  for(i = 0; i < ctx->sym_size; i++)
    for(j = 0; j < MOD; j++) {
      if(pos[i] & (1 << j)) {
	if(!start)
	  fprintf(ctx->tl_out, " && ");
	fprintf(ctx->tl_out, "%s", ctx->sym_table[MOD * i + j]);
	start = 0;
      }
      if(neg[i] & (1 << j)) {
	if(!start)
	  fprintf(ctx->tl_out, " && ");
	fprintf(ctx->tl_out, "!%s", ctx->sym_table[MOD * i + j]);
	start = 0;
      }
    }
  if(start)
    fprintf(ctx->tl_out, "1");
}

void print_set(Ltl2baContext *ctx, int *l, int type) /* prints the content of a set */
{
  int i, j, start = 1;;
  if(type != 1) fprintf(ctx->tl_out, "{");
  for(i = 0; i < set_size(ctx, type); i++)
    for(j = 0; j < MOD; j++)
      if(l[i] & (1 << j)) {
        switch(type) {
          case 0: case 2:
            if(!start) fprintf(ctx->tl_out, ",");
            fprintf(ctx->tl_out, "%i", MOD * i + j);
            break;
          case 1:
            if(!start) fprintf(ctx->tl_out, " & ");
            fprintf(ctx->tl_out, "%s", ctx->sym_table[MOD * i + j]);
            break;
        }
        start = 0;
      }
  if(type != 1) fprintf(ctx->tl_out, "}");
}

int empty_set(Ltl2baContext *ctx, int *l, int type) /* tests if empty */
{
  int i, test = 0;
  for(i = 0; i < set_size(ctx, type); i++)
    test |= l[i];
  return !test;
}

int same_sets(Ltl2baContext *ctx, int *l1, int *l2, int type) /* tests if identical */
{
  int i, test = 1;
  for(i = 0; i < set_size(ctx, type); i++)
    test &= (l1[i] == l2[i]);
  return test;
}

int included_set(Ltl2baContext *ctx, int *l1, int *l2, int type)
{                    /* tests if first set included in second */
  int i, test = 0;
  for(i = 0; i < set_size(ctx, type); i++)
    test |= (l1[i] & ~l2[i]);
  return !test;
}

int in_set(int *l, int n) /* tests if element is in set */
{
  return(l[n/MOD] & (1 << (n%MOD)));
}

int *list_set(Ltl2baContext *ctx, int *l, int type) /* transforms set into list */
{
  int i, j, size = 1, *list;
  for(i = 0; i < set_size(ctx, type); i++)
    for(j = 0; j < MOD; j++)
      if(l[i] & (1 << j))
	size++;
  list = (int *)tl_emalloc(ctx, size * sizeof(int));
  list[0] = size;
  size = 1;
  for(i = 0; i < set_size(ctx, type); i++)
    for(j = 0; j < MOD; j++)
      if(l[i] & (1 << j))
	list[size++] = MOD * i + j;
  return list;
}
