/***** ltl2ba : ltl2ba.h *****/

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

#ifndef LTL2BA_H
#define LTL2BA_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <setjmp.h>

/* Forward declaration */
typedef struct Ltl2baContext Ltl2baContext;

/********************************************************************\
|*                     Data structures                              *|
\********************************************************************/

typedef struct Symbol {
	char		*name;
	struct Symbol	*next;	/* linked list, symbol table */
} Symbol;

typedef struct Node {
	short		ntyp;	/* node type */
	struct Symbol	*sym;
	struct Node	*lft;	/* tree */
	struct Node	*rgt;	/* tree */
	struct Node	*nxt;	/* if linked list */
} Node;

typedef struct Graph {
	Symbol		*name;
	Symbol		*incoming;
	Symbol		*outgoing;
	Symbol		*oldstring;
	Symbol		*nxtstring;
	Node		*New;
	Node		*Old;
	Node		*Other;
	Node		*Next;
	unsigned char	isred[64], isgrn[64];
	unsigned char	redcnt, grncnt;
	unsigned char	reachable;
	struct Graph	*nxt;
} Graph;

typedef struct Mapping {
	char	*from;
	Graph	*to;
	struct Mapping	*nxt;
} Mapping;

typedef struct ATrans {
  int *to;
  int *pos;
  int *neg;
  struct ATrans *nxt;
} ATrans;

typedef struct AProd {
  int astate;
  struct ATrans *prod;
  struct ATrans *trans;
  struct AProd *nxt;
  struct AProd *prv;
} AProd;

typedef struct GTrans {
  int *pos;
  int *neg;
  struct GState *to;
  int *final;
  struct GTrans *nxt;
} GTrans;

typedef struct GState {
  int id;
  int incoming;
  int *nodes_set;
  struct GTrans *trans;
  struct GState *nxt;
  struct GState *prv;
} GState;

typedef struct BTrans {
  struct BState *to;
  int *pos;
  int *neg;
  struct BTrans *nxt;
} BTrans;

typedef struct BState {
  struct GState *gstate;
  int id;
  int incoming;
  int final;
  struct BTrans *trans;
  struct BState *nxt;
  struct BState *prv;
} BState;

typedef struct GScc {
  struct GState *gstate;
  int rank;
  int theta;
  struct GScc *nxt;
} GScc;

typedef struct BScc {
  struct BState *bstate;
  int rank;
  int theta;
  struct BScc *nxt;
} BScc;

typedef struct Cache {
  Node *before;
  Node *after;
  int same;
  struct Cache *nxt;
} Cache;

/* Memory tracking for cleanup */
typedef struct AllocBlock {
  void *ptr;
  struct AllocBlock *next;
} AllocBlock;

/********************************************************************\
|*                       Token types                                *|
\********************************************************************/

enum {
	ALWAYS=257,
	AND,		/* 258 */
	EQUIV,		/* 259 */
	EVENTUALLY,	/* 260 */
	FALSE,		/* 261 */
	IMPLIES,	/* 262 */
	NOT,		/* 263 */
	OR,		/* 264 */
	PREDICATE,	/* 265 */
	TRUE,		/* 266 */
	U_OPER,		/* 267 */
	V_OPER		/* 268 */
#ifdef NXT
	, NEXT		/* 269 */
#endif
};

/********************************************************************\
|*                    Context structure                              *|
\********************************************************************/

#define Nhash		255
#define A_LARGE		80
#define NREVENT		3
#define MOD		(8 * (int)sizeof(int))

struct Ltl2baContext {
  /* --- Error handling --- */
  int            error_code;
  char           error_msg[512];
  jmp_buf        error_jmp;

  /* --- Configuration flags --- */
  int  tl_stats;
  int  tl_simp_log;
  int  tl_simp_diff;
  int  tl_simp_fly;
  int  tl_simp_scc;
  int  tl_fjtofj;
  int  tl_verbose;
  int  tl_terse;
  int  tl_errs;

  /* --- I/O --- */
  FILE          *tl_out;
  char           uform[4096];
  int            hasuform;
  int            cnt;

  /* --- Lexer (lex.c) --- */
  Symbol        *symtab[Nhash + 1];
  char           yytext[2048];

  /* --- Parser (parse.c) --- */
  int            tl_yychar;
  Node          *tl_yylval;

  /* --- Memory management (mem.c) --- */
  unsigned long  All_Mem;
  void          *freelist[A_LARGE];
  long           mem_req[A_LARGE];
  long           mem_event[NREVENT][A_LARGE];
  ATrans        *atrans_list;
  GTrans        *gtrans_list;
  BTrans        *btrans_list;
  int            aallocs, afrees, apool;
  int            gallocs, gfrees, gpool;
  int            ballocs, bfrees, bpool;

  /* --- Allocation tracking for cleanup --- */
  AllocBlock    *alloc_list;

  /* --- Cache (cache.c) --- */
  Cache         *cache_stored;
  unsigned long  Caches;
  unsigned long  CacheHits;

  /* --- Rewrite (rewrt.c) --- */
  Node          *can;

  /* --- Trans (trans.c) --- */
  int            Stack_mx;
  int            Max_Red;
  int            Total;
  char           dumpbuf[2048];

  /* --- Alternating automaton (alternating.c) --- */
  Node         **label;
  char         **sym_table;
  ATrans       **transition;
  int           *final_set;
  int            node_id;
  int            sym_id;
  int            node_size;
  int            sym_size;
  int            astate_count;
  int            atrans_count;
  struct rusage  tr_debut, tr_fin;
  struct timeval t_diff;

  /* --- Generalized Buchi (generalized.c) --- */
  GState        *gstack;
  GState        *gremoved;
  GState        *gstates;
  GState       **init;
  GScc          *gscc_stack;
  int            init_size;
  int            gstate_id;
  int            gstate_count;
  int            gtrans_count;
  int           *fin;
  int           *final_list;
  int            scc_id;
  int            scc_size;
  int           *bad_scc;
  int            grank;

  /* --- Buchi automaton (buchi.c) --- */
  BState        *bstack;
  BState        *bstates;
  BState        *bremoved;
  BScc          *bscc_stack;
  int            accept;
  int            bstate_count;
  int            btrans_count;
  int            brank;
};

/********************************************************************\
|*                  Function prototypes                             *|
\********************************************************************/

/* cache.c / node operations */
Node	*in_cache(Ltl2baContext *ctx, Node *);
Node	*cached(Ltl2baContext *ctx, Node *);
void	cache_stats(Ltl2baContext *ctx);
void	releasenode(Ltl2baContext *ctx, int, Node *);
Node	*tl_nn(Ltl2baContext *ctx, int, Node *, Node *);
Node	*getnode(Ltl2baContext *ctx, Node *);
Node	*dupnode(Ltl2baContext *ctx, Node *);
int	sameform(Ltl2baContext *ctx, Node *, Node *);
int	isequal(Ltl2baContext *ctx, Node *, Node *);
int	anywhere(Ltl2baContext *ctx, int, Node *, Node *);

/* rewrt.c */
Node	*Canonical(Ltl2baContext *ctx, Node *);
Node	*canonical(Ltl2baContext *ctx, Node *);
Node	*push_negation(Ltl2baContext *ctx, Node *);
Node	*right_linked(Node *);

/* lex.c */
Symbol	*tl_lookup(Ltl2baContext *ctx, char *);
Symbol	*getsym(Ltl2baContext *ctx, Symbol *);
int	tl_yylex(Ltl2baContext *ctx);

/* parse.c */
void	tl_parse(Ltl2baContext *ctx);

/* main.c */
char	*emalloc(Ltl2baContext *ctx, int);
int	tl_Getchar(Ltl2baContext *ctx);
void	tl_UnGetchar(Ltl2baContext *ctx);
void	put_uform(Ltl2baContext *ctx);
void	dump(Ltl2baContext *ctx, Node *);
void	tl_explain(int);
void	Fatal(Ltl2baContext *ctx, const char *);
void	fatal(Ltl2baContext *ctx, const char *);
void	tl_yyerror(Ltl2baContext *ctx, char *);

/* mem.c */
void	*tl_emalloc(Ltl2baContext *ctx, int);
void	tfree(Ltl2baContext *ctx, void *);
ATrans	*emalloc_atrans(Ltl2baContext *ctx);
void	free_atrans(Ltl2baContext *ctx, ATrans *, int);
void	free_all_atrans(Ltl2baContext *ctx);
GTrans	*emalloc_gtrans(Ltl2baContext *ctx);
void	free_gtrans(Ltl2baContext *ctx, GTrans *, GTrans *, int);
BTrans	*emalloc_btrans(Ltl2baContext *ctx);
void	free_btrans(Ltl2baContext *ctx, BTrans *, BTrans *, int);
void	a_stats(Ltl2baContext *ctx);

/* set.c */
int	*new_set(Ltl2baContext *ctx, int);
int	*clear_set(Ltl2baContext *ctx, int *, int);
int	*make_set(Ltl2baContext *ctx, int, int);
void	copy_set(Ltl2baContext *ctx, int *, int *, int);
int	*dup_set(Ltl2baContext *ctx, int *, int);
void	merge_sets(Ltl2baContext *ctx, int *, int *, int);
void	do_merge_sets(Ltl2baContext *ctx, int *, int *, int *, int);
int	*intersect_sets(Ltl2baContext *ctx, int *, int *, int);
int	empty_intersect_sets(Ltl2baContext *ctx, int *, int *, int);
void	add_set(int *, int);
void	rem_set(int *, int);
void	spin_print_set(Ltl2baContext *ctx, int *, int *);
void	print_set(Ltl2baContext *ctx, int *, int);
int	empty_set(Ltl2baContext *ctx, int *, int);
int	same_sets(Ltl2baContext *ctx, int *, int *, int);
int	included_set(Ltl2baContext *ctx, int *, int *, int);
int	in_set(int *, int);
int	*list_set(Ltl2baContext *ctx, int *, int);

/* trans.c */
int	dump_cond(Ltl2baContext *ctx, Node *, Node *, int);
Symbol	*DoDump(Ltl2baContext *ctx, Node *);
void	trans(Ltl2baContext *ctx, Node *);
#ifdef NXT
int	only_nxt(Node *);
#endif

/* alternating.c */
ATrans	*dup_trans(Ltl2baContext *ctx, ATrans *);
ATrans	*merge_trans(Ltl2baContext *ctx, ATrans *, ATrans *);
void	do_merge_trans(Ltl2baContext *ctx, ATrans **, ATrans *, ATrans *);
void	mk_alternating(Ltl2baContext *ctx, Node *);

/* generalized.c */
void	mk_generalized(Ltl2baContext *ctx);

/* buchi.c */
void	mk_buchi(Ltl2baContext *ctx);

/* main.c - misc */
int	timeval_subtract(struct timeval *, struct timeval *, struct timeval *);
void	addtrans(Graph *, char *, Node *, char *);
void	fsm_print(void);

/********************************************************************\
|*                        Macros                                    *|
\********************************************************************/

#define ZN	(Node *)0
#define ZS	(Symbol *)0

/* These macros require 'ctx' to be in scope */
#define True	tl_nn(ctx, TRUE,  ZN, ZN)
#define False	tl_nn(ctx, FALSE, ZN, ZN)
#define Not(a)	push_negation(ctx, tl_nn(ctx, NOT, a, ZN))
#define rewrite(n)	canonical(ctx, right_linked(n))

typedef Node	*Nodeptr;
#define YYSTYPE	 Nodeptr

#define Debug(x)	{ if (0) printf(x); }
#define Debug2(x,y)	{ if (ctx->tl_verbose) printf(x,y); }
#define Dump(x)		{ if (0) dump(ctx, x); }
#define Explain(x)	{ if (ctx->tl_verbose) tl_explain(x); }

#define Assert(x, y)	{ if (!(x)) { tl_explain(y); \
			  Fatal(ctx, ": assertion failed\n"); } }
#define min(x,y)        ((x<y)?x:y)

/* set_size macro: type 0=node, 1=sym, 2=scc */
#define set_size(ctx, t) ((t)==1?(ctx)->sym_size:((t)==2?(ctx)->scc_size:(ctx)->node_size))

/********************************************************************\
|*              Context management                                  *|
\********************************************************************/

/* Initialize a context with default values */
static inline void ltl2ba_init_context(Ltl2baContext *ctx) {
  memset(ctx, 0, sizeof(Ltl2baContext));
  ctx->tl_simp_log  = 1;
  ctx->tl_simp_diff = 1;
  ctx->tl_simp_fly  = 1;
  ctx->tl_simp_scc  = 1;
  ctx->tl_fjtofj    = 1;
  ctx->node_id      = 1;
  ctx->gstate_id    = 1;
  ctx->tl_out       = stdout;
}

/* Free all tracked allocations */
static inline void ltl2ba_free_allocs(Ltl2baContext *ctx) {
  AllocBlock *b = ctx->alloc_list;
  while (b) {
    AllocBlock *next = b->next;
    free(b->ptr);
    free(b);
    b = next;
  }
  ctx->alloc_list = NULL;
}

#endif /* LTL2BA_H */
