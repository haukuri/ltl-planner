from ltlplanner.booleans import parser
from ltlplanner.booleans.expressions import *


def test_and_expression():
    formula = "a && b"
    parsed = parser.parse(formula)
    expected = ANDExpression(SymbolExpression("a"), SymbolExpression("b"))
    assert parsed == expected


def test_or_expression():
    formula = "a || b"
    parsed = parser.parse(formula)
    expected = ORExpression(SymbolExpression("a"), SymbolExpression("b"))
    assert parsed == expected


def test_complex_expression():
    formula = "a && ( b || c  || !d)"
    parsed = parser.parse(formula)
    expected = ANDExpression(
        SymbolExpression("a"),
        ORExpression(
            ORExpression(SymbolExpression("b"), SymbolExpression("c")),
            NotSymbolExpression("d"),
        ),
    )
    assert parsed == expected
