/***** ltl2ba : parse.c *****/

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

static Node	*tl_formula(Ltl2baContext *ctx);
static Node	*tl_factor(Ltl2baContext *ctx);
static Node	*tl_level(Ltl2baContext *ctx, int);

static int	prec[2][4] = {
	{ U_OPER,  V_OPER, 0, 0},  /* left associative */
	{ OR, AND, IMPLIES, EQUIV, },	/* left associative */
};

static int
implies(Ltl2baContext *ctx, Node *a, Node *b)
{
  return
    (isequal(ctx, a,b) ||
     b->ntyp == TRUE ||
     a->ntyp == FALSE ||
     (b->ntyp == AND && implies(ctx, a, b->lft) && implies(ctx, a, b->rgt)) ||
     (a->ntyp == OR && implies(ctx, a->lft, b) && implies(ctx, a->rgt, b)) ||
     (a->ntyp == AND && (implies(ctx, a->lft, b) || implies(ctx, a->rgt, b))) ||
     (b->ntyp == OR && (implies(ctx, a, b->lft) || implies(ctx, a, b->rgt))) ||
     (b->ntyp == U_OPER && implies(ctx, a, b->rgt)) ||
     (a->ntyp == V_OPER && implies(ctx, a->rgt, b)) ||
     (a->ntyp == U_OPER && implies(ctx, a->lft, b) && implies(ctx, a->rgt, b)) ||
     (b->ntyp == V_OPER && implies(ctx, a, b->lft) && implies(ctx, a, b->rgt)) ||
     ((a->ntyp == U_OPER || a->ntyp == V_OPER) && a->ntyp == b->ntyp &&
         implies(ctx, a->lft, b->lft) && implies(ctx, a->rgt, b->rgt)));
}

static Node *
bin_simpler(Ltl2baContext *ctx, Node *ptr)
{	Node *a, *b;

	if (ptr)
	switch (ptr->ntyp) {
	case U_OPER:
		if (ptr->rgt->ntyp == TRUE
		||  ptr->rgt->ntyp == FALSE
		||  ptr->lft->ntyp == FALSE)
		{	ptr = ptr->rgt;
			break;
		}
		if (implies(ctx, ptr->lft, ptr->rgt)) /* NEW */
		{	ptr = ptr->rgt;
		        break;
		}
		if (ptr->lft->ntyp == U_OPER
		&&  isequal(ctx, ptr->lft->lft, ptr->rgt))
		{	/* (p U q) U p = (q U p) */
			ptr->lft = ptr->lft->rgt;
			break;
		}
		if (ptr->rgt->ntyp == U_OPER
		&&  implies(ctx, ptr->lft, ptr->rgt->lft))
		{	/* NEW */
			ptr = ptr->rgt;
			break;
		}
#ifdef NXT
		/* X p U X q == X (p U q) */
		if (ptr->rgt->ntyp == NEXT
		&&  ptr->lft->ntyp == NEXT)
		{	ptr = tl_nn(ctx, NEXT,
				tl_nn(ctx, U_OPER,
					ptr->lft->lft,
					ptr->rgt->lft), ZN);
		        break;
		}

		/* NEW : F X p == X F p */
		if (ptr->lft->ntyp == TRUE &&
		    ptr->rgt->ntyp == NEXT) {
		  ptr = tl_nn(ctx, NEXT, tl_nn(ctx, U_OPER, True, ptr->rgt->lft), ZN);
		  break;
		}

#endif

		/* NEW : F G F p == G F p */
		if (ptr->lft->ntyp == TRUE &&
		    ptr->rgt->ntyp == V_OPER &&
		    ptr->rgt->lft->ntyp == FALSE &&
		    ptr->rgt->rgt->ntyp == U_OPER &&
		    ptr->rgt->rgt->lft->ntyp == TRUE) {
		  ptr = ptr->rgt;
		  break;
		}

		/* NEW */
		if (ptr->lft->ntyp != TRUE &&
		    implies(ctx, push_negation(ctx, tl_nn(ctx, NOT, dupnode(ctx, ptr->rgt), ZN)),
			    ptr->lft))
		{       ptr->lft = True;
		        break;
		}
		break;
	case V_OPER:
		if (ptr->rgt->ntyp == FALSE
		||  ptr->rgt->ntyp == TRUE
		||  ptr->lft->ntyp == TRUE)
		{	ptr = ptr->rgt;
			break;
		}
		if (implies(ctx, ptr->rgt, ptr->lft))
		{	/* p V p = p */
			ptr = ptr->rgt;
			break;
		}
		/* F V (p V q) == F V q */
		if (ptr->lft->ntyp == FALSE
		&&  ptr->rgt->ntyp == V_OPER)
		{	ptr->rgt = ptr->rgt->rgt;
			break;
		}
#ifdef NXT
		/* NEW : G X p == X G p */
		if (ptr->lft->ntyp == FALSE &&
		    ptr->rgt->ntyp == NEXT) {
		  ptr = tl_nn(ctx, NEXT, tl_nn(ctx, V_OPER, False, ptr->rgt->lft), ZN);
		  break;
		}
#endif
		/* NEW : G F G p == F G p */
		if (ptr->lft->ntyp == FALSE &&
		    ptr->rgt->ntyp == U_OPER &&
		    ptr->rgt->lft->ntyp == TRUE &&
		    ptr->rgt->rgt->ntyp == V_OPER &&
		    ptr->rgt->rgt->lft->ntyp == FALSE) {
		  ptr = ptr->rgt;
		  break;
		}

		/* NEW */
		if (ptr->rgt->ntyp == V_OPER
		&&  implies(ctx, ptr->rgt->lft, ptr->lft))
		{	ptr = ptr->rgt;
			break;
		}

		/* NEW */
		if (ptr->lft->ntyp != FALSE &&
		    implies(ctx, ptr->lft,
			    push_negation(ctx, tl_nn(ctx, NOT, dupnode(ctx, ptr->rgt), ZN))))
		{       ptr->lft = False;
		        break;
		}
		break;
#ifdef NXT
	case NEXT:
		/* NEW : X G F p == G F p */
		if (ptr->lft->ntyp == V_OPER &&
		    ptr->lft->lft->ntyp == FALSE &&
		    ptr->lft->rgt->ntyp == U_OPER &&
		    ptr->lft->rgt->lft->ntyp == TRUE) {
		  ptr = ptr->lft;
		  break;
		}
		/* NEW : X F G p == F G p */
		if (ptr->lft->ntyp == U_OPER &&
		    ptr->lft->lft->ntyp == TRUE &&
		    ptr->lft->rgt->ntyp == V_OPER &&
		    ptr->lft->rgt->lft->ntyp == FALSE) {
		  ptr = ptr->lft;
		  break;
		}
		break;
#endif
	case IMPLIES:
		if (implies(ctx, ptr->lft, ptr->rgt))
		  {	ptr = True;
			break;
		}
		ptr = tl_nn(ctx, OR, Not(ptr->lft), ptr->rgt);
		ptr = rewrite(ptr);
		break;
	case EQUIV:
		if (implies(ctx, ptr->lft, ptr->rgt) &&
		    implies(ctx, ptr->rgt, ptr->lft))
		  {	ptr = True;
			break;
		}
		a = rewrite(tl_nn(ctx, AND,
			dupnode(ctx, ptr->lft),
			dupnode(ctx, ptr->rgt)));
		b = rewrite(tl_nn(ctx, AND,
			Not(ptr->lft),
			Not(ptr->rgt)));
		ptr = tl_nn(ctx, OR, a, b);
		ptr = rewrite(ptr);
		break;
	case AND:
		/* p && (q U p) = p */
		if (ptr->rgt->ntyp == U_OPER
		&&  isequal(ctx, ptr->rgt->rgt, ptr->lft))
		{	ptr = ptr->lft;
			break;
		}
		if (ptr->lft->ntyp == U_OPER
		&&  isequal(ctx, ptr->lft->rgt, ptr->rgt))
		{	ptr = ptr->rgt;
			break;
		}

		/* p && (q V p) == q V p */
		if (ptr->rgt->ntyp == V_OPER
		&&  isequal(ctx, ptr->rgt->rgt, ptr->lft))
		{	ptr = ptr->rgt;
			break;
		}
		if (ptr->lft->ntyp == V_OPER
		&&  isequal(ctx, ptr->lft->rgt, ptr->rgt))
		{	ptr = ptr->lft;
			break;
		}

		/* (p U q) && (r U q) = (p && r) U q*/
		if (ptr->rgt->ntyp == U_OPER
		&&  ptr->lft->ntyp == U_OPER
		&&  isequal(ctx, ptr->rgt->rgt, ptr->lft->rgt))
		{	ptr = tl_nn(ctx, U_OPER,
				tl_nn(ctx, AND, ptr->lft->lft, ptr->rgt->lft),
				ptr->lft->rgt);
			break;
		}

		/* (p V q) && (p V r) = p V (q && r) */
		if (ptr->rgt->ntyp == V_OPER
		&&  ptr->lft->ntyp == V_OPER
		&&  isequal(ctx, ptr->rgt->lft, ptr->lft->lft))
		{	ptr = tl_nn(ctx, V_OPER,
				ptr->rgt->lft,
				tl_nn(ctx, AND, ptr->lft->rgt, ptr->rgt->rgt));
			break;
		}
#ifdef NXT
		/* X p && X q == X (p && q) */
		if (ptr->rgt->ntyp == NEXT
		&&  ptr->lft->ntyp == NEXT)
		{	ptr = tl_nn(ctx, NEXT,
				tl_nn(ctx, AND,
					ptr->rgt->lft,
					ptr->lft->lft), ZN);
			break;
		}
#endif

		/* (p V q) && (r U q) == p V q */
		if (ptr->rgt->ntyp == U_OPER
		&&  ptr->lft->ntyp == V_OPER
		&&  isequal(ctx, ptr->lft->rgt, ptr->rgt->rgt))
		{	ptr = ptr->lft;
			break;
		}

		if (isequal(ctx, ptr->lft, ptr->rgt)	/* (p && p) == p */
		||  ptr->rgt->ntyp == FALSE	/* (p && F) == F */
		||  ptr->lft->ntyp == TRUE	/* (T && p) == p */
		||  implies(ctx, ptr->rgt, ptr->lft))/* NEW */
		{	ptr = ptr->rgt;
			break;
		}
		if (ptr->rgt->ntyp == TRUE	/* (p && T) == p */
		||  ptr->lft->ntyp == FALSE	/* (F && p) == F */
		||  implies(ctx, ptr->lft, ptr->rgt))/* NEW */
		{	ptr = ptr->lft;
			break;
		}

		/* NEW : F G p && F G q == F G (p && q) */
		if (ptr->lft->ntyp == U_OPER &&
		    ptr->lft->lft->ntyp == TRUE &&
		    ptr->lft->rgt->ntyp == V_OPER &&
		    ptr->lft->rgt->lft->ntyp == FALSE &&
		    ptr->rgt->ntyp == U_OPER &&
		    ptr->rgt->lft->ntyp == TRUE &&
		    ptr->rgt->rgt->ntyp == V_OPER &&
		    ptr->rgt->rgt->lft->ntyp == FALSE)
		  {
		    ptr = tl_nn(ctx, U_OPER, True,
				tl_nn(ctx, V_OPER, False,
				      tl_nn(ctx, AND, ptr->lft->rgt->rgt,
					    ptr->rgt->rgt->rgt)));
		    break;
		  }

		/* NEW */
		if (implies(ctx, ptr->lft,
			    push_negation(ctx, tl_nn(ctx, NOT, dupnode(ctx, ptr->rgt), ZN)))
		 || implies(ctx, ptr->rgt,
			    push_negation(ctx, tl_nn(ctx, NOT, dupnode(ctx, ptr->lft), ZN))))
		{       ptr = False;
		        break;
		}
		break;

	case OR:
		/* p || (q U p) == q U p */
		if (ptr->rgt->ntyp == U_OPER
		&&  isequal(ctx, ptr->rgt->rgt, ptr->lft))
		{	ptr = ptr->rgt;
			break;
		}

		/* p || (q V p) == p */
		if (ptr->rgt->ntyp == V_OPER
		&&  isequal(ctx, ptr->rgt->rgt, ptr->lft))
		{	ptr = ptr->lft;
			break;
		}

		/* (p U q) || (p U r) = p U (q || r) */
		if (ptr->rgt->ntyp == U_OPER
		&&  ptr->lft->ntyp == U_OPER
		&&  isequal(ctx, ptr->rgt->lft, ptr->lft->lft))
		{	ptr = tl_nn(ctx, U_OPER,
				ptr->rgt->lft,
				tl_nn(ctx, OR, ptr->lft->rgt, ptr->rgt->rgt));
			break;
		}

		if (isequal(ctx, ptr->lft, ptr->rgt)	/* (p || p) == p */
		||  ptr->rgt->ntyp == FALSE	/* (p || F) == p */
		||  ptr->lft->ntyp == TRUE	/* (T || p) == T */
		||  implies(ctx, ptr->rgt, ptr->lft))/* NEW */
		{	ptr = ptr->lft;
			break;
		}
		if (ptr->rgt->ntyp == TRUE	/* (p || T) == T */
		||  ptr->lft->ntyp == FALSE	/* (F || p) == p */
		||  implies(ctx, ptr->lft, ptr->rgt))/* NEW */
		{	ptr = ptr->rgt;
			break;
		}

		/* (p V q) || (r V q) = (p || r) V q */
		if (ptr->rgt->ntyp == V_OPER
		&&  ptr->lft->ntyp == V_OPER
		&&  isequal(ctx, ptr->lft->rgt, ptr->rgt->rgt))
		{	ptr = tl_nn(ctx, V_OPER,
				tl_nn(ctx, OR, ptr->lft->lft, ptr->rgt->lft),
				ptr->rgt->rgt);
			break;
		}

		/* (p V q) || (r U q) == r U q */
		if (ptr->rgt->ntyp == U_OPER
		&&  ptr->lft->ntyp == V_OPER
		&&  isequal(ctx, ptr->lft->rgt, ptr->rgt->rgt))
		{	ptr = ptr->rgt;
			break;
		}

		/* NEW : G F p || G F q == G F (p || q) */
		if (ptr->lft->ntyp == V_OPER &&
		    ptr->lft->lft->ntyp == FALSE &&
		    ptr->lft->rgt->ntyp == U_OPER &&
		    ptr->lft->rgt->lft->ntyp == TRUE &&
		    ptr->rgt->ntyp == V_OPER &&
		    ptr->rgt->lft->ntyp == FALSE &&
		    ptr->rgt->rgt->ntyp == U_OPER &&
		    ptr->rgt->rgt->lft->ntyp == TRUE)
		  {
		    ptr = tl_nn(ctx, V_OPER, False,
				tl_nn(ctx, U_OPER, True,
				      tl_nn(ctx, OR, ptr->lft->rgt->rgt,
					    ptr->rgt->rgt->rgt)));
		    break;
		  }

		/* NEW */
		if (implies(ctx, push_negation(ctx, tl_nn(ctx, NOT, dupnode(ctx, ptr->rgt), ZN)),
			    ptr->lft)
		 || implies(ctx, push_negation(ctx, tl_nn(ctx, NOT, dupnode(ctx, ptr->lft), ZN)),
			    ptr->rgt))
		{       ptr = True;
		        break;
		}
		break;
	}
	return ptr;
}

static Node *
bin_minimal(Ltl2baContext *ctx, Node *ptr)
{       if (ptr)
	switch (ptr->ntyp) {
	case IMPLIES:
		return tl_nn(ctx, OR, Not(ptr->lft), ptr->rgt);
	case EQUIV:
		return tl_nn(ctx, OR,
			     tl_nn(ctx, AND,dupnode(ctx, ptr->lft),dupnode(ctx, ptr->rgt)),
			     tl_nn(ctx, AND,Not(ptr->lft),Not(ptr->rgt)));
	}
	return ptr;
}

static Node *
tl_factor(Ltl2baContext *ctx)
{	Node *ptr = ZN;

	switch (ctx->tl_yychar) {
	case '(':
		ptr = tl_formula(ctx);
		if (ctx->tl_yychar != ')')
			tl_yyerror(ctx, "expected ')'");
		ctx->tl_yychar = tl_yylex(ctx);
		goto simpl;
	case NOT:
		ptr = ctx->tl_yylval;
		ctx->tl_yychar = tl_yylex(ctx);
		ptr->lft = tl_factor(ctx);
		ptr = push_negation(ctx, ptr);
		goto simpl;
	case ALWAYS:
		ctx->tl_yychar = tl_yylex(ctx);

		ptr = tl_factor(ctx);

		if(ctx->tl_simp_log) {
		  if (ptr->ntyp == FALSE
		      ||  ptr->ntyp == TRUE)
		    break;	/* [] false == false */

		  if (ptr->ntyp == V_OPER)
		    {	if (ptr->lft->ntyp == FALSE)
		      break;	/* [][]p = []p */

		    ptr = ptr->rgt;	/* [] (p V q) = [] q */
		    }
		}

		ptr = tl_nn(ctx, V_OPER, False, ptr);
		goto simpl;
#ifdef NXT
	case NEXT:
		ctx->tl_yychar = tl_yylex(ctx);

		ptr = tl_factor(ctx);

		if ((ptr->ntyp == TRUE || ptr->ntyp == FALSE)&& ctx->tl_simp_log)
			break;	/* X true = true , X false = false */

		ptr = tl_nn(ctx, NEXT, ptr, ZN);
		goto simpl;
#endif
	case EVENTUALLY:
		ctx->tl_yychar = tl_yylex(ctx);

		ptr = tl_factor(ctx);

		if(ctx->tl_simp_log) {
		  if (ptr->ntyp == TRUE
		      ||  ptr->ntyp == FALSE)
		    break;	/* <> true == true */

		  if (ptr->ntyp == U_OPER
		      &&  ptr->lft->ntyp == TRUE)
		    break;	/* <><>p = <>p */

		  if (ptr->ntyp == U_OPER)
		    {	/* <> (p U q) = <> q */
		      ptr = ptr->rgt;
		      /* fall thru */
		    }
		}

		ptr = tl_nn(ctx, U_OPER, True, ptr);
	simpl:
		if (ctx->tl_simp_log)
		  ptr = bin_simpler(ctx, ptr);
		break;
	case PREDICATE:
		ptr = ctx->tl_yylval;
		ctx->tl_yychar = tl_yylex(ctx);
		break;
	case TRUE:
	case FALSE:
		ptr = ctx->tl_yylval;
		ctx->tl_yychar = tl_yylex(ctx);
		break;
	}
	if (!ptr) tl_yyerror(ctx, "expected predicate");
#if 0
	printf("factor:	");
	tl_explain(ptr->ntyp);
	printf("\n");
#endif
	return ptr;
}

static Node *
tl_level(Ltl2baContext *ctx, int nr)
{	int i; Node *ptr = ZN;

	if (nr < 0)
		return tl_factor(ctx);

	ptr = tl_level(ctx, nr-1);
again:
	for (i = 0; i < 4; i++)
		if (ctx->tl_yychar == prec[nr][i])
		{	ctx->tl_yychar = tl_yylex(ctx);
			ptr = tl_nn(ctx, prec[nr][i],
				ptr, tl_level(ctx, nr-1));
			if(ctx->tl_simp_log) ptr = bin_simpler(ctx, ptr);
			else ptr = bin_minimal(ctx, ptr);
			goto again;
		}
	if (!ptr) tl_yyerror(ctx, "syntax error");
#if 0
	printf("level %d:	", nr);
	tl_explain(ptr->ntyp);
	printf("\n");
#endif
	return ptr;
}

static Node *
tl_formula(Ltl2baContext *ctx)
{	ctx->tl_yychar = tl_yylex(ctx);
	return tl_level(ctx, 1);	/* 2 precedence levels, 1 and 0 */
}

void
tl_parse(Ltl2baContext *ctx)
{       Node *n = tl_formula(ctx);
        if (ctx->tl_verbose)
	{	printf("formula: ");
		put_uform(ctx);
		printf("\n");
	}
	trans(ctx, n);
}
