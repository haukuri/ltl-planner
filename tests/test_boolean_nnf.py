from ltlplanner.booleans import parser
from ltlplanner.booleans.expressions import *


def test_negative_normal_form():
    formula = "!(a || b)"
    actual = parser.parse(formula).nnf()
    expected = ANDExpression(NotSymbolExpression("a"), NotSymbolExpression("b"))
    assert actual == expected
