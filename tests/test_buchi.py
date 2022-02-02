from ltlplanner import buchi

def test_buchi_from_ltl():
    ltl = '[]<>a && []<>b '
    buchi_graph = buchi.from_ltl(ltl)
    actual_guard = buchi_graph.edges["T0_init", "accept_S1"]["guard"]
    assert actual_guard.check(["a", "b"]) == True
    assert actual_guard.check(["a"]) == False
    assert actual_guard.check(["b"]) == False
