# Plan: Refactor ltl2ba C Code for Python CFFI Integration

## Overview

Convert ltl2ba from a standalone CLI into a shared library callable from Python via CFFI,
with structured data output (bypassing Promela text parsing).

---

## Phase 1: Eliminate Global State — Create `Ltl2baContext`

### 1.1 Define the context struct in `ltl2ba.h`

Create a single `Ltl2baContext` struct that holds **all** mutable state currently stored in
global/static variables across all 11 C files (~60+ variables).

```c
typedef struct Ltl2baContext {
    /* --- Error handling --- */
    int            error_code;        /* 0 = OK, nonzero = error */
    char           error_msg[512];    /* human-readable error message */
    jmp_buf        error_jmp;         /* longjmp target for Fatal() */

    /* --- Configuration (from main.c) --- */
    int  tl_stats, tl_simp_log, tl_simp_diff, tl_simp_fly;
    int  tl_simp_scc, tl_fjtofj, tl_verbose, tl_terse;

    /* --- Parser state (main.c, lex.c, parse.c) --- */
    char           uform[4096];
    int            hasuform, cnt;
    int            tl_errs;
    int            tl_yychar;
    Node          *tl_yylval;
    Symbol        *symtab[Nhash + 1];
    char           yytext[2048];

    /* --- Memory management (mem.c) --- */
    unsigned long  All_Mem;
    ATrans        *atrans_list;
    GTrans        *gtrans_list;
    BTrans        *btrans_list;
    int            aallocs, afrees, apool;
    int            gallocs, gfrees, gpool;
    int            ballocs, bfrees, bpool;
    union M       *freelist[80];    /* A_LARGE = 80 */
    long           req[80];
    long           event[3][80];    /* NREVENT = 3 */

    /* --- Alternating automaton (alternating.c) --- */
    Node         **label;
    char         **sym_table;
    ATrans       **transition;
    int           *final_set;
    int            node_id, sym_id, node_size, sym_size;
    int            astate_count, atrans_count;

    /* --- Generalized Buchi (generalized.c) --- */
    GState        *gstack, *gremoved, *gstates, **init;
    GScc          *gscc_stack;
    int            init_size, gstate_id, gstate_count, gtrans_count;
    int           *fin, *gfinal;
    int            scc_id, scc_size, *bad_scc;
    int            grank;

    /* --- Buchi automaton (buchi.c) --- */
    BState        *bstack, *bstates, *bremoved;
    BScc          *bscc_stack;
    int            accept, bstate_count, btrans_count;
    int            brank;

    /* --- Cache (cache.c) --- */
    Cache         *stored;
    unsigned long  Caches, CacheHits;

    /* --- Rewrite (rewrt.c) --- */
    Node          *can;

    /* --- Trans (trans.c) --- */
    int            Stack_mx, Max_Red, Total;
    char           dumpbuf[2048];

    /* --- Output buffer (replaces FILE* tl_out) --- */
    char          *out_buf;
    size_t         out_buf_size;
    size_t         out_buf_pos;

    /* --- Structured result (populated after mk_buchi) --- */
    Ltl2baResult  *result;
} Ltl2baContext;
```

### 1.2 Define the structured result

```c
typedef struct Ltl2baEdge {
    int         src_id;
    int         dst_id;
    const char *pos_labels;   /* space-separated positive propositions */
    const char *neg_labels;   /* space-separated negated propositions */
    int         is_true_guard; /* 1 if guard is just "true" (1) */
} Ltl2baEdge;

typedef struct Ltl2baState {
    int         id;
    const char *name;         /* e.g. "T0_init", "accept_S1" */
    int         is_accept;    /* 1 if accepting state */
    int         is_initial;   /* 1 if initial state */
} Ltl2baState;

typedef struct Ltl2baResult {
    Ltl2baState *states;
    int          num_states;
    Ltl2baEdge  *edges;
    int          num_edges;
    char       **symbols;     /* all proposition names */
    int          num_symbols;
} Ltl2baResult;
```

### 1.3 Refactor each C file

**Strategy**: Every function that currently reads/writes a global gets a `Ltl2baContext *ctx`
parameter prepended. Replace every bare global reference with `ctx->member`.

Files to change (in dependency order):

| File | Globals to move | Estimated lines changed |
|------|----------------|------------------------|
| `set.c` | `mod` (constant, can leave as-is or move) | ~5 |
| `mem.c` | freelist, req, event, atrans_list, gtrans_list, btrans_list, alloc counters | ~100 |
| `cache.c` | stored, Caches, CacheHits | ~30 |
| `rewrt.c` | can | ~20 |
| `lex.c` | symtab[], yytext[] | ~40 |
| `parse.c` | tl_yychar, tl_yylval, prec[][] (prec is const — leave) | ~30 |
| `alternating.c` | label, sym_table, transition, final_set, node_id, sym_id, etc. | ~80 |
| `generalized.c` | gstack, gstates, init, fin, final, scc_stack, rank, etc. | ~100 |
| `buchi.c` | bstack, bstates, bremoved, scc_stack, rank, accept, etc. | ~100 |
| `trans.c` | Stack_mx, Max_Red, Total, dumpbuf | ~30 |
| `main.c` | All flags, uform, cnt, tl_out, tl_errs | ~60 |

**Mechanical refactoring approach:**
1. Add `Ltl2baContext *ctx` as first parameter to every non-static function
2. For static functions, either add the parameter or access ctx via a file-scope variable (less clean but faster)
3. Replace `global_var` → `ctx->global_var` throughout
4. Update all `extern` declarations to be struct members instead
5. Update the header file signatures

### 1.4 Replace `Fatal()`/`exit()` with `setjmp`/`longjmp`

```c
/* In ltl2ba.h */
#include <setjmp.h>

/* New Fatal replacement */
#define FATAL(ctx, msg) do { \
    strncpy((ctx)->error_msg, (msg), sizeof((ctx)->error_msg) - 1); \
    (ctx)->error_code = 1; \
    longjmp((ctx)->error_jmp, 1); \
} while(0)
```

All calls to `Fatal(s)` become `FATAL(ctx, s)`. The top-level API function does
`if (setjmp(ctx->error_jmp) != 0) { /* cleanup and return error */ }`.

### 1.5 Replace `fprintf(tl_out, ...)` with buffer writes

Since we're producing structured output, most `fprintf(tl_out, ...)` calls in the
printing functions can be removed or redirected to a debug buffer. The structured
result is populated by walking the final `bstates` linked list directly.

---

## Phase 2: Add Structured Output Extraction

### 2.1 New file: `ltl2ba/result.c`

After `mk_buchi()` completes, walk the Buchi automaton data structures and populate
an `Ltl2baResult`:

```c
Ltl2baResult *extract_result(Ltl2baContext *ctx) {
    /* Count states by walking ctx->bstates linked list */
    /* Count edges by walking each state's trans linked list */
    /* Allocate result arrays */
    /* Populate state names (using same naming scheme as print_buchi):
       - "T0_init" for initial, "T0_S{id}" for normal, "accept_S{id}" for accepting */
    /* Populate edges with positive/negative proposition labels from
       pos/neg bitsets, resolved via ctx->sym_table */
    return result;
}
```

### 2.2 Name generation

The existing `print_buchi()` function in `buchi.c` (around line 500+) generates state names
like `T0_init`, `T0_S2`, `accept_S4`. We replicate this logic in `extract_result()` using
the same `id` and `final` fields.

### 2.3 Guard representation

Each edge's guard is a conjunction of positive and negative propositions stored as bitsets
(`pos` and `neg` fields of `BTrans`). We convert these to string lists using `sym_table[]`.
Special case: if both `pos` and `neg` are empty, the guard is "true" (always enabled).

---

## Phase 3: Public C API

### 3.1 New file: `ltl2ba/ltl2ba_api.h`

This is the **only** header CFFI needs to see — a clean, minimal C API:

```c
/* ltl2ba_api.h — Public API for calling ltl2ba from external programs */

typedef struct Ltl2baEdge {
    int         src_id;
    int         dst_id;
    const char *pos_labels;
    const char *neg_labels;
    int         is_true_guard;
} Ltl2baEdge;

typedef struct Ltl2baState {
    int         id;
    const char *name;
    int         is_accept;
    int         is_initial;
} Ltl2baState;

typedef struct Ltl2baResult {
    Ltl2baState *states;
    int          num_states;
    Ltl2baEdge  *edges;
    int          num_edges;
    char       **symbols;
    int          num_symbols;
    const char  *error_msg;   /* NULL if no error */
} Ltl2baResult;

/* Opaque handle to internal context */
typedef struct Ltl2baContext Ltl2baContext;

/* Create/destroy context */
Ltl2baContext *ltl2ba_create(void);
void           ltl2ba_destroy(Ltl2baContext *ctx);

/* Main conversion function — returns NULL on allocation failure */
Ltl2baResult  *ltl2ba_convert(Ltl2baContext *ctx, const char *formula);

/* Free a result */
void           ltl2ba_free_result(Ltl2baResult *result);
```

### 3.2 Implementation: `ltl2ba/ltl2ba_api.c`

```c
Ltl2baContext *ltl2ba_create(void) {
    Ltl2baContext *ctx = calloc(1, sizeof(Ltl2baContext));
    /* Set defaults */
    ctx->tl_simp_log = ctx->tl_simp_diff = ctx->tl_simp_fly = 1;
    ctx->tl_simp_scc = ctx->tl_fjtofj = 1;
    ctx->node_id = 1;
    ctx->gstate_id = 1;
    return ctx;
}

Ltl2baResult *ltl2ba_convert(Ltl2baContext *ctx, const char *formula) {
    /* Reset mutable state for re-entrant calls */
    reset_context(ctx);

    /* Copy formula */
    strncpy(ctx->uform, formula, sizeof(ctx->uform) - 1);
    ctx->hasuform = strlen(ctx->uform);

    /* Set up error recovery */
    if (setjmp(ctx->error_jmp) != 0) {
        /* Fatal error occurred — cleanup and return error result */
        Ltl2baResult *err = calloc(1, sizeof(Ltl2baResult));
        err->error_msg = strdup(ctx->error_msg);
        cleanup_context(ctx);
        return err;
    }

    /* Run the pipeline: parse → alternating → generalized → buchi */
    tl_parse(ctx);
    /* (tl_parse internally calls trans() which calls mk_alternating,
        mk_generalized, mk_buchi) */

    /* Extract structured result */
    Ltl2baResult *result = extract_result(ctx);

    /* Cleanup internal allocations */
    cleanup_context(ctx);

    return result;
}

void ltl2ba_destroy(Ltl2baContext *ctx) {
    cleanup_context(ctx);
    free(ctx);
}

void ltl2ba_free_result(Ltl2baResult *result) {
    /* Free all strings and arrays in result */
    ...
    free(result);
}
```

---

## Phase 4: Build as Shared Library

### 4.1 Update `ltl2ba/Makefile`

```makefile
CC=gcc
CFLAGS= -O3 -ansi -DNXT -fPIC

OBJS= parse.o lex.o main.o trans.o buchi.o set.o \
      mem.o rewrt.o cache.o alternating.o generalized.o \
      result.o ltl2ba_api.o

# Standalone binary (keep for testing)
ltl2ba: $(OBJS)
	$(CC) $(CFLAGS) -o ltl2ba $(OBJS)

# Shared library for CFFI
libltl2ba.so: $(OBJS)
	$(CC) $(CFLAGS) -shared -o libltl2ba.so $(OBJS)

$(OBJS): ltl2ba.h

clean:
	rm -f ltl2ba libltl2ba.so *.o core
```

Key change: **`-fPIC`** is required for shared library compilation.

---

## Phase 5: CFFI Python Binding

### 5.1 New file: `ltlplanner/ltl2ba_cffi_build.py`

This is the CFFI "build" script (ABI mode — no compiler needed at import time):

```python
import cffi
from pathlib import Path

ffi = cffi.FFI()

# Read the public API header for CFFI
api_header = (Path(__file__).parent.parent / "ltl2ba" / "ltl2ba_api.h").read_text()
ffi.cdef(api_header)

# We'll load the .so at runtime (ABI mode)
```

### 5.2 New file: `ltlplanner/ltl2ba_cffi.py`

Runtime wrapper that loads the `.so` and provides a Pythonic API:

```python
import cffi
from pathlib import Path

ffi = cffi.FFI()

# Declare the API types and functions
ffi.cdef("""
    typedef struct Ltl2baEdge { ... } Ltl2baEdge;
    typedef struct Ltl2baState { ... } Ltl2baState;
    typedef struct Ltl2baResult { ... } Ltl2baResult;
    typedef struct Ltl2baContext Ltl2baContext;

    Ltl2baContext *ltl2ba_create(void);
    void           ltl2ba_destroy(Ltl2baContext *ctx);
    Ltl2baResult  *ltl2ba_convert(Ltl2baContext *ctx, const char *formula);
    void           ltl2ba_free_result(Ltl2baResult *result);
""")

_lib_path = Path(__file__).parent.parent / "ltl2ba" / "libltl2ba.so"
lib = ffi.dlopen(str(_lib_path))


def convert(formula: str) -> dict:
    """Convert an LTL formula to a Buchi automaton.

    Returns a dict with keys: states, edges, symbols, initial, accept.
    """
    ctx = lib.ltl2ba_create()
    try:
        result = lib.ltl2ba_convert(ctx, formula.encode("utf-8"))
        try:
            if result.error_msg != ffi.NULL:
                raise RuntimeError(ffi.string(result.error_msg).decode())

            states = {}
            initial = set()
            accept = set()
            for i in range(result.num_states):
                s = result.states[i]
                name = ffi.string(s.name).decode()
                states[s.id] = name
                if s.is_initial:
                    initial.add(name)
                if s.is_accept:
                    accept.add(name)

            edges = {}
            for i in range(result.num_edges):
                e = result.edges[i]
                src = states[e.src_id]
                dst = states[e.dst_id]
                if e.is_true_guard:
                    guard = "(1)"
                else:
                    parts = []
                    if e.pos_labels != ffi.NULL:
                        for p in ffi.string(e.pos_labels).decode().split():
                            parts.append(p)
                    if e.neg_labels != ffi.NULL:
                        for n in ffi.string(e.neg_labels).decode().split():
                            parts.append(f"!{n}")
                    guard = "(" + " && ".join(parts) + ")"
                edges[(src, dst)] = guard

            return {
                "states": set(states.values()),
                "edges": edges,
                "initial": initial,
                "accept": accept,
            }
        finally:
            lib.ltl2ba_free_result(result)
    finally:
        lib.ltl2ba_destroy(ctx)
```

### 5.3 Update `ltlplanner/ltl2ba_wrapper.py`

```python
from .ltl2ba_cffi import convert

def run_ltl2ba(ltl_formula: str) -> dict:
    """Convert LTL formula to Buchi automaton via CFFI (no subprocess)."""
    return convert(ltl_formula)
```

### 5.4 Update `ltlplanner/buchi.py`

Add a new `from_cffi_result` class method that takes the structured dict directly,
bypassing `promela.py`:

```python
@classmethod
def from_ltl(cls, formula: str) -> "Buchi":
    result = run_ltl2ba(formula)  # now returns structured dict
    return cls.from_cffi_result(result)

@classmethod
def from_cffi_result(cls, result: dict) -> "Buchi":
    buchi = cls()
    buchi.initial |= result["initial"]
    buchi.accept |= result["accept"]
    for (src, dst), guard_str in result["edges"].items():
        guard = parse_boolean_expression(guard_str)
        buchi.add_edge(src, dst, guard=guard)
    return buchi
```

### 5.5 Add `cffi` dependency to `pyproject.toml`

```toml
dependencies = [
    "cffi>=1.16.0",
    ...
]
```

---

## Phase 6: Build System Integration

### 6.1 Update top-level `Makefile`

```makefile
dependencies:
	$(MAKE) -C ltl2ba libltl2ba.so
```

### 6.2 Alternative: `pyproject.toml` build hook

If using `uv`/`pip install -e .`, add a build script that compiles the .so automatically.
This can be a simple `[tool.setuptools.cmdclass]` override or a Makefile invocation.

---

## Implementation Order (incremental, testable at each step)

| Step | Description | Testability |
|------|-------------|-------------|
| 1 | Add `-fPIC` to Makefile, verify existing binary still works | `make && ./ltl2ba -f '[]<>a'` |
| 2 | Define `Ltl2baContext` struct in `ltl2ba.h` | Compiles |
| 3 | Refactor `set.c` and `mem.c` (lowest dependencies) | Unit-testable |
| 4 | Refactor `cache.c`, `rewrt.c` | Compiles |
| 5 | Refactor `lex.c`, `parse.c` | Compiles |
| 6 | Refactor `alternating.c`, `generalized.c`, `buchi.c` | Compiles |
| 7 | Refactor `trans.c`, `main.c` | Full binary works again |
| 8 | Replace `Fatal()`/`exit()` with longjmp | Binary still works |
| 9 | Add `result.c` — structured output extraction | Can test via main.c |
| 10 | Add `ltl2ba_api.c` + `ltl2ba_api.h` — public API | Build .so, test from C |
| 11 | Add CFFI Python wrapper | `python -c "from ltlplanner.ltl2ba_cffi import convert; print(convert('[]<>a'))"` |
| 12 | Update `buchi.py` and `ltl2ba_wrapper.py` | Existing tests pass |
| 13 | Remove subprocess dependency | All tests pass |

---

## Risk Mitigation

- **Keep the CLI working** throughout refactoring — `main()` just creates a context and
  calls the same pipeline. Existing tests validate correctness at each step.
- **Refactor mechanically** — the global→ctx transformation is repetitive but safe.
  Each file can be refactored and tested independently.
- **setjmp/longjmp is safe** here because we don't have C++ destructors or complex
  stack-allocated resources. All allocations go through the context's memory pools.
- **CFFI ABI mode** means no compilation at import time — just `dlopen`. This is simpler
  than API mode and sufficient for our use case.
