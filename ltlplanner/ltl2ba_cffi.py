"""CFFI-based binding to the ltl2ba shared library."""

from pathlib import Path

import cffi

ffi = cffi.FFI()

ffi.cdef("""
    typedef struct Ltl2baEdge {
        char *src;
        char *dst;
        char *guard;
    } Ltl2baEdge;

    typedef struct Ltl2baState {
        char *name;
        int is_initial;
        int is_accept;
    } Ltl2baState;

    typedef struct Ltl2baResult {
        int ok;
        char *error_msg;
        Ltl2baState *states;
        int num_states;
        Ltl2baEdge *edges;
        int num_edges;
    } Ltl2baResult;

    Ltl2baResult *ltl2ba_translate(const char *formula);
    void ltl2ba_free_result(Ltl2baResult *result);
""")

_lib_dir = Path(__file__).parent.parent / "ltl2ba"
_lib_path = _lib_dir / "libltl2ba.so"
_lib = None


def _get_lib():
    global _lib
    if _lib is None:
        if not _lib_path.exists():
            raise RuntimeError(
                f"libltl2ba.so not found at {_lib_path}. "
                "Run 'make libltl2ba.so' in the ltl2ba directory first."
            )
        _lib = ffi.dlopen(str(_lib_path))
    return _lib


class Ltl2baError(Exception):
    pass


def translate(formula: str) -> dict:
    """Translate an LTL formula to a Buchi automaton.

    Returns a dict with keys:
        states: list of dicts with 'name', 'is_initial', 'is_accept'
        edges: list of dicts with 'src', 'dst', 'guard'
    """
    lib = _get_lib()
    result = lib.ltl2ba_translate(formula.encode("utf-8"))
    try:
        if not result.ok:
            msg = ffi.string(result.error_msg).decode("utf-8") if result.error_msg else "Unknown error"
            raise Ltl2baError(msg)

        states = []
        for i in range(result.num_states):
            s = result.states[i]
            states.append({
                "name": ffi.string(s.name).decode("utf-8"),
                "is_initial": bool(s.is_initial),
                "is_accept": bool(s.is_accept),
            })

        edges = []
        for i in range(result.num_edges):
            e = result.edges[i]
            edges.append({
                "src": ffi.string(e.src).decode("utf-8"),
                "dst": ffi.string(e.dst).decode("utf-8"),
                "guard": ffi.string(e.guard).decode("utf-8"),
            })

        return {"states": states, "edges": edges}
    finally:
        lib.ltl2ba_free_result(result)
