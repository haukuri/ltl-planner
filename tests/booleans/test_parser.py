from ltlplanner.booleans import parser

def test_basic_expression():
    expr = parser.parse('a && b || c')

    assert expr.check({'a', 'b'})
    assert expr.check({'c'})
    assert not expr.check({'a'})
    assert not expr.check({'b'})