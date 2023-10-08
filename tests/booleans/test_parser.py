from ltlplanner.booleans import parser
from ltlplanner.booleans.expressions import (
    ANDExpression,
    SymbolExpression,
    ORExpression,
    NotSymbolExpression,
    NotExpression
)


def test_basic_expression():
    expr = parser.parse("a && b || c")

    assert expr.check({"a", "b"})
    assert expr.check({"c"})
    assert not expr.check({"a"})
    assert not expr.check({"b"})
    assert expr.symbols() == {"a", "b", "c"}


def test_and_expression():
    formula = "a && b"
    expr = parser.parse(formula)
    expected = ANDExpression(SymbolExpression("a"), SymbolExpression("b"))
    assert expr.symbols() == {"a", "b"}
    assert expr == expected


def test_or_expression():
    formula = "a || b"
    expr = parser.parse(formula)
    expected = ORExpression(SymbolExpression("a"), SymbolExpression("b"))
    assert expr == expected


def test_complex_expression():
    formula = "a && ( b || c  || !d)"
    expr = parser.parse(formula)
    expected = ANDExpression(
        SymbolExpression("a"),
        ORExpression(
            ORExpression(SymbolExpression("b"), SymbolExpression("c")),
            NotSymbolExpression("d"),
        ),
    )
    assert expr == expected
    assert expr.symbols() == {"a", "b", "c", "d"}


def test_and_evaluation():
    formula = "a && b"
    expr = parser.parse(formula)
    assert expr.check(["a", "b"])
    assert not expr.check(["a"])
    assert not expr.check(["b"])
    assert expr.symbols() == {"a", "b"}


def test_multicharacter_symbol_evaluation():
    formula = "abba && a"
    expr = parser.parse(formula)
    assert expr.check(["abba", "a"])
    assert not expr.check(["b", "a"])
    assert expr.symbols() == {"abba", "a"}


def test_not_expression_equality():
    formula = "!(a && b)"
    expr = parser.parse(formula, nnf=False)
    assert expr != NotExpression(ANDExpression(SymbolExpression("a"), SymbolExpression("c")))
    assert expr == NotExpression(ANDExpression(SymbolExpression("a"), SymbolExpression("b")))
    assert expr.symbols() == {"a", "b"}
