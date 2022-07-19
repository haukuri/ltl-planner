from ltlplanner.booleans import parser
from ltlplanner.booleans.expressions import ANDExpression, NotSymbolExpression


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
