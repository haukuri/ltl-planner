/***** ltl2ba : ltl2ba_api.c *****/
/* Public C API implementation */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "ltl2ba.h"
#include "ltl2ba_api.h"

/********************************************************************\
|*              Helper: format state name                           *|
\********************************************************************/

static char *format_state_name(Ltl2baContext *ctx, BState *s)
{
  char buf[256];
  if (s->id == -1) {
    if (s->final == ctx->accept)
      snprintf(buf, sizeof(buf), "accept_init");
    else
      snprintf(buf, sizeof(buf), "T%d_init", s->final);
  } else if (s->id == 0) {
    if (s->final == ctx->accept)
      snprintf(buf, sizeof(buf), "accept_all");
    else
      snprintf(buf, sizeof(buf), "T%d_all", s->final);
  } else {
    if (s->final == ctx->accept)
      snprintf(buf, sizeof(buf), "accept_S%d", s->id);
    else
      snprintf(buf, sizeof(buf), "T%d_S%d", s->final, s->id);
  }
  return strdup(buf);
}

/********************************************************************\
|*         Helper: format guard expression for an edge              *|
\********************************************************************/

static void append_str(char **buf, int *len, int *cap, const char *s)
{
  int slen = strlen(s);
  while (*len + slen + 1 > *cap) {
    *cap *= 2;
    *buf = realloc(*buf, *cap);
  }
  memcpy(*buf + *len, s, slen);
  *len += slen;
  (*buf)[*len] = '\0';
}

static char *format_guard(Ltl2baContext *ctx, int *pos, int *neg)
{
  int cap = 128, len = 0;
  char *buf = malloc(cap);
  buf[0] = '\0';
  int first = 1;
  int i;
  int sz = ctx->sym_size;

  /* Check if guard is "true" (no pos or neg constraints) */
  int any = 0;
  for (i = 0; i < sz; i++) {
    if (pos[i] || neg[i]) { any = 1; break; }
  }
  if (!any) {
    append_str(&buf, &len, &cap, "(1)");
    return buf;
  }

  for (i = 0; i < sz * MOD; i++) {
    if (in_set(pos, i)) {
      if (!first) append_str(&buf, &len, &cap, " && ");
      append_str(&buf, &len, &cap, "(");
      append_str(&buf, &len, &cap, ctx->sym_table[i]);
      append_str(&buf, &len, &cap, ")");
      first = 0;
    }
  }
  for (i = 0; i < sz * MOD; i++) {
    if (in_set(neg, i)) {
      if (!first) append_str(&buf, &len, &cap, " && ");
      append_str(&buf, &len, &cap, "!(");
      append_str(&buf, &len, &cap, ctx->sym_table[i]);
      append_str(&buf, &len, &cap, ")");
      first = 0;
    }
  }
  return buf;
}

/********************************************************************\
|*              Count states and edges                              *|
\********************************************************************/

static int count_bstates(Ltl2baContext *ctx)
{
  int count = 0;
  BState *s;
  for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt)
    count++;
  return count;
}

static int count_bedges(Ltl2baContext *ctx)
{
  int count = 0;
  BState *s;
  BTrans *t;
  for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt)
    for (t = s->trans->nxt; t != s->trans; t = t->nxt)
      count++;
  return count;
}

/********************************************************************\
|*                    Main API function                              *|
\********************************************************************/

Ltl2baResult *ltl2ba_translate(const char *formula)
{
  Ltl2baResult *result = calloc(1, sizeof(Ltl2baResult));
  Ltl2baContext ctx_storage;
  Ltl2baContext *ctx = &ctx_storage;

  ltl2ba_init_context(ctx);

  /* Suppress output - write to /dev/null */
  ctx->tl_out = fopen("/dev/null", "w");
  if (!ctx->tl_out) ctx->tl_out = stdout;

  /* Copy formula */
  strncpy(ctx->uform, formula, sizeof(ctx->uform) - 1);
  ctx->uform[sizeof(ctx->uform) - 1] = '\0';
  /* Replace tabs and quotes with spaces */
  {
    int i;
    for (i = 0; ctx->uform[i]; i++) {
      if (ctx->uform[i] == '\t' || ctx->uform[i] == '\"' || ctx->uform[i] == '\n')
        ctx->uform[i] = ' ';
    }
  }
  ctx->hasuform = strlen(ctx->uform);

  /* Set up error recovery */
  if (setjmp(ctx->error_jmp) != 0) {
    /* Error occurred during translation */
    result->ok = 0;
    result->error_msg = strdup(ctx->error_msg);
    if (ctx->tl_out && ctx->tl_out != stdout)
      fclose(ctx->tl_out);
    ltl2ba_free_allocs(ctx);
    return result;
  }

  /* Run the translation pipeline */
  tl_parse(ctx);

  if (ctx->tl_errs) {
    result->ok = 0;
    result->error_msg = strdup("Parse error in LTL formula");
    if (ctx->tl_out && ctx->tl_out != stdout)
      fclose(ctx->tl_out);
    ltl2ba_free_allocs(ctx);
    return result;
  }

  /* Extract results from the Buchi automaton */
  result->ok = 1;

  if (!ctx->bstates || ctx->bstates->nxt == ctx->bstates) {
    /* Empty automaton */
    result->num_states = 0;
    result->num_edges = 0;
    result->states = NULL;
    result->edges = NULL;
  } else {
    int ns = count_bstates(ctx);
    int ne = count_bedges(ctx);
    BState *s;
    BTrans *t;
    int si, ei;

    result->num_states = ns;
    result->states = calloc(ns, sizeof(Ltl2baState));
    result->num_edges = ne;
    result->edges = calloc(ne, sizeof(Ltl2baEdge));

    /* Fill states */
    si = 0;
    for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt) {
      result->states[si].name = format_state_name(ctx, s);
      result->states[si].is_initial = (s->id == -1) ? 1 : 0;
      result->states[si].is_accept = (s->final == ctx->accept) ? 1 : 0;
      si++;
    }

    /* Fill edges */
    ei = 0;
    for (s = ctx->bstates->nxt; s != ctx->bstates; s = s->nxt) {
      char *src_name = format_state_name(ctx, s);
      for (t = s->trans->nxt; t != s->trans; t = t->nxt) {
        result->edges[ei].src = strdup(src_name);
        result->edges[ei].dst = format_state_name(ctx, t->to);
        result->edges[ei].guard = format_guard(ctx, t->pos, t->neg);
        ei++;
      }
      free(src_name);
    }
  }

  if (ctx->tl_out && ctx->tl_out != stdout)
    fclose(ctx->tl_out);
  ltl2ba_free_allocs(ctx);
  return result;
}

void ltl2ba_free_result(Ltl2baResult *result)
{
  int i;
  if (!result) return;
  if (result->error_msg) free(result->error_msg);
  for (i = 0; i < result->num_states; i++) {
    free(result->states[i].name);
  }
  free(result->states);
  for (i = 0; i < result->num_edges; i++) {
    free(result->edges[i].src);
    free(result->edges[i].dst);
    free(result->edges[i].guard);
  }
  free(result->edges);
  free(result);
}
