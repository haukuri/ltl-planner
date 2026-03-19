"""
Tests for ProductGraph.

We build small Büchi automata and transition systems by hand so these tests
have no dependency on the external ltl2ba binary.
"""
from ltlplanner.buchi import Buchi
from ltlplanner.ts import TransitionSystem
from ltlplanner.product import ProductGraph
from ltlplanner.booleans.parser import parse as parse_expr


def _make_buchi():
    """
    Simple two-state Büchi automaton:
      b0 --[a]--> b1
      b1 --[1]--> b1   (self-loop, always true)
    initial: {b0}, accept: {b1}
    """
    b = Buchi()
    b.initial.add("b0")
    b.accept.add("b1")
    b.add_edge("b0", "b1", guard=parse_expr("a"))
    b.add_edge("b1", "b1", guard=parse_expr("1"))
    return b


def _make_ts():
    """
    Two-node transition system:
      s0 ---> s1
    s1 is labelled {"a"}
    initial: {s0}
    """
    ts = TransitionSystem()
    ts.initial.add("s0")
    ts.add_edge("s0", "s1")
    ts.labels("s1").add("a")
    return ts


def test_product_initial_is_cartesian_product():
    b = _make_buchi()
    ts = _make_ts()
    pg = ProductGraph(b, ts)
    assert pg.initial == {("b0", "s0")}


def test_product_initial_multiple_initials():
    b = _make_buchi()
    ts = _make_ts()
    b.initial.add("b1")
    ts.initial.add("s1")
    pg = ProductGraph(b, ts)
    assert pg.initial == {("b0", "s0"), ("b0", "s1"), ("b1", "s0"), ("b1", "s1")}


def test_product_post_includes_node_when_guard_satisfied():
    """
    From (b0, s0): buchi goes b0->b1, ts goes s0->s1, s1 has label "a".
    Guard on b0->b1 is "a". s1 has "a" so (b1, s1) should be in post.
    """
    b = _make_buchi()
    ts = _make_ts()
    pg = ProductGraph(b, ts)
    result = pg.post(("b0", "s0"))
    assert ("b1", "s1") in result


def test_product_post_excludes_node_when_guard_fails():
    """
    s0 has no labels, so guard "a" fails for s0.
    A transition to a node labelled {} should not be included when guard requires "a".
    """
    b = Buchi()
    b.initial.add("b0")
    b.add_edge("b0", "b1", guard=parse_expr("a"))

    ts = TransitionSystem()
    ts.initial.add("s0")
    ts.add_edge("s0", "s1")
    # s1 has NO labels

    pg = ProductGraph(b, ts)
    result = pg.post(("b0", "s0"))
    assert ("b1", "s1") not in result


def test_product_post_empty_when_no_successors():
    b = _make_buchi()
    ts = _make_ts()
    pg = ProductGraph(b, ts)
    # s1 has no outgoing edges in ts; b1 has self-loop
    # post of (b1, s1) requires ts to have an outgoing edge from s1 — it doesn't
    result = pg.post(("b1", "s1"))
    assert result == set()


def test_product_pre_is_inverse_of_post():
    b = _make_buchi()
    ts = _make_ts()
    pg = ProductGraph(b, ts)
    src = ("b0", "s0")
    for dst in pg.post(src):
        assert src in pg.pre(dst)


def test_product_post_true_guard_always_passes():
    """
    Self-loop on b1 has guard "1" (always true). Any ts successor should be included.
    """
    b = Buchi()
    b.initial.add("b1")
    b.add_edge("b1", "b1", guard=parse_expr("1"))

    ts = TransitionSystem()
    ts.initial.add("s0")
    ts.add_edge("s0", "s1")
    # s1 has no labels — but guard is always true, so it should pass

    pg = ProductGraph(b, ts)
    result = pg.post(("b1", "s0"))
    assert ("b1", "s1") in result
