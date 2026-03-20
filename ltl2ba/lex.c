/***** ltl2ba : lex.c *****/

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

#include <stdlib.h>
#include <ctype.h>
#include "ltl2ba.h"

static int	tl_lex(Ltl2baContext *ctx);

#define Token(y)        ctx->tl_yylval = tl_nn(ctx, y,ZN,ZN); return y

int
isalnum_(int c)
{       return (isalnum(c) || c == '_');
}

int
hash(char *s)
{       int h=0;

        while (*s)
        {       h += *s++;
                h <<= 1;
                if (h&(Nhash+1))
                        h |= 1;
        }
        return h&Nhash;
}

static void
getword(Ltl2baContext *ctx, int first, int (*tst)(int))
{	int i=0; char c;

	ctx->yytext[i++]= (char ) first;
	while (tst(c = tl_Getchar(ctx)))
		ctx->yytext[i++] = c;
	ctx->yytext[i] = '\0';
	tl_UnGetchar(ctx);
}

static int
follow(Ltl2baContext *ctx, int tok, int ifyes, int ifno)
{	int c;
	char buf[32];

	if ((c = tl_Getchar(ctx)) == tok)
		return ifyes;
	tl_UnGetchar(ctx);
	ctx->tl_yychar = c;
	sprintf(buf, "expected '%c'", tok);
	tl_yyerror(ctx, buf);	/* no return from here */
	return ifno;
}

int
tl_yylex(Ltl2baContext *ctx)
{	int c = tl_lex(ctx);
#if 0
	printf("c = %d\n", c);
#endif
	return c;
}

static int
tl_lex(Ltl2baContext *ctx)
{	int c;

	do {
		c = tl_Getchar(ctx);
		ctx->yytext[0] = (char ) c;
		ctx->yytext[1] = '\0';

		if (c <= 0)
		{	Token(';');
		}

	} while (c == ' ');	/* '\t' is removed in tl_main.c */

	if (islower(c))
	{	getword(ctx, c, isalnum_);
		if (strcmp("true", ctx->yytext) == 0)
		{	Token(TRUE);
		}
		if (strcmp("false", ctx->yytext) == 0)
		{	Token(FALSE);
		}
		ctx->tl_yylval = tl_nn(ctx, PREDICATE,ZN,ZN);
		ctx->tl_yylval->sym = tl_lookup(ctx, ctx->yytext);
		return PREDICATE;
	}
	if (c == '<')
	{	c = tl_Getchar(ctx);
		if (c == '>')
		{	Token(EVENTUALLY);
		}
		if (c != '-')
		{	tl_UnGetchar(ctx);
			tl_yyerror(ctx, "expected '<>' or '<->'");
		}
		c = tl_Getchar(ctx);
		if (c == '>')
		{	Token(EQUIV);
		}
		tl_UnGetchar(ctx);
		tl_yyerror(ctx, "expected '<->'");
	}
	if (c == 'N')
	{	c = tl_Getchar(ctx);
		if (c != 'O')
		{	tl_UnGetchar(ctx);
			tl_yyerror(ctx, "expected 'NOT'");
		}
		c = tl_Getchar(ctx);
		if (c == 'T')
		{	Token(NOT);
		}
		tl_UnGetchar(ctx);
		tl_yyerror(ctx, "expected 'NOT'");
	}

	switch (c) {
	case '/' : c = follow(ctx, '\\', AND, '/'); break;
	case '\\': c = follow(ctx, '/', OR, '\\'); break;
	case '&' : c = follow(ctx, '&', AND, '&'); break;
	case '|' : c = follow(ctx, '|', OR, '|'); break;
	case '[' : c = follow(ctx, ']', ALWAYS, '['); break;
	case '-' : c = follow(ctx, '>', IMPLIES, '-'); break;
	case '!' : c = NOT; break;
	case 'U' : c = U_OPER; break;
	case 'V' : c = V_OPER; break;
#ifdef NXT
	case 'X' : c = NEXT; break;
#endif
	default  : break;
	}
	Token(c);
}

Symbol *
tl_lookup(Ltl2baContext *ctx, char *s)
{	Symbol *sp;
	int h = hash(s);

	for (sp = ctx->symtab[h]; sp; sp = sp->next)
		if (strcmp(sp->name, s) == 0)
			return sp;

	sp = (Symbol *) tl_emalloc(ctx, sizeof(Symbol));
	sp->name = (char *) tl_emalloc(ctx, strlen(s) + 1);
	strcpy(sp->name, s);
	sp->next = ctx->symtab[h];
	ctx->symtab[h] = sp;

	return sp;
}

Symbol *
getsym(Ltl2baContext *ctx, Symbol *s)
{	Symbol *n = (Symbol *) tl_emalloc(ctx, sizeof(Symbol));

	n->name = s->name;
	return n;
}
