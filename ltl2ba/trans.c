/***** ltl2ba : trans.c *****/

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

#ifdef NXT
int
only_nxt(Node *n)
{
        switch (n->ntyp) {
        case NEXT:
                return 1;
        case OR:
        case AND:
                return only_nxt(n->rgt) && only_nxt(n->lft);
        default:
                return 0;
        }
}
#endif

int
dump_cond(Ltl2baContext *ctx, Node *pp, Node *r, int first)
{       Node *q;
        int frst = first;

        if (!pp) return frst;

        q = dupnode(ctx, pp);
        q = rewrite(q);

        if (q->ntyp == PREDICATE
        ||  q->ntyp == NOT
#ifndef NXT
        ||  q->ntyp == OR
#endif
        ||  q->ntyp == FALSE)
        {       if (!frst) fprintf(ctx->tl_out, " && ");
                dump(ctx, q);
                frst = 0;
#ifdef NXT
        } else if (q->ntyp == OR)
        {       if (!frst) fprintf(ctx->tl_out, " && ");
                fprintf(ctx->tl_out, "((");
                frst = dump_cond(ctx, q->lft, r, 1);

                if (!frst)
                        fprintf(ctx->tl_out, ") || (");
                else
                {       if (only_nxt(q->lft))
                        {       fprintf(ctx->tl_out, "1))");
                                return 0;
                        }
                }

                frst = dump_cond(ctx, q->rgt, r, 1);

                if (frst)
                {       if (only_nxt(q->rgt))
                                fprintf(ctx->tl_out, "1");
                        else
                                fprintf(ctx->tl_out, "0");
                        frst = 0;
                }

                fprintf(ctx->tl_out, "))");
#endif
        } else  if (q->ntyp == V_OPER
                && !anywhere(ctx, AND, q->rgt, r))
        {       frst = dump_cond(ctx, q->rgt, r, frst);
        } else  if (q->ntyp == AND)
        {
                frst = dump_cond(ctx, q->lft, r, frst);
                frst = dump_cond(ctx, q->rgt, r, frst);
        }

        return frst;
}

static void
sdump(Ltl2baContext *ctx, Node *n)
{
	switch (n->ntyp) {
	case PREDICATE:	strcat(ctx->dumpbuf, n->sym->name);
			break;
	case U_OPER:	strcat(ctx->dumpbuf, "U");
			goto common2;
	case V_OPER:	strcat(ctx->dumpbuf, "V");
			goto common2;
	case OR:	strcat(ctx->dumpbuf, "|");
			goto common2;
	case AND:	strcat(ctx->dumpbuf, "&");
common2:		sdump(ctx, n->rgt);
common1:		sdump(ctx, n->lft);
			break;
#ifdef NXT
	case NEXT:	strcat(ctx->dumpbuf, "X");
			goto common1;
#endif
	case NOT:	strcat(ctx->dumpbuf, "!");
			goto common1;
	case TRUE:	strcat(ctx->dumpbuf, "T");
			break;
	case FALSE:	strcat(ctx->dumpbuf, "F");
			break;
	default:	strcat(ctx->dumpbuf, "?");
			break;
	}
}

Symbol *
DoDump(Ltl2baContext *ctx, Node *n)
{
	if (!n) return ZS;

	if (n->ntyp == PREDICATE)
		return n->sym;

	ctx->dumpbuf[0] = '\0';
	sdump(ctx, n);
	return tl_lookup(ctx, ctx->dumpbuf);
}

void trans(Ltl2baContext *ctx, Node *p)
{
  if (!p || ctx->tl_errs) return;

  if (ctx->tl_verbose || ctx->tl_terse) {
    fprintf(ctx->tl_out, "\t/* Normlzd: ");
    dump(ctx, p);
    fprintf(ctx->tl_out, " */\n");
  }
  if (ctx->tl_terse)
    return;

  mk_alternating(ctx, p);
  mk_generalized(ctx);
  mk_buchi(ctx);
}
