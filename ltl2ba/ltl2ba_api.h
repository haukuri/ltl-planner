/***** ltl2ba : ltl2ba_api.h *****/
/* Public C API for calling ltl2ba as a library */

#ifndef LTL2BA_API_H
#define LTL2BA_API_H

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************\
|*                  Result data structures                          *|
\********************************************************************/

typedef struct Ltl2baEdge {
  char *src;       /* source state name (e.g. "accept_init", "T0_S3") */
  char *dst;       /* destination state name */
  char *guard;     /* guard expression (e.g. "(1)", "(p) && !(q)") */
} Ltl2baEdge;

typedef struct Ltl2baState {
  char *name;      /* state name */
  int  is_initial; /* 1 if this is the initial state */
  int  is_accept;  /* 1 if this is an accepting state */
} Ltl2baState;

typedef struct Ltl2baResult {
  int           ok;           /* 1 = success, 0 = error */
  char         *error_msg;    /* error message if !ok, NULL otherwise */

  Ltl2baState  *states;       /* array of states */
  int           num_states;

  Ltl2baEdge   *edges;        /* array of edges */
  int           num_edges;
} Ltl2baResult;

/********************************************************************\
|*                    API functions                                  *|
\********************************************************************/

/* Translate an LTL formula to a Buchi automaton.
 * Returns a result struct with states and edges.
 * The caller must free the result with ltl2ba_free_result(). */
Ltl2baResult *ltl2ba_translate(const char *formula);

/* Free a result previously returned by ltl2ba_translate(). */
void ltl2ba_free_result(Ltl2baResult *result);

#ifdef __cplusplus
}
#endif

#endif /* LTL2BA_API_H */
