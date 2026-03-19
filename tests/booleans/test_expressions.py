from ltlplanner.booleans import parser
from ltlplanner.booleans.expressions import (
    ANDExpression,
    ORExpression,
    NotExpression,
    NotSymbolExpression,
    SymbolExpression,
    TrueExpression,
)


def test_boolean_distance():
    """
    Tests the computation of a "distance" between a set of atomic propositions and
    a boolean expression
    """
    formula = "!(a || b) || a"
    expr = parser.parse(formula)
    assert expr.distance(["a"]) == 0, "Distance from a=True, b=False: 0"
    assert expr.distance(["b"]) == 1, "Distance from a=False, b=True: 1"


def test_negative_normal_form():
    formula = "!(a || b)"
    actual = parser.parse(formula).nnf()
    expected = ANDExpression(NotSymbolExpression("a"), NotSymbolExpression("b"))
    assert actual == expected


# --- TrueExpression ---

def test_true_expression_check_always_true():
    expr = TrueExpression()
    assert expr.check([]) is True
    assert expr.check(["a", "b", "c"]) is True


def test_true_expression_distance_always_zero():
    expr = TrueExpression()
    assert expr.distance([]) == 0
    assert expr.distance(["anything"]) == 0


# --- NotSymbolExpression ---

def test_not_symbol_check_absent():
    expr = NotSymbolExpression("x")
    assert expr.check([]) is True
    assert expr.check(["y"]) is True


def test_not_symbol_check_present():
    expr = NotSymbolExpression("x")
    assert expr.check(["x"]) is False


def test_not_symbol_distance_absent():
    expr = NotSymbolExpression("x")
    assert expr.distance([]) == 0


def test_not_symbol_distance_present():
    expr = NotSymbolExpression("x")
    assert expr.distance(["x"]) == 1


# --- NotExpression ---

def test_not_expression_check_negates_inner():
    inner = SymbolExpression("a")
    expr = NotExpression(inner)
    assert expr.check(["a"]) is False
    assert expr.check([]) is True


def test_not_expression_nnf_and_becomes_or():
    # !(a && b) -> (!a || !b)
    formula = "!(a && b)"
    actual = parser.parse(formula).nnf()
    expected = ORExpression(NotSymbolExpression("a"), NotSymbolExpression("b"))
    assert actual == expected


# --- ORExpression distance ---

def test_or_expression_distance_takes_minimum():
    # distance(a || b) where only b is present -> min(1, 0) = 0
    expr = ORExpression(SymbolExpression("a"), SymbolExpression("b"))
    assert expr.distance(["b"]) == 0
    assert expr.distance([]) == 1   # min(1, 1)
    assert expr.distance(["a", "b"]) == 0


# --- ANDExpression distance ---

def test_and_expression_distance_sums_children():
    # distance(a && b) where neither is present -> 1 + 1 = 2
    expr = ANDExpression(SymbolExpression("a"), SymbolExpression("b"))
    assert expr.distance([]) == 2
    assert expr.distance(["a"]) == 1
    assert expr.distance(["a", "b"]) == 0
