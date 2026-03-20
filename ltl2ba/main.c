/***** ltl2ba : main.c *****/

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

#include <unistd.h>
#include "ltl2ba.h"

char *
emalloc(Ltl2baContext *ctx, int n)
{       char *tmp;

        if (!(tmp = (char *) malloc(n)))
                fatal(ctx, "not enough memory");
        memset(tmp, 0, n);
        return tmp;
}

int
tl_Getchar(Ltl2baContext *ctx)
{
	if (ctx->cnt < ctx->hasuform)
		return ctx->uform[ctx->cnt++];
	ctx->cnt++;
	return -1;
}

void
put_uform(Ltl2baContext *ctx)
{
	fprintf(ctx->tl_out, "%s", ctx->uform);
}

void
tl_UnGetchar(Ltl2baContext *ctx)
{
	if (ctx->cnt > 0) ctx->cnt--;
}

static void
tl_endstats(Ltl2baContext *ctx)
{
	printf("\ntotal memory used: %9ld\n", ctx->All_Mem);
	a_stats(ctx);
}

#define Binop(a)		\
		fprintf(ctx->tl_out, "(");	\
		dump(ctx, n->lft);		\
		fprintf(ctx->tl_out, a);	\
		dump(ctx, n->rgt);		\
		fprintf(ctx->tl_out, ")")

void
dump(Ltl2baContext *ctx, Node *n)
{
	if (!n) return;

	switch(n->ntyp) {
	case OR:	Binop(" || "); break;
	case AND:	Binop(" && "); break;
	case U_OPER:	Binop(" U ");  break;
	case V_OPER:	Binop(" V ");  break;
#ifdef NXT
	case NEXT:
		fprintf(ctx->tl_out, "X");
		fprintf(ctx->tl_out, " (");
		dump(ctx, n->lft);
		fprintf(ctx->tl_out, ")");
		break;
#endif
	case NOT:
		fprintf(ctx->tl_out, "!");
		fprintf(ctx->tl_out, " (");
		dump(ctx, n->lft);
		fprintf(ctx->tl_out, ")");
		break;
	case FALSE:
		fprintf(ctx->tl_out, "false");
		break;
	case TRUE:
		fprintf(ctx->tl_out, "true");
		break;
	case PREDICATE:
		fprintf(ctx->tl_out, "(%s)", n->sym->name);
		break;
	case -1:
		fprintf(ctx->tl_out, " D ");
		break;
	default:
		printf("Unknown token: ");
		tl_explain(n->ntyp);
		break;
	}
}

void
tl_explain(int n)
{
	switch (n) {
	case ALWAYS:	printf("[]"); break;
	case EVENTUALLY: printf("<>"); break;
	case IMPLIES:	printf("->"); break;
	case EQUIV:	printf("<->"); break;
	case PREDICATE:	printf("predicate"); break;
	case OR:	printf("||"); break;
	case AND:	printf("&&"); break;
	case NOT:	printf("!"); break;
	case U_OPER:	printf("U"); break;
	case V_OPER:	printf("V"); break;
#ifdef NXT
	case NEXT:	printf("X"); break;
#endif
	case TRUE:	printf("true"); break;
	case FALSE:	printf("false"); break;
	case ';':	printf("end of formula"); break;
	default:	printf("%c", n); break;
	}
}

static void
non_fatal(Ltl2baContext *ctx, const char *s1)
{
	int i;

	printf("ltl2ba: ");
	fputs(s1, stdout);
	if (ctx->tl_yychar != -1 && ctx->tl_yychar != 0)
	{	printf(", saw '");
		tl_explain(ctx->tl_yychar);
		printf("'");
	}
	printf("\nltl2ba: %s\n---------", ctx->uform);
	for (i = 0; i < ctx->cnt; i++)
		printf("-");
	printf("^\n");
	fflush(stdout);
	ctx->tl_errs++;
}

void
tl_yyerror(Ltl2baContext *ctx, char *s1)
{
	Fatal(ctx, s1);
}

void
Fatal(Ltl2baContext *ctx, const char *s1)
{
  non_fatal(ctx, s1);
  /* In library mode, longjmp back to the API entry point */
  if (ctx->error_code == 0) {
    ctx->error_code = 1;
    snprintf(ctx->error_msg, sizeof(ctx->error_msg), "%s", s1);
  }
  longjmp(ctx->error_jmp, 1);
}

void
fatal(Ltl2baContext *ctx, const char *s1)
{
  Fatal(ctx, s1);
}

/* Subtract the `struct timeval' values X and Y, storing the result X-Y in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */

int
timeval_subtract (result, x, y)
struct timeval *result, *x, *y;
{
	if (x->tv_usec < y->tv_usec) {
		x->tv_usec += 1000000;
		x->tv_sec--;
	}

	/* Compute the time remaining to wait. tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/********************************************************************\
|*                       CLI entry point                            *|
\********************************************************************/

static void
usage(void)
{
        printf("usage: ltl2ba [-flag] -f 'formula'\n");
        printf("                   or -F file\n");
        printf(" -f 'formula'\ttranslate LTL ");
        printf("into never claim\n");
        printf(" -F file\tlike -f, but with the LTL ");
        printf("formula stored in a 1-line file\n");
        printf(" -d\t\tdisplay automata (D)escription at each step\n");
        printf(" -s\t\tcomputing time and automata sizes (S)tatistics\n");
        printf(" -l\t\tdisable (L)ogic formula simplification\n");
        printf(" -p\t\tdisable a-(P)osteriori simplification\n");
        printf(" -o\t\tdisable (O)n-the-fly simplification\n");
        printf(" -c\t\tdisable strongly (C)onnected components simplification\n");
        printf(" -a\t\tdisable trick in (A)ccepting conditions\n");

        exit(1);
}

static int
tl_main(Ltl2baContext *ctx, int argc, char *argv[])
{       int i;
	while (argc > 1 && argv[1][0] == '-')
	{	switch (argv[1][1]) {
		case 'f':	argc--; argv++;
				for (i = 0; i < argv[1][i]; i++)
				{	if (argv[1][i] == '\t'
					||  argv[1][i] == '\"'
					||  argv[1][i] == '\n')
						argv[1][i] = ' ';
				}
				strcpy(ctx->uform, argv[1]);
				ctx->hasuform = strlen(ctx->uform);
				break;
		default :	usage();
		}
		argc--; argv++;
	}
	if (ctx->hasuform == 0) usage();
	tl_parse(ctx);
	if (ctx->tl_stats) tl_endstats(ctx);
	return ctx->tl_errs;
}

int
main(int argc, char *argv[])
{	int i;
	Ltl2baContext ctx_storage;
	Ltl2baContext *ctx = &ctx_storage;
	char **ltl_file = (char **)0;
	char **add_ltl  = (char **)0;

	ltl2ba_init_context(ctx);
	ctx->tl_out = stdout;

	while (argc > 1 && argv[1][0] == '-')
        {       switch (argv[1][1]) {
                case 'F': ltl_file = (char **) (argv+2);
                          argc--; argv++; break;
                case 'f': add_ltl = (char **) argv;
                          argc--; argv++; break;
                case 'a': ctx->tl_fjtofj = 0; break;
                case 'c': ctx->tl_simp_scc = 0; break;
                case 'o': ctx->tl_simp_fly = 0; break;
                case 'p': ctx->tl_simp_diff = 0; break;
                case 'l': ctx->tl_simp_log = 0; break;
                case 'd': ctx->tl_verbose = 1; break;
                case 's': ctx->tl_stats = 1; break;
                default : usage(); break;
                }
                argc--, argv++;
        }

	if(!ltl_file && !add_ltl) usage();

        if (ltl_file)
        {       char formula[4096];
                add_ltl = ltl_file-2; add_ltl[1][1] = 'f';
                if (!(ctx->tl_out = fopen(*ltl_file, "r")))
                {       printf("ltl2ba: cannot open %s\n", *ltl_file);
                        exit(1);
                }
                fgets(formula, 4096, ctx->tl_out);
                fclose(ctx->tl_out);
                ctx->tl_out = stdout;
                *ltl_file = (char *) formula;
        }

	/* Use setjmp for error recovery */
	if (setjmp(ctx->error_jmp) != 0) {
		/* Error occurred */
		ltl2ba_free_allocs(ctx);
		return 1;
	}

        if (argc > 1)
        {       char out1[64], out2[64];
                FILE *inp, *out;
                char buf[1024];
                strcpy(out1, "_tmp1_");
                strcpy(out2, "_tmp2_");
                inp = fopen(argv[1], "r");
                out = fopen(out2, "w");
                if (!inp || !out)
                {       printf("ltl2ba: cannot cp %s to %s\n", argv[1], out2);
                        exit(1);
                }
                while (fgets(buf, 1024, inp))
                        fprintf(out, "%s", buf);
                fclose(inp);
                ctx->tl_out = out;
                i = tl_main(ctx, 2, add_ltl);
                fclose(ctx->tl_out);
                (void) unlink((const char *)out1);
        } else
	{
                if (argc > 0)
                        i = tl_main(ctx, 2, add_ltl);
		else
			usage();
	}

	ltl2ba_free_allocs(ctx);
	return i;
}
